#include "../include/robotics/sensor_fusion.h"

namespace rtos {
namespace robotics {

RobotState SensorFusion::update(const ImuSample& imu,
                                const JointState& joints) {
  RobotState state;
  state.imu = imu;
  state.joints = joints;
  return state;
}

}  // namespace robotics
}  // namespace rtos
