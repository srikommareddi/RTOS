#include "../include/hal/spi_bus.h"

namespace rtos {
namespace hal {

bool DummySpiBus::open(const std::string& device) {
  device_ = device;
  open_ = true;
  return true;
}

void DummySpiBus::close() {
  open_ = false;
}

bool DummySpiBus::transfer(const std::vector<uint8_t>& tx,
                           std::vector<uint8_t>* rx) {
  if (!open_ || !rx) {
    return false;
  }
  *rx = tx;
  return true;
}

}  // namespace hal
}  // namespace rtos
