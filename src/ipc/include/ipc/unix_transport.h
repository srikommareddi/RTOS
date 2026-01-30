#pragma once

#include "ipc_transport.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace rtos {
namespace ipc {

struct UnixTransportConfig {
  bool is_server = false;
  std::string path = "/tmp/rtos_ipc.sock";
  size_t max_clients = 8;
};

class UnixTransport final : public IpcTransport {
 public:
  explicit UnixTransport(UnixTransportConfig config);
  ~UnixTransport() override;

  void start(TransportReceiveHandler handler) override;
  void stop() override;
  bool publish(const std::vector<uint8_t>& bytes) override;

 private:
  void run_accept_loop();
  void run_receive_loop(int socket_fd);
  void remove_client_fd(int socket_fd);
  void close_socket(int& socket_fd);
  bool send_frame(int socket_fd, const std::vector<uint8_t>& bytes);
  bool recv_frame(int socket_fd, std::vector<uint8_t>* bytes);

  UnixTransportConfig config_;
  std::atomic<bool> running_{false};
  TransportReceiveHandler handler_;

  std::mutex socket_mutex_;
  int listen_fd_ = -1;
  int server_fd_ = -1;
  std::vector<int> client_fds_;
  std::vector<std::thread> client_threads_;
  std::thread accept_thread_;
};

}  // namespace ipc
}  // namespace rtos
