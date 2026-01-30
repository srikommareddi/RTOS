#include "../include/ipc/ipc_bus.h"

#include "../include/ipc/binary_serializer.h"
#include "../include/ipc/local_transport.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace rtos {
namespace ipc {

IpcBus::IpcBus()
    : IpcBus(std::make_unique<LocalTransport>(),
             std::make_unique<BinarySerializer>()) {}

IpcBus::IpcBus(std::unique_ptr<IpcTransport> transport,
               std::unique_ptr<IpcSerializer> serializer,
               Options options)
    : next_id_(1),
      transport_(std::move(transport)),
      serializer_(std::move(serializer)),
      options_(options) {
  running_.store(true);
  if (transport_) {
    transport_->start([this](const std::vector<uint8_t>& bytes) {
      if (!serializer_) {
        return;
      }
      IpcMessage message;
      if (!serializer_->deserialize(bytes, &message)) {
        return;
      }
      dispatch(std::move(message));
    });
  }
  retry_thread_ = std::thread([this]() { retry_loop(); });
}

IpcBus::~IpcBus() {
  running_.store(false);
  pending_cv_.notify_all();
  if (retry_thread_.joinable()) {
    retry_thread_.join();
  }
  if (transport_) {
    transport_->stop();
  }
}

uint64_t IpcBus::subscribe(const std::string& topic, IpcHandler handler) {
  if (topic.empty() || !handler) {
    return 0;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  uint64_t id = next_id_++;
  subscriptions_.emplace(id, Subscription{topic, std::move(handler)});
  return id;
}

void IpcBus::unsubscribe(uint64_t subscription_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  subscriptions_.erase(subscription_id);
}

void IpcBus::publish(IpcMessage message) {
  if (message.topic.empty()) {
    return;
  }

  if (message.timestamp == std::chrono::steady_clock::time_point{}) {
    message.timestamp = std::chrono::steady_clock::now();
  }
  if (message.sequence == 0) {
    message.sequence = next_sequence_.fetch_add(1);
  }

  if (!serializer_ || !transport_) {
    return;
  }

  if (message.qos == DeliveryQos::kAtLeastOnce && !message.is_ack) {
    PendingMessage pending;
    pending.message = message;
    pending.retries_left = options_.retry_count;
    pending.next_due = std::chrono::steady_clock::now() + options_.retry_interval;
    {
      std::lock_guard<std::mutex> lock(pending_mutex_);
      pending_[message.sequence] = std::move(pending);
    }
    pending_cv_.notify_one();
  }

  {
    std::lock_guard<std::mutex> lock(send_mutex_);
    auto bytes = serializer_->serialize(message);
    if (bytes.empty()) {
      return;
    }
    transport_->publish(bytes);
  }
}

void IpcBus::dispatch(IpcMessage message) {
  if (message.is_ack) {
    handle_ack(message.ack_for);
    return;
  }

  if (message.qos == DeliveryQos::kAtLeastOnce && message.sequence != 0) {
    send_ack(message.sequence);
  }

  std::vector<IpcHandler> handlers;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers.reserve(subscriptions_.size());
    for (const auto& entry : subscriptions_) {
      const auto& sub = entry.second;
      if (sub.topic == message.topic) {
        handlers.push_back(sub.handler);
      }
    }
  }

  for (const auto& handler : handlers) {
    handler(message);
  }
}

void IpcBus::send_ack(uint64_t sequence) {
  if (!serializer_ || !transport_ || sequence == 0) {
    return;
  }
  IpcMessage ack;
  ack.topic = "__ipc_ack";
  ack.is_ack = true;
  ack.ack_for = sequence;
  ack.qos = DeliveryQos::kBestEffort;
  std::lock_guard<std::mutex> lock(send_mutex_);
  auto bytes = serializer_->serialize(ack);
  if (!bytes.empty()) {
    transport_->publish(bytes);
  }
}

void IpcBus::handle_ack(uint64_t sequence) {
  if (sequence == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(pending_mutex_);
  pending_.erase(sequence);
}

void IpcBus::retry_loop() {
  std::unique_lock<std::mutex> lock(pending_mutex_);
  while (running_.load()) {
    if (pending_.empty()) {
      pending_cv_.wait(lock);
      continue;
    }

    auto now = std::chrono::steady_clock::now();
    auto next_due = now + std::chrono::hours(24);
    for (auto it = pending_.begin(); it != pending_.end();) {
      if (it->second.next_due <= now) {
        if (it->second.retries_left == 0) {
          it = pending_.erase(it);
          continue;
        }
        uint64_t sequence = it->first;
        IpcMessage message = it->second.message;
        it->second.retries_left--;
        it->second.next_due = now + options_.retry_interval;
        lock.unlock();
        if (serializer_ && transport_) {
          std::lock_guard<std::mutex> send_lock(send_mutex_);
          auto bytes = serializer_->serialize(message);
          if (!bytes.empty()) {
            transport_->publish(bytes);
          }
        }
        lock.lock();
        it = pending_.find(sequence);
        if (it == pending_.end()) {
          now = std::chrono::steady_clock::now();
          continue;
        }
        now = std::chrono::steady_clock::now();
      }
      if (it != pending_.end()) {
        next_due = std::min(next_due, it->second.next_due);
        ++it;
      }
    }

    pending_cv_.wait_until(lock, next_due);
  }
}

size_t IpcBus::subscriber_count(const std::string& topic) const {
  std::lock_guard<std::mutex> lock(mutex_);
  size_t count = 0;
  for (const auto& entry : subscriptions_) {
    const auto& sub = entry.second;
    if (sub.topic == topic) {
      ++count;
    }
  }
  return count;
}

}  // namespace ipc
}  // namespace rtos
