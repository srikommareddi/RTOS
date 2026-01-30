#include "../include/diagnostics/watchdog.h"

namespace rtos {
namespace diagnostics {

namespace {

int64_t now_ns() {
  auto now = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

}  // namespace

Watchdog::Watchdog() = default;

Watchdog::~Watchdog() {
  stop();
}

void Watchdog::start(std::chrono::milliseconds timeout,
                     TimeoutHandler handler) {
  if (running_.load()) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(config_mutex_);
    timeout_ = timeout;
    handler_ = std::move(handler);
  }
  last_kick_ns_.store(now_ns());
  running_.store(true);
  thread_ = std::thread([this]() { run(); });
}

void Watchdog::stop() {
  if (!running_.exchange(false)) {
    return;
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

void Watchdog::kick() {
  last_kick_ns_.store(now_ns());
}

void Watchdog::run() {
  while (running_.load()) {
    std::chrono::milliseconds timeout_copy;
    TimeoutHandler handler_copy;
    {
      std::lock_guard<std::mutex> lock(config_mutex_);
      timeout_copy = timeout_;
      handler_copy = handler_;
    }
    std::this_thread::sleep_for(timeout_copy / 4);
    int64_t elapsed = now_ns() - last_kick_ns_.load();
    if (elapsed >
        std::chrono::duration_cast<std::chrono::nanoseconds>(timeout_copy)
            .count()) {
      if (handler_copy) {
        handler_copy();
      }
      last_kick_ns_.store(now_ns());
    }
  }
}

}  // namespace diagnostics
}  // namespace rtos
