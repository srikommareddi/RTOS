#include "../include/ipc/ipc_bus.h"

#include <cassert>
#include <string>

int main() {
  rtos::ipc::IpcBus bus;
  bool received = false;

  auto id = bus.subscribe("vision.frame", [&](const rtos::ipc::IpcMessage& msg) {
    received = true;
    assert(msg.topic == "vision.frame");
  });

  rtos::ipc::IpcMessage message;
  message.topic = "vision.frame";
  bus.publish(message);

  assert(id != 0);
  assert(received);
  return 0;
}
