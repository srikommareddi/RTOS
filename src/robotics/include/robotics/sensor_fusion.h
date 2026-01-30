#pragma once

#include <array>

namespace rtos {
namespace robotics {

struct ImuSample {
  std::array<float, 3> accel{};
  std::array<float, 3> gyro{};
};

struct JointState {
  std::array<float, 6> position{};
  std::array<float, 6> velocity{};
};

struct RobotState {
  JointState joints;
  ImuSample imu;
};

class SensorFusion {
 public:
  RobotState update(const ImuSample& imu, const JointState& joints);
};

}  // namespace robotics
}  // namespace rtos
