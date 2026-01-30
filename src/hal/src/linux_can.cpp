#include "../include/hal/linux_can.h"

#if defined(__linux__)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

#endif

namespace rtos {
namespace hal {

LinuxCanBus::LinuxCanBus() = default;

LinuxCanBus::~LinuxCanBus() {
  close();
}

bool LinuxCanBus::open(const std::string& device) {
#if defined(__linux__)
  socket_fd_ = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (socket_fd_ < 0) {
    return false;
  }

  ifreq ifr{};
  std::snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", device.c_str());
  if (::ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
    close();
    return false;
  }

  sockaddr_can addr{};
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (::bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    close();
    return false;
  }
  return true;
#else
  (void)device;
  return false;
#endif
}

void LinuxCanBus::close() {
#if defined(__linux__)
  if (socket_fd_ >= 0) {
    ::close(socket_fd_);
    socket_fd_ = -1;
  }
#endif
}

bool LinuxCanBus::write_frame(const CanFrame& frame) {
#if defined(__linux__)
  if (socket_fd_ < 0) {
    return false;
  }
  can_frame out{};
  out.can_id = frame.id;
  out.can_dlc = static_cast<__u8>(std::min<size_t>(frame.data.size(), 8));
  std::copy(frame.data.begin(),
            frame.data.begin() + out.can_dlc,
            out.data);
  return ::write(socket_fd_, &out, sizeof(out)) == sizeof(out);
#else
  (void)frame;
  return false;
#endif
}

bool LinuxCanBus::read_frame(CanFrame* frame) {
#if defined(__linux__)
  if (socket_fd_ < 0 || !frame) {
    return false;
  }
  can_frame in{};
  if (::read(socket_fd_, &in, sizeof(in)) != sizeof(in)) {
    return false;
  }
  frame->id = in.can_id;
  frame->data.assign(in.data, in.data + in.can_dlc);
  return true;
#else
  (void)frame;
  return false;
#endif
}

}  // namespace hal
}  // namespace rtos
