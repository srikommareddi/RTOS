#  Embedded Platform Scaffold

This repository provides a high-level architecture scaffold for a future-forward
robotic surgical platform. It includes a system diagram, IPC module skeleton,
and diagnostics placeholders.
## Structure

- `docs/architecture.mmd`: Mermaid architecture diagram.
- `src/ipc/`: IPC module (publish/subscribe bus, serializer, transports).
- `src/diagnostics/`: Diagnostics and health monitoring placeholders.
- `src/diagnostics/` includes fault manager + watchdog utilities.
- `src/hal/`: HAL driver stubs for CAN/I2C/SPI.
- `src/hal/` also includes Linux `/dev` adapters and a QNX resource manager stub
  with a dispatch loop, connect/io handlers, and devctl support.
- `src/rt/`: RT scheduling wrappers and RT-safe queue.
- `src/rt/` includes a simple RT pipeline stage abstraction.
- `src/security/`: Secure boot and mock KMS scaffolding.
- `src/robotics/`: Control loop and sensor fusion stubs.

Executables:
- `ipc_bus_test`
- `shm_transport_test`
- `diagnostics_cli`
- `hal_polling`
- `rt_pipeline_demo`
- `robot_control_demo`
- `rt_qnx_test` (QNX only)

## Build (optional)

```
cmake -S . -B build
cmake --build build
```

## IPC Usage (example)

Local (in-process):
```
rtos::ipc::IpcBus bus;
```

TCP (multi-process):
```
auto transport = std::make_unique<rtos::ipc::TcpTransport>(
    rtos::ipc::TcpTransportConfig{true, "0.0.0.0", 5500, 8});
auto serializer = std::make_unique<rtos::ipc::BinarySerializer>();
rtos::ipc::IpcBus bus(std::move(transport), std::move(serializer));
```

UNIX domain socket (lower latency):
```
auto transport = std::make_unique<rtos::ipc::UnixTransport>(
    rtos::ipc::UnixTransportConfig{true, "/tmp/rtos_ipc.sock", 8});
auto serializer = std::make_unique<rtos::ipc::BinarySerializer>();
rtos::ipc::IpcBus bus(std::move(transport), std::move(serializer));
```

Protobuf serializer (optional):
```
cmake -S . -B build -DIPC_ENABLE_PROTOBUF=ON
```

Generate protobuf sources:
```
cd src/ipc/proto
protoc -I . --cpp_out=gen ipc_envelope.proto
```

Shared memory IPC (lower latency, same host):
```
auto transport = std::make_unique<rtos::ipc::ShmTransport>(
    rtos::ipc::ShmTransportConfig{"/rtos_ipc_shm", 1 << 20, true, 0, 4});
auto serializer = std::make_unique<rtos::ipc::BinarySerializer>();
rtos::ipc::IpcBus bus(std::move(transport), std::move(serializer));
```

Shared memory fan-out (multi-consumer):
```
rtos::ipc::ShmTransportConfig config;
config.name = "/rtos_ipc_shm";
config.size_bytes = 1 << 20;
config.is_owner = false;
config.consumer_id = 1;
config.max_consumers = 4;
```

Secure boot verification (scaffold):
```
rtos::security::MockCryptoProvider crypto;
rtos::security::MockKmsClient kms;
rtos::security::SecureBootVerifier verifier(&crypto, &kms);
```
