#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace rtos {
namespace ipc {

using TransportReceiveHandler = std::function<void(const std::vector<uint8_t>&)>;

class IpcTransport {
 public:
  virtual ~IpcTransport() = default;

  virtual void start(TransportReceiveHandler handler) = 0;
  virtual void stop() = 0;
  virtual bool publish(const std::vector<uint8_t>& bytes) = 0;
};

}  // namespace ipc
}  // namespace rtos
