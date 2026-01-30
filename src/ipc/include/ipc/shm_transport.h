#pragma once

#include "ipc_transport.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace rtos {
namespace ipc {

struct ShmTransportConfig {
  std::string name = "/rtos_ipc_shm";
  size_t size_bytes = 1 << 20;
  bool is_owner = false;
  size_t consumer_id = 0;
  size_t max_consumers = 4;
};

class ShmTransport final : public IpcTransport {
 public:
  explicit ShmTransport(ShmTransportConfig config);
  ~ShmTransport() override;

  void start(TransportReceiveHandler handler) override;
  void stop() override;
  bool publish(const std::vector<uint8_t>& bytes) override;

 private:
  void receive_loop();
  bool write_frame(const std::vector<uint8_t>& bytes);
  bool read_frame(std::vector<uint8_t>* bytes);

  ShmTransportConfig config_;
  std::atomic<bool> running_{false};
  TransportReceiveHandler handler_;
  std::thread receiver_thread_;

  int shm_fd_ = -1;
  void* shm_ptr_ = nullptr;
  size_t consumer_id_ = 0;
};

}  // namespace ipc
}  // namespace rtos
