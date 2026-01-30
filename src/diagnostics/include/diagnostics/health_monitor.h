#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace rtos {
namespace diagnostics {

enum class HealthState { kOk, kDegraded, kFault };

struct HealthEvent {
  std::string component;
  HealthState state;
  std::string detail;
  std::chrono::steady_clock::time_point timestamp;
};

class HealthMonitor {
 public:
  void report_event(HealthEvent event);
  std::vector<HealthEvent> recent_events(size_t max_count) const;
  HealthState current_state(const std::string& component) const;

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, HealthState> states_;
  std::vector<HealthEvent> events_;
};

}  // namespace diagnostics
}  // namespace rtos
