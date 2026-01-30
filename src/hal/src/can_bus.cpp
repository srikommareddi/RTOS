#include "../include/hal/can_bus.h"

namespace rtos {
namespace hal {

bool DummyCanBus::open(const std::string& device) {
  device_ = device;
  open_ = true;
  return true;
}

void DummyCanBus::close() {
  open_ = false;
}

bool DummyCanBus::write_frame(const CanFrame& frame) {
  if (!open_) {
    return false;
  }
  return !frame.data.empty();
}

bool DummyCanBus::read_frame(CanFrame* frame) {
  if (!open_ || !frame) {
    return false;
  }
  frame->id = 0x100;
  frame->data = {0x00};
  return true;
}

}  // namespace hal
}  // namespace rtos
