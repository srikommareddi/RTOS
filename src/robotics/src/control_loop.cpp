#include "../include/robotics/control_loop.h"

namespace rtos {
namespace robotics {

ControlCommand ControlLoop::step(const RobotState& state) {
  ControlCommand cmd;
  for (size_t i = 0; i < cmd.torque.size(); ++i) {
    cmd.torque[i] = -0.1f * state.joints.position[i];
  }
  return cmd;
}

}  // namespace robotics
}  // namespace rtos
