#include "../include/ipc/protobuf_serializer.h"

#include "ipc_envelope.pb.h"

#include <chrono>

namespace rtos {
namespace ipc {

namespace {

uint64_t to_nanoseconds(std::chrono::steady_clock::time_point tp) {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          tp.time_since_epoch())
          .count());
}

}  // namespace

std::vector<uint8_t> ProtobufSerializer::serialize(
    const IpcMessage& message) const {
  rtos::ipc::IpcEnvelope envelope;
  envelope.set_topic(message.topic);
  envelope.set_payload(message.payload.data(),
                       static_cast<int>(message.payload.size()));
  envelope.set_sequence(message.sequence);
  envelope.set_qos(static_cast<uint32_t>(message.qos));
  envelope.set_timestamp_ns(to_nanoseconds(message.timestamp));
  envelope.set_ack_for(message.ack_for);
  envelope.set_is_ack(message.is_ack);

  std::string out;
  if (!envelope.SerializeToString(&out)) {
    return {};
  }
  return std::vector<uint8_t>(out.begin(), out.end());
}

bool ProtobufSerializer::deserialize(const std::vector<uint8_t>& bytes,
                                      IpcMessage* message) const {
  if (!message) {
    return false;
  }

  rtos::ipc::IpcEnvelope envelope;
  if (!envelope.ParseFromArray(bytes.data(),
                               static_cast<int>(bytes.size()))) {
    return false;
  }

  message->topic = envelope.topic();
  message->payload.assign(envelope.payload().begin(),
                          envelope.payload().end());
  message->sequence = envelope.sequence();
  message->qos = static_cast<DeliveryQos>(envelope.qos());
  message->ack_for = envelope.ack_for();
  message->is_ack = envelope.is_ack();
  message->timestamp = std::chrono::steady_clock::now();
  return true;
}

}  // namespace ipc
}  // namespace rtos
