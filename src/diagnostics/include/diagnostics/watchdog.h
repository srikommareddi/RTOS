#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

namespace rtos {
namespace diagnostics {

class Watchdog {
 public:
  using TimeoutHandler = std::function<void()>;

  Watchdog();
  ~Watchdog();

  void start(std::chrono::milliseconds timeout, TimeoutHandler handler);
  void stop();
  void kick();

 private:
  void run();

  std::atomic<bool> running_{false};
  std::chrono::milliseconds timeout_{1000};
  TimeoutHandler handler_;
  std::atomic<int64_t> last_kick_ns_{0};
  std::mutex config_mutex_;
  std::thread thread_;
};

}  // namespace diagnostics
}  // namespace rtos
