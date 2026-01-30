#pragma once

#include "ipc_transport.h"

#include <mutex>

namespace rtos {
namespace ipc {

class LocalTransport final : public IpcTransport {
 public:
  void start(TransportReceiveHandler handler) override;
  void stop() override;
  bool publish(const std::vector<uint8_t>& bytes) override;

 private:
  std::mutex mutex_;
  TransportReceiveHandler handler_;
};

}  // namespace ipc
}  // namespace rtos
