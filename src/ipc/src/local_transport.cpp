#include "../include/ipc/local_transport.h"

namespace rtos {
namespace ipc {

void LocalTransport::start(TransportReceiveHandler handler) {
  std::lock_guard<std::mutex> lock(mutex_);
  handler_ = std::move(handler);
}

void LocalTransport::stop() {
  std::lock_guard<std::mutex> lock(mutex_);
  handler_ = nullptr;
}

bool LocalTransport::publish(const std::vector<uint8_t>& bytes) {
  TransportReceiveHandler handler;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    handler = handler_;
  }

  if (!handler) {
    return false;
  }
  handler(bytes);
  return true;
}

}  // namespace ipc
}  // namespace rtos
