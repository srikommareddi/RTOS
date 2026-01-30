#pragma once

#include "ipc_serializer.h"

namespace rtos {
namespace ipc {

class BinarySerializer final : public IpcSerializer {
 public:
  std::vector<uint8_t> serialize(const IpcMessage& message) const override;
  bool deserialize(const std::vector<uint8_t>& bytes,
                   IpcMessage* message) const override;
};

}  // namespace ipc
}  // namespace rtos
