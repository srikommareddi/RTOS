#include "../include/hal/i2c_bus.h"

namespace rtos {
namespace hal {

bool DummyI2cBus::open(const std::string& device) {
  device_ = device;
  open_ = true;
  return true;
}

void DummyI2cBus::close() {
  open_ = false;
}

bool DummyI2cBus::write(uint8_t address, const std::vector<uint8_t>& bytes) {
  if (!open_ || address == 0) {
    return false;
  }
  return !bytes.empty();
}

bool DummyI2cBus::read(uint8_t address, size_t length,
                       std::vector<uint8_t>* out) {
  if (!open_ || !out || address == 0) {
    return false;
  }
  out->assign(length, 0x00);
  return true;
}

}  // namespace hal
}  // namespace rtos
