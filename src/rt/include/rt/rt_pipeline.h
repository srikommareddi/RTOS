#pragma once

#include "rt_queue.h"
#include "rt_scheduler.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

namespace rtos {
namespace rt {

template <typename In, typename Out>
class RtStage {
 public:
  using Handler = std::function<Out(const In&)>;

  RtStage(const std::string& name, size_t capacity, Handler handler)
      : name_(name), queue_(capacity), handler_(std::move(handler)) {}

  void start(int priority) {
    running_.store(true);
    thread_ = std::thread([this, priority]() {
      RtScheduler::set_thread_name(name_);
      RtScheduler::set_thread_realtime(priority);
      run();
    });
  }

  void stop() {
    running_.store(false);
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  bool push(const In& item) {
    return queue_.try_push(item);
  }

  bool pop(Out* out) {
    return output_.try_pop(out);
  }

 private:
  void run() {
    while (running_.load()) {
      In input;
      if (!queue_.pop_for(&input, std::chrono::milliseconds(10))) {
        continue;
      }
      Out out = handler_(input);
      output_.try_push(std::move(out));
    }
  }

  std::string name_;
  RtQueue<In> queue_;
  RtQueue<Out> output_{64};
  Handler handler_;
  std::atomic<bool> running_{false};
  std::thread thread_;
};

}  // namespace rt
}  // namespace rtos
