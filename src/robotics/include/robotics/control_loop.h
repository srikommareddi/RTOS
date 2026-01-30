#pragma once

#include "sensor_fusion.h"

#include <array>

namespace rtos {
namespace robotics {

struct ControlCommand {
  std::array<float, 6> torque{};
};

class ControlLoop {
 public:
  ControlCommand step(const RobotState& state);
};

}  // namespace robotics
}  // namespace rtos
