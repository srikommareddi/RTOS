#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace rtos {
namespace ipc {

enum class DeliveryQos : uint8_t { kBestEffort = 0, kAtLeastOnce = 1 };

struct IpcMessage {
  std::string topic;
  std::vector<uint8_t> payload;
  std::chrono::steady_clock::time_point timestamp;
  uint64_t sequence = 0;
  DeliveryQos qos = DeliveryQos::kBestEffort;
  uint64_t ack_for = 0;
  bool is_ack = false;
};

}  // namespace ipc
}  // namespace rtos
