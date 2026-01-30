#pragma once

#include "i2c_bus.h"

#include <cstdint>

namespace rtos {
namespace hal {

class LinuxI2cBus final : public I2cBus {
 public:
  LinuxI2cBus();
  ~LinuxI2cBus() override;

  bool open(const std::string& device) override;
  void close() override;
  bool write(uint8_t address, const std::vector<uint8_t>& bytes) override;
  bool read(uint8_t address, size_t length,
            std::vector<uint8_t>* out) override;

 private:
  int fd_ = -1;
};

}  // namespace hal
}  // namespace rtos
