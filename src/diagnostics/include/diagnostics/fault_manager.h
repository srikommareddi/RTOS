#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace rtos {
namespace diagnostics {

enum class FaultSeverity { kInfo, kWarning, kCritical };

struct FaultRecord {
  std::string code;
  FaultSeverity severity;
  std::string description;
  std::chrono::steady_clock::time_point timestamp;
};

class FaultManager {
 public:
  void raise_fault(FaultRecord record);
  void clear_fault(const std::string& code);
  std::vector<FaultRecord> active_faults() const;
  bool has_critical() const;

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, FaultRecord> active_;
};

}  // namespace diagnostics
}  // namespace rtos
