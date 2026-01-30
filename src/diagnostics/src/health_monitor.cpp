#include "../include/diagnostics/health_monitor.h"

#include <chrono>
#include <utility>

namespace rtos {
namespace diagnostics {

void HealthMonitor::report_event(HealthEvent event) {
  if (event.timestamp == std::chrono::steady_clock::time_point{}) {
    event.timestamp = std::chrono::steady_clock::now();
  }

  std::lock_guard<std::mutex> lock(mutex_);
  states_[event.component] = event.state;
  events_.push_back(std::move(event));

  if (events_.size() > 1000) {
    events_.erase(events_.begin(), events_.begin() + 200);
  }
}

std::vector<HealthEvent> HealthMonitor::recent_events(size_t max_count) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (max_count == 0 || events_.empty()) {
    return {};
  }

  size_t start = events_.size() > max_count ? events_.size() - max_count : 0;
  return std::vector<HealthEvent>(events_.begin() + start, events_.end());
}

HealthState HealthMonitor::current_state(const std::string& component) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = states_.find(component);
  if (it == states_.end()) {
    return HealthState::kOk;
  }
  return it->second;
}

}  // namespace diagnostics
}  // namespace rtos
