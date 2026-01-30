#include "../include/ipc/unix_transport.h"

#include <algorithm>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>

namespace rtos {
namespace ipc {

namespace {

constexpr int kBacklog = 8;

bool send_all(int socket_fd, const uint8_t* data, size_t length) {
  size_t sent = 0;
  while (sent < length) {
    ssize_t result = ::send(socket_fd, data + sent, length - sent, 0);
    if (result <= 0) {
      return false;
    }
    sent += static_cast<size_t>(result);
  }
  return true;
}

bool recv_all(int socket_fd, uint8_t* data, size_t length) {
  size_t received = 0;
  while (received < length) {
    ssize_t result = ::recv(socket_fd, data + received, length - received, 0);
    if (result <= 0) {
      return false;
    }
    received += static_cast<size_t>(result);
  }
  return true;
}

sockaddr_un make_address(const std::string& path) {
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
  return addr;
}

}  // namespace

UnixTransport::UnixTransport(UnixTransportConfig config) : config_(std::move(config)) {}

UnixTransport::~UnixTransport() {
  stop();
}

void UnixTransport::start(TransportReceiveHandler handler) {
  handler_ = std::move(handler);
  running_.store(true);

  if (config_.is_server) {
    listen_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
      running_.store(false);
      return;
    }

    ::unlink(config_.path.c_str());
    sockaddr_un addr = make_address(config_.path);
    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
      close_socket(listen_fd_);
      running_.store(false);
      return;
    }

    if (::listen(listen_fd_, kBacklog) < 0) {
      close_socket(listen_fd_);
      running_.store(false);
      return;
    }

    accept_thread_ = std::thread([this]() { run_accept_loop(); });
    return;
  }

  server_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    running_.store(false);
    return;
  }

  sockaddr_un addr = make_address(config_.path);
  if (::connect(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    close_socket(server_fd_);
    running_.store(false);
    return;
  }

  client_threads_.emplace_back([this]() { run_receive_loop(server_fd_); });
}

void UnixTransport::stop() {
  if (!running_.exchange(false)) {
    return;
  }

  close_socket(listen_fd_);
  close_socket(server_fd_);

  {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    for (int& fd : client_fds_) {
      close_socket(fd);
    }
  }

  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }

  for (auto& thread : client_threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  client_threads_.clear();

  if (config_.is_server) {
    ::unlink(config_.path.c_str());
  }
}

bool UnixTransport::publish(const std::vector<uint8_t>& bytes) {
  if (!running_.load()) {
    return false;
  }

  if (config_.is_server) {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    bool ok = true;
    for (int fd : client_fds_) {
      ok = send_frame(fd, bytes) && ok;
    }
    return ok;
  }

  if (server_fd_ < 0) {
    return false;
  }
  return send_frame(server_fd_, bytes);
}

void UnixTransport::run_accept_loop() {
  while (running_.load()) {
    sockaddr_un client_addr{};
    socklen_t len = sizeof(client_addr);
    int client_fd =
        ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
    if (client_fd < 0) {
      continue;
    }

    {
      std::lock_guard<std::mutex> lock(socket_mutex_);
      if (client_fds_.size() >= config_.max_clients) {
        ::close(client_fd);
        continue;
      }
      client_fds_.push_back(client_fd);
    }

    client_threads_.emplace_back([this, client_fd]() {
      run_receive_loop(client_fd);
    });
  }
}

void UnixTransport::run_receive_loop(int socket_fd) {
  while (running_.load()) {
    std::vector<uint8_t> frame;
    if (!recv_frame(socket_fd, &frame)) {
      break;
    }
    if (handler_) {
      handler_(frame);
    }
  }
  if (config_.is_server) {
    remove_client_fd(socket_fd);
  }
  close_socket(socket_fd);
}

void UnixTransport::remove_client_fd(int socket_fd) {
  std::lock_guard<std::mutex> lock(socket_mutex_);
  auto it = std::remove(client_fds_.begin(), client_fds_.end(), socket_fd);
  client_fds_.erase(it, client_fds_.end());
}

void UnixTransport::close_socket(int& socket_fd) {
  if (socket_fd >= 0) {
    ::close(socket_fd);
    socket_fd = -1;
  }
}

bool UnixTransport::send_frame(int socket_fd,
                               const std::vector<uint8_t>& bytes) {
  uint32_t length = htonl(static_cast<uint32_t>(bytes.size()));
  if (!send_all(socket_fd, reinterpret_cast<uint8_t*>(&length), sizeof(length))) {
    return false;
  }
  return send_all(socket_fd, bytes.data(), bytes.size());
}

bool UnixTransport::recv_frame(int socket_fd, std::vector<uint8_t>* bytes) {
  uint32_t length = 0;
  if (!recv_all(socket_fd, reinterpret_cast<uint8_t*>(&length), sizeof(length))) {
    return false;
  }
  length = ntohl(length);
  if (length == 0) {
    return false;
  }
  bytes->resize(length);
  return recv_all(socket_fd, bytes->data(), length);
}

}  // namespace ipc
}  // namespace rtos
