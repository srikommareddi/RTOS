#include "../include/diagnostics/health_monitor.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

namespace {

rtos::diagnostics::HealthState parse_state(const std::string& token) {
  if (token == "ok") {
    return rtos::diagnostics::HealthState::kOk;
  }
  if (token == "degraded") {
    return rtos::diagnostics::HealthState::kDegraded;
  }
  return rtos::diagnostics::HealthState::kFault;
}

const char* to_string(rtos::diagnostics::HealthState state) {
  switch (state) {
    case rtos::diagnostics::HealthState::kOk:
      return "ok";
    case rtos::diagnostics::HealthState::kDegraded:
      return "degraded";
    case rtos::diagnostics::HealthState::kFault:
      return "fault";
    default:
      return "unknown";
  }
}

}  // namespace

int main() {
  rtos::diagnostics::HealthMonitor monitor;
  std::string line;

  std::cout << "Diagnostics CLI\n";
  std::cout << "Commands: emit <component> <ok|degraded|fault> <detail>\n";
  std::cout << "          status <component>\n";
  std::cout << "          recent <count>\n";
  std::cout << "          quit\n";

  while (std::cout << "> ", std::getline(std::cin, line)) {
    std::istringstream input(line);
    std::string cmd;
    input >> cmd;
    if (cmd == "quit" || cmd == "exit") {
      break;
    }
    if (cmd == "emit") {
      std::string component;
      std::string state_token;
      std::string detail;
      input >> component >> state_token;
      std::getline(input, detail);
      if (!detail.empty() && detail[0] == ' ') {
        detail.erase(0, 1);
      }
      rtos::diagnostics::HealthEvent event{
          component, parse_state(state_token), detail,
          std::chrono::steady_clock::now()};
      monitor.report_event(std::move(event));
      std::cout << "ok\n";
      continue;
    }
    if (cmd == "status") {
      std::string component;
      input >> component;
      auto state = monitor.current_state(component);
      std::cout << component << ": " << to_string(state) << "\n";
      continue;
    }
    if (cmd == "recent") {
      size_t count = 10;
      input >> count;
      auto events = monitor.recent_events(count);
      for (const auto& event : events) {
        std::cout << event.component << " " << to_string(event.state) << " "
                  << event.detail << "\n";
      }
      continue;
    }
    if (!cmd.empty()) {
      std::cout << "unknown command\n";
    }
  }
  return 0;
}
