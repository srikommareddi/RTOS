#include "../include/ipc/shm_transport.h"

#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

namespace rtos {
namespace ipc {

namespace {

constexpr size_t kMaxConsumers = 8;

struct ShmRingBuffer {
  pthread_mutex_t mutex;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
  size_t capacity;
  size_t head;
  size_t tail;
  size_t max_consumers;
  size_t tails[kMaxConsumers];
  uint8_t active[kMaxConsumers];
  uint8_t data[1];
};

constexpr size_t kHeaderSize =
    sizeof(ShmRingBuffer) - sizeof(((ShmRingBuffer*)0)->data);

size_t aligned_size(size_t value) {
  constexpr size_t kAlign = 8;
  return (value + (kAlign - 1)) & ~(kAlign - 1);
}

size_t ring_min_tail(const ShmRingBuffer* ring) {
  size_t min_tail = ring->head;
  bool found = false;
  for (size_t i = 0; i < ring->max_consumers; ++i) {
    if (ring->active[i]) {
      min_tail = found ? std::min(min_tail, ring->tails[i]) : ring->tails[i];
      found = true;
    }
  }
  return found ? min_tail : ring->head;
}

bool ring_has_space(const ShmRingBuffer* ring, size_t needed) {
  size_t head = ring->head;
  size_t tail = ring_min_tail(ring);
  size_t capacity = ring->capacity;
  if (head >= tail) {
    return (capacity - (head - tail) - 1) >= needed;
  }
  return (tail - head - 1) >= needed;
}

size_t ring_size(const ShmRingBuffer* ring, size_t tail) {
  size_t head = ring->head;
  size_t capacity = ring->capacity;
  if (head >= tail) {
    return head - tail;
  }
  return capacity - (tail - head);
}

void ring_write(ShmRingBuffer* ring, const uint8_t* data, size_t length) {
  size_t capacity = ring->capacity;
  size_t head = ring->head;
  size_t first = std::min(length, capacity - head);
  std::memcpy(ring->data + head, data, first);
  if (length > first) {
    std::memcpy(ring->data, data + first, length - first);
  }
  ring->head = (head + length) % capacity;
}

void ring_read(ShmRingBuffer* ring, size_t* tail, uint8_t* data, size_t length) {
  size_t capacity = ring->capacity;
  size_t local_tail = *tail;
  size_t first = std::min(length, capacity - local_tail);
  std::memcpy(data, ring->data + local_tail, first);
  if (length > first) {
    std::memcpy(data + first, ring->data, length - first);
  }
  *tail = (local_tail + length) % capacity;
}

void init_ring(ShmRingBuffer* ring, size_t capacity, size_t max_consumers) {
  pthread_mutexattr_t mutex_attr;
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&ring->mutex, &mutex_attr);
  pthread_mutexattr_destroy(&mutex_attr);

  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&ring->not_empty, &cond_attr);
  pthread_cond_init(&ring->not_full, &cond_attr);
  pthread_condattr_destroy(&cond_attr);

  ring->capacity = capacity;
  ring->head = 0;
  ring->tail = 0;
  ring->max_consumers = std::min(max_consumers, kMaxConsumers);
  for (size_t i = 0; i < ring->max_consumers; ++i) {
    ring->tails[i] = 0;
    ring->active[i] = 0;
  }
}

}  // namespace

ShmTransport::ShmTransport(ShmTransportConfig config)
    : config_(std::move(config)),
      consumer_id_(std::min(config_.consumer_id, kMaxConsumers - 1)) {}

ShmTransport::~ShmTransport() {
  stop();
}

void ShmTransport::start(TransportReceiveHandler handler) {
  handler_ = std::move(handler);
  running_.store(true);

  int flags = O_RDWR;
  if (config_.is_owner) {
    flags |= O_CREAT;
  }
  shm_fd_ = ::shm_open(config_.name.c_str(), flags, 0666);
  if (shm_fd_ < 0) {
    running_.store(false);
    return;
  }

  size_t total_size = aligned_size(kHeaderSize + config_.size_bytes);
  if (config_.is_owner) {
    if (::ftruncate(shm_fd_, static_cast<off_t>(total_size)) != 0) {
      stop();
      return;
    }
  }

  shm_ptr_ = ::mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    shm_fd_, 0);
  if (shm_ptr_ == MAP_FAILED) {
    shm_ptr_ = nullptr;
    stop();
    return;
  }

  auto* ring = reinterpret_cast<ShmRingBuffer*>(shm_ptr_);
  if (config_.is_owner) {
    init_ring(ring, config_.size_bytes, config_.max_consumers);
  }

  {
    pthread_mutex_lock(&ring->mutex);
    if (consumer_id_ < ring->max_consumers) {
      ring->active[consumer_id_] = 1;
      ring->tails[consumer_id_] = ring->head;
    }
    pthread_mutex_unlock(&ring->mutex);
  }

  receiver_thread_ = std::thread([this]() { receive_loop(); });
}

void ShmTransport::stop() {
  if (!running_.exchange(false)) {
    return;
  }

  if (shm_ptr_) {
    auto* ring = reinterpret_cast<ShmRingBuffer*>(shm_ptr_);
    pthread_mutex_lock(&ring->mutex);
    if (consumer_id_ < ring->max_consumers) {
      ring->active[consumer_id_] = 0;
      ring->tails[consumer_id_] = ring->head;
    }
    pthread_cond_broadcast(&ring->not_empty);
    pthread_cond_broadcast(&ring->not_full);
    pthread_mutex_unlock(&ring->mutex);
  }

  if (receiver_thread_.joinable()) {
    receiver_thread_.join();
  }

  if (shm_ptr_) {
    size_t total_size = aligned_size(kHeaderSize + config_.size_bytes);
    ::munmap(shm_ptr_, total_size);
    shm_ptr_ = nullptr;
  }
  if (shm_fd_ >= 0) {
    ::close(shm_fd_);
    shm_fd_ = -1;
  }
  if (config_.is_owner) {
    ::shm_unlink(config_.name.c_str());
  }
}

bool ShmTransport::publish(const std::vector<uint8_t>& bytes) {
  if (!running_.load()) {
    return false;
  }
  return write_frame(bytes);
}

void ShmTransport::receive_loop() {
  while (running_.load()) {
    std::vector<uint8_t> frame;
    if (!read_frame(&frame)) {
      continue;
    }
    if (handler_) {
      handler_(frame);
    }
  }
}

bool ShmTransport::write_frame(const std::vector<uint8_t>& bytes) {
  if (!shm_ptr_) {
    return false;
  }

  auto* ring = reinterpret_cast<ShmRingBuffer*>(shm_ptr_);
  uint32_t length = static_cast<uint32_t>(bytes.size());
  size_t needed = aligned_size(sizeof(length) + length);

  pthread_mutex_lock(&ring->mutex);
  if (!ring_has_space(ring, needed)) {
    pthread_cond_signal(&ring->not_full);
    pthread_mutex_unlock(&ring->mutex);
    return false;
  }

  ring_write(ring, reinterpret_cast<uint8_t*>(&length), sizeof(length));
  ring_write(ring, bytes.data(), length);
  pthread_cond_signal(&ring->not_empty);
  pthread_mutex_unlock(&ring->mutex);
  return true;
}

bool ShmTransport::read_frame(std::vector<uint8_t>* bytes) {
  if (!shm_ptr_ || !bytes) {
    return false;
  }

  auto* ring = reinterpret_cast<ShmRingBuffer*>(shm_ptr_);
  pthread_mutex_lock(&ring->mutex);
  if (consumer_id_ >= ring->max_consumers || !ring->active[consumer_id_]) {
    pthread_mutex_unlock(&ring->mutex);
    return false;
  }

  size_t tail = ring->tails[consumer_id_];
  while (running_.load() && ring_size(ring, tail) < sizeof(uint32_t)) {
    pthread_cond_wait(&ring->not_empty, &ring->mutex);
    tail = ring->tails[consumer_id_];
  }
  if (!running_.load()) {
    pthread_mutex_unlock(&ring->mutex);
    return false;
  }

  uint32_t length = 0;
  ring_read(ring, &tail, reinterpret_cast<uint8_t*>(&length), sizeof(length));
  if (length == 0 || length > ring->capacity) {
    pthread_mutex_unlock(&ring->mutex);
    return false;
  }

  while (ring_size(ring, tail) < length) {
    pthread_cond_wait(&ring->not_empty, &ring->mutex);
  }

  bytes->resize(length);
  ring_read(ring, &tail, bytes->data(), length);
  ring->tails[consumer_id_] = tail;
  pthread_cond_signal(&ring->not_full);
  pthread_mutex_unlock(&ring->mutex);
  return true;
}

}  // namespace ipc
}  // namespace rtos
