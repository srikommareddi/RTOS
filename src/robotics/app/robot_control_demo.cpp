#include "../include/robotics/control_loop.h"
#include "../include/robotics/sensor_fusion.h"
#include "../../rt/include/rt/rt_scheduler.h"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
  rtos::robotics::SensorFusion fusion;
  rtos::robotics::ControlLoop controller;

  rtos::rt::RtScheduler::set_thread_name("robot-ctrl");
  rtos::rt::RtScheduler::set_thread_realtime(30);

  for (int i = 0; i < 5; ++i) {
    rtos::robotics::ImuSample imu{};
    rtos::robotics::JointState joints{};
    joints.position = {0.1f * i, 0.05f * i, 0.0f, 0.02f * i, 0.0f, 0.0f};

    auto state = fusion.update(imu, joints);
    auto cmd = controller.step(state);
    std::cout << "cmd torque[0]=" << cmd.torque[0] << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return 0;
}
