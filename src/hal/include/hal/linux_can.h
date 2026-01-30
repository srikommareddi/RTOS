#pragma once

#include "can_bus.h"

namespace rtos {
namespace hal {

class LinuxCanBus final : public CanBus {
 public:
  LinuxCanBus();
  ~LinuxCanBus() override;

  bool open(const std::string& device) override;
  void close() override;
  bool write_frame(const CanFrame& frame) override;
  bool read_frame(CanFrame* frame) override;

 private:
  int socket_fd_ = -1;
};

}  // namespace hal
}  // namespace rtos
