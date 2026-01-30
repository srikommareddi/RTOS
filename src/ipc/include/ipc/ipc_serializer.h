#pragma once

#include "ipc_message.h"

#include <cstdint>
#include <vector>

namespace rtos {
namespace ipc {

class IpcSerializer {
 public:
  virtual ~IpcSerializer() = default;

  virtual std::vector<uint8_t> serialize(const IpcMessage& message) const = 0;
  virtual bool deserialize(const std::vector<uint8_t>& bytes,
                           IpcMessage* message) const = 0;
};

}  // namespace ipc
}  // namespace rtos
