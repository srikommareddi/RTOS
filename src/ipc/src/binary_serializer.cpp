#include "../include/ipc/binary_serializer.h"

#include <chrono>
#include <cstring>

namespace rtos {
namespace ipc {

namespace {

constexpr size_t kMaxPayloadBytes = 4 * 1024 * 1024;

uint64_t host_to_be64(uint64_t value) {
  return ((value & 0x00000000000000FFULL) << 56) |
         ((value & 0x000000000000FF00ULL) << 40) |
         ((value & 0x0000000000FF0000ULL) << 24) |
         ((value & 0x00000000FF000000ULL) << 8) |
         ((value & 0x000000FF00000000ULL) >> 8) |
         ((value & 0x0000FF0000000000ULL) >> 24) |
         ((value & 0x00FF000000000000ULL) >> 40) |
         ((value & 0xFF00000000000000ULL) >> 56);
}

uint64_t be64_to_host(uint64_t value) {
  return host_to_be64(value);
}

void append_u16(std::vector<uint8_t>* bytes, uint16_t value) {
  bytes->push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  bytes->push_back(static_cast<uint8_t>(value & 0xFF));
}

void append_u32(std::vector<uint8_t>* bytes, uint32_t value) {
  bytes->push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
  bytes->push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
  bytes->push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  bytes->push_back(static_cast<uint8_t>(value & 0xFF));
}

void append_u64(std::vector<uint8_t>* bytes, uint64_t value) {
  uint64_t be = host_to_be64(value);
  for (int i = 7; i >= 0; --i) {
    bytes->push_back(static_cast<uint8_t>((be >> (i * 8)) & 0xFF));
  }
}

bool read_u16(const std::vector<uint8_t>& bytes, size_t* offset,
              uint16_t* value) {
  if (*offset + 2 > bytes.size()) {
    return false;
  }
  *value = static_cast<uint16_t>((bytes[*offset] << 8) |
                                 bytes[*offset + 1]);
  *offset += 2;
  return true;
}

bool read_u32(const std::vector<uint8_t>& bytes, size_t* offset,
              uint32_t* value) {
  if (*offset + 4 > bytes.size()) {
    return false;
  }
  *value = (static_cast<uint32_t>(bytes[*offset]) << 24) |
           (static_cast<uint32_t>(bytes[*offset + 1]) << 16) |
           (static_cast<uint32_t>(bytes[*offset + 2]) << 8) |
           (static_cast<uint32_t>(bytes[*offset + 3]));
  *offset += 4;
  return true;
}

bool read_u64(const std::vector<uint8_t>& bytes, size_t* offset,
              uint64_t* value) {
  if (*offset + 8 > bytes.size()) {
    return false;
  }
  uint64_t be = 0;
  for (int i = 0; i < 8; ++i) {
    be = (be << 8) | bytes[*offset + i];
  }
  *value = be64_to_host(be);
  *offset += 8;
  return true;
}

}  // namespace

std::vector<uint8_t> BinarySerializer::serialize(
    const IpcMessage& message) const {
  std::vector<uint8_t> bytes;
  if (message.topic.size() > UINT16_MAX ||
      message.payload.size() > kMaxPayloadBytes) {
    return bytes;
  }

  bytes.reserve(2 + message.topic.size() + 1 + 8 + 8 + 1 + 4 +
                message.payload.size());
  append_u16(&bytes, static_cast<uint16_t>(message.topic.size()));
  bytes.insert(bytes.end(), message.topic.begin(), message.topic.end());

  bytes.push_back(static_cast<uint8_t>(message.qos));
  append_u64(&bytes, message.sequence);
  append_u64(&bytes, message.ack_for);
  bytes.push_back(static_cast<uint8_t>(message.is_ack ? 1 : 0));
  append_u32(&bytes, static_cast<uint32_t>(message.payload.size()));
  bytes.insert(bytes.end(), message.payload.begin(), message.payload.end());

  return bytes;
}

bool BinarySerializer::deserialize(const std::vector<uint8_t>& bytes,
                                   IpcMessage* message) const {
  if (!message) {
    return false;
  }

  size_t offset = 0;
  uint16_t topic_len = 0;
  uint32_t payload_len = 0;
  uint64_t sequence = 0;
  uint8_t qos = 0;

  if (!read_u16(bytes, &offset, &topic_len)) {
    return false;
  }
  if (offset + topic_len + 1 + 8 + 8 + 1 + 4 > bytes.size()) {
    return false;
  }

  std::string topic(bytes.begin() + offset, bytes.begin() + offset + topic_len);
  offset += topic_len;

  qos = bytes[offset++];
  if (qos > static_cast<uint8_t>(DeliveryQos::kAtLeastOnce)) {
    return false;
  }

  if (!read_u64(bytes, &offset, &sequence)) {
    return false;
  }
  uint64_t ack_for = 0;
  if (!read_u64(bytes, &offset, &ack_for)) {
    return false;
  }
  uint8_t is_ack = bytes[offset++];
  if (!read_u32(bytes, &offset, &payload_len)) {
    return false;
  }
  if (payload_len > kMaxPayloadBytes || offset + payload_len > bytes.size()) {
    return false;
  }

  std::vector<uint8_t> payload(bytes.begin() + offset,
                               bytes.begin() + offset + payload_len);

  message->topic = std::move(topic);
  message->payload = std::move(payload);
  message->sequence = sequence;
  message->qos = static_cast<DeliveryQos>(qos);
  message->ack_for = ack_for;
  message->is_ack = (is_ack != 0);
  message->timestamp = std::chrono::steady_clock::now();
  return true;
}

}  // namespace ipc
}  // namespace rtos
