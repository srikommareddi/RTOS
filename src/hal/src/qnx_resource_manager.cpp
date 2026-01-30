#include "../include/hal/qnx_resource_manager.h"

#if defined(__QNXNTO__)
#include <sys/dispatch.h>
#include <sys/iofunc.h>
#include <unistd.h>

#include <devctl.h>
#include <errno.h>
#include <cstring>
#include <mutex>
#include <string>
#endif

namespace rtos {
namespace hal {

#if defined(__QNXNTO__)
namespace {

constexpr int kMaxMessageSize = 2048;

struct DeviceState {
  std::string payload = "rtos_qnx_resmgr\n";
  bool allow_write = true;
  int open_count = 0;
  std::mutex mutex;
};

struct DeviceAttr {
  iofunc_attr_t attr;
  DeviceState* state = nullptr;
};

struct DeviceStatus {
  int open_count;
  int allow_write;
  int payload_size;
};

enum {
  DCMD_QNX_GET_STATUS = __DIOF(_DCMD_MISC, 0x80, DeviceStatus),
  DCMD_QNX_SET_WRITE = __DIOT(_DCMD_MISC, 0x81, int)
};

int io_open(resmgr_context_t* ctp, io_open_t* msg,
            RESMGR_HANDLE_T* handle, void* extra) {
  auto* attr = reinterpret_cast<DeviceAttr*>(handle);
  auto* state = attr->state;
  {
    std::lock_guard<std::mutex> lock(state->mutex);
    if (!state->allow_write &&
        (msg->connect.ioflag & (O_WRONLY | O_RDWR))) {
      return EACCES;
    }
    state->open_count++;
  }
  return iofunc_open_default(ctp, msg, &attr->attr, extra);
}

int io_close_ocb(resmgr_context_t* ctp, void* reserved, RESMGR_OCB_T* ocb) {
  auto* attr = reinterpret_cast<DeviceAttr*>(
      static_cast<iofunc_ocb_t*>(ocb)->attr);
  if (attr && attr->state) {
    std::lock_guard<std::mutex> lock(attr->state->mutex);
    if (attr->state->open_count > 0) {
      attr->state->open_count--;
    }
  }
  return iofunc_close_ocb_default(ctp, reserved, ocb);
}

int io_read(resmgr_context_t* ctp, io_read_t* msg, iofunc_ocb_t* ocb) {
  int status = iofunc_read_verify(ctp, msg, ocb, nullptr);
  if (status != EOK) {
    return status;
  }
  if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
    return ENOSYS;
  }

  auto* attr = reinterpret_cast<DeviceAttr*>(ocb->attr);
  auto* state = attr->state;
  std::string payload;
  {
    std::lock_guard<std::mutex> lock(state->mutex);
    payload = state->payload;
  }

  int bytes =
      static_cast<int>(payload.size()) - static_cast<int>(ocb->offset);
  if (bytes < 0) {
    bytes = 0;
  }
  if (bytes > msg->i.nbytes) {
    bytes = msg->i.nbytes;
  }

  _IO_SET_READ_NBYTES(ctp, bytes);
  if (bytes > 0) {
    resmgr_msgwrite(ctp, payload.data() + ocb->offset, bytes, 0);
    ocb->offset += bytes;
  }
  return _RESMGR_NPARTS(0);
}

int io_write(resmgr_context_t* ctp, io_write_t* msg, iofunc_ocb_t* ocb) {
  int status = iofunc_write_verify(ctp, msg, ocb, nullptr);
  if (status != EOK) {
    return status;
  }

  if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
    return ENOSYS;
  }

  auto* attr = reinterpret_cast<DeviceAttr*>(ocb->attr);
  auto* state = attr->state;

  {
    std::lock_guard<std::mutex> lock(state->mutex);
    if (!state->allow_write) {
      return EACCES;
    }
  }

  std::string buffer;
  buffer.resize(msg->i.nbytes);
  int nbytes =
      resmgr_msgread(ctp, buffer.data(), msg->i.nbytes, sizeof(msg->i));
  if (nbytes < 0) {
    return nbytes;
  }

  {
    std::lock_guard<std::mutex> lock(state->mutex);
    state->payload.assign(buffer.begin(), buffer.begin() + nbytes);
  }
  _IO_SET_WRITE_NBYTES(ctp, nbytes);
  return _RESMGR_NPARTS(0);
}

int io_devctl(resmgr_context_t* ctp, io_devctl_t* msg, iofunc_ocb_t* ocb) {
  auto* attr = reinterpret_cast<DeviceAttr*>(ocb->attr);
  auto* state = attr->state;

  switch (msg->i.dcmd) {
    case DCMD_QNX_GET_STATUS: {
      DeviceStatus status{};
      {
        std::lock_guard<std::mutex> lock(state->mutex);
        status.open_count = state->open_count;
        status.allow_write = state->allow_write ? 1 : 0;
        status.payload_size = static_cast<int>(state->payload.size());
      }
      std::memcpy(_DEVCTL_DATA(msg->o), &status, sizeof(status));
      msg->o.ret_val = 0;
      msg->o.nbytes = sizeof(status);
      return _RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + msg->o.nbytes);
    }
    case DCMD_QNX_SET_WRITE: {
      int allow = 0;
      std::memcpy(&allow, _DEVCTL_DATA(msg->i), sizeof(allow));
      {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->allow_write = (allow != 0);
      }
      msg->o.ret_val = 0;
      msg->o.nbytes = 0;
      return _RESMGR_PTR(ctp, &msg->o, sizeof(msg->o));
    }
    default:
      return ENOSYS;
  }
}

}  // namespace
#endif

QnxResourceManager::QnxResourceManager() = default;

QnxResourceManager::~QnxResourceManager() {
  stop();
}

bool QnxResourceManager::start(const std::string& mount_point) {
#if defined(__QNXNTO__)
  if (running_) {
    return true;
  }

  mount_point_ = mount_point;
  dispatch_t* dpp = dispatch_create();
  if (!dpp) {
    return false;
  }

  resmgr_attr_t rattr{};
  rattr.nparts_max = 1;
  rattr.msg_max_size = kMaxMessageSize;

  static DeviceState device_state;
  static DeviceAttr device_attr;
  iofunc_attr_init(&device_attr.attr, S_IFCHR | 0666, nullptr, nullptr);
  device_attr.state = &device_state;

  resmgr_connect_funcs_t connect_funcs{};
  resmgr_io_funcs_t io_funcs{};
  iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                   _RESMGR_IO_NFUNCS, &io_funcs);

  connect_funcs.open = io_open;
  io_funcs.read = io_read;
  io_funcs.write = io_write;
  io_funcs.devctl = io_devctl;
  io_funcs.close_ocb = io_close_ocb;

  int id = resmgr_attach(dpp, &rattr, mount_point_.c_str(), _FTYPE_ANY, 0,
                         &connect_funcs, &io_funcs, &device_attr);
  if (id < 0) {
    dispatch_destroy(dpp);
    return false;
  }

  running_ = true;
  dispatch_handle_ = dpp;
  resmgr_id_ = id;
  dispatch_thread_ = std::thread([this]() {
    auto* dpp_local = static_cast<dispatch_t*>(dispatch_handle_);
    auto* ctx = dispatch_context_alloc(dpp_local);
    dispatch_context_ = ctx;
    while (running_) {
      if (!dispatch_block(ctx)) {
        continue;
      }
      dispatch_handler(ctx);
    }
    dispatch_context_free(ctx);
    dispatch_context_ = nullptr;
  });

  return true;
#else
  (void)mount_point;
  return false;
#endif
}

void QnxResourceManager::stop() {
  if (!running_) {
    return;
  }
  running_ = false;

#if defined(__QNXNTO__)
  if (dispatch_handle_) {
    auto* dpp_local = static_cast<dispatch_t*>(dispatch_handle_);
    if (dispatch_context_) {
      dispatch_unblock(static_cast<dispatch_context_t*>(dispatch_context_));
    }
    if (dispatch_thread_.joinable()) {
      dispatch_thread_.join();
    }
    if (resmgr_id_ >= 0) {
      resmgr_detach(dpp_local, resmgr_id_, 0);
      resmgr_id_ = -1;
    }
    dispatch_destroy(dpp_local);
    dispatch_handle_ = nullptr;
  }
#else
  if (dispatch_thread_.joinable()) {
    dispatch_thread_.join();
  }
#endif
}

}  // namespace hal
}  // namespace rtos
