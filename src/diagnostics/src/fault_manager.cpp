#include "../include/diagnostics/fault_manager.h"

namespace rtos {
namespace diagnostics {

void FaultManager::raise_fault(FaultRecord record) {
  if (record.timestamp == std::chrono::steady_clock::time_point{}) {
    record.timestamp = std::chrono::steady_clock::now();
  }
  std::lock_guard<std::mutex> lock(mutex_);
  active_[record.code] = std::move(record);
}

void FaultManager::clear_fault(const std::string& code) {
  std::lock_guard<std::mutex> lock(mutex_);
  active_.erase(code);
}

std::vector<FaultRecord> FaultManager::active_faults() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<FaultRecord> faults;
  faults.reserve(active_.size());
  for (const auto& entry : active_) {
    faults.push_back(entry.second);
  }
  return faults;
}

bool FaultManager::has_critical() const {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& entry : active_) {
    if (entry.second.severity == FaultSeverity::kCritical) {
      return true;
    }
  }
  return false;
}

}  // namespace diagnostics
}  // namespace rtos
