#pragma once

#include "ipc_message.h"
#include "ipc_serializer.h"
#include "ipc_transport.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace rtos {
namespace ipc {

using IpcHandler = std::function<void(const IpcMessage&)>;

class IpcBus {
 public:
  struct Options {
    size_t retry_count;
    std::chrono::milliseconds retry_interval;

    Options(size_t retries = 3,
            std::chrono::milliseconds interval =
                std::chrono::milliseconds(50))
        : retry_count(retries), retry_interval(interval) {}
  };

  IpcBus();
  IpcBus(std::unique_ptr<IpcTransport> transport,
         std::unique_ptr<IpcSerializer> serializer,
         Options options = Options{});
  ~IpcBus();

  uint64_t subscribe(const std::string& topic, IpcHandler handler);
  void unsubscribe(uint64_t subscription_id);
  void publish(IpcMessage message);

  size_t subscriber_count(const std::string& topic) const;

 private:
  void dispatch(IpcMessage message);
  void send_ack(uint64_t sequence);
  void handle_ack(uint64_t sequence);
  void retry_loop();

  struct Subscription {
    std::string topic;
    IpcHandler handler;
  };

  struct PendingMessage {
    IpcMessage message;
    size_t retries_left = 0;
    std::chrono::steady_clock::time_point next_due;
  };

  mutable std::mutex mutex_;
  std::atomic<uint64_t> next_sequence_{1};
  uint64_t next_id_;
  std::unordered_map<uint64_t, Subscription> subscriptions_;

  std::unique_ptr<IpcTransport> transport_;
  std::unique_ptr<IpcSerializer> serializer_;
  Options options_;
  std::mutex send_mutex_;

  std::mutex pending_mutex_;
  std::condition_variable pending_cv_;
  std::unordered_map<uint64_t, PendingMessage> pending_;
  std::thread retry_thread_;
  std::atomic<bool> running_{false};
};

}  // namespace ipc
}  // namespace rtos
