#pragma once

#include "spi_bus.h"

namespace rtos {
namespace hal {

class LinuxSpiBus final : public SpiBus {
 public:
  LinuxSpiBus();
  ~LinuxSpiBus() override;

  bool open(const std::string& device) override;
  void close() override;
  bool transfer(const std::vector<uint8_t>& tx,
                std::vector<uint8_t>* rx) override;

  void set_mode(uint8_t mode);
  void set_speed_hz(uint32_t speed_hz);

 private:
  int fd_ = -1;
  uint8_t mode_ = 0;
  uint32_t speed_hz_ = 1000000;
};

}  // namespace hal
}  // namespace rtos
