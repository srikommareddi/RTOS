#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>

namespace rtos {
namespace rt {

template <typename T>
class RtQueue {
 public:
  explicit RtQueue(size_t capacity) : capacity_(capacity) {}

  bool try_push(T item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= capacity_) {
      return false;
    }
    queue_.push_back(std::move(item));
    not_empty_.notify_one();
    return true;
  }

  bool push_for(T item, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!not_full_.wait_for(lock, timeout,
                            [this]() { return queue_.size() < capacity_; })) {
      return false;
    }
    queue_.push_back(std::move(item));
    not_empty_.notify_one();
    return true;
  }

  bool try_pop(T* out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty() || !out) {
      return false;
    }
    *out = std::move(queue_.front());
    queue_.pop_front();
    not_full_.notify_one();
    return true;
  }

  bool pop_for(T* out, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!out) {
      return false;
    }
    if (!not_empty_.wait_for(lock, timeout,
                             [this]() { return !queue_.empty(); })) {
      return false;
    }
    *out = std::move(queue_.front());
    queue_.pop_front();
    not_full_.notify_one();
    return true;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

 private:
  size_t capacity_;
  mutable std::mutex mutex_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;
  std::deque<T> queue_;
};

}  // namespace rt
}  // namespace rtos
