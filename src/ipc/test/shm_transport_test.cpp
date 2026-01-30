#include "../include/ipc/binary_serializer.h"
#include "../include/ipc/ipc_bus.h"
#include "../include/ipc/shm_transport.h"

#include <cassert>
#include <chrono>
#include <thread>

int main() {
  auto transport = std::make_unique<rtos::ipc::ShmTransport>(
      rtos::ipc::ShmTransportConfig{"/rtos_ipc_test_shm", 1 << 16, true});
  auto serializer = std::make_unique<rtos::ipc::BinarySerializer>();
  rtos::ipc::IpcBus bus(std::move(transport), std::move(serializer));

  bool received = false;
  bus.subscribe("shm.test", [&](const rtos::ipc::IpcMessage&) {
    received = true;
  });

  rtos::ipc::IpcMessage message;
  message.topic = "shm.test";
  message.payload = {0x01, 0x02};
  bus.publish(message);

  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (!received && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  assert(received);
  return 0;
}
