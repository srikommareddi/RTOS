cmake_minimum_required(VERSION 3.20)
project(future_forward_platform LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ipc
  src/ipc/src/binary_serializer.cpp
  src/ipc/src/ipc_bus.cpp
  src/ipc/src/local_transport.cpp
  src/ipc/src/shm_transport.cpp
  src/ipc/src/tcp_transport.cpp
  src/ipc/src/unix_transport.cpp
)
target_include_directories(ipc PUBLIC
  src/ipc/include
)
find_package(Threads REQUIRED)
target_link_libraries(ipc PUBLIC Threads::Threads)

option(IPC_ENABLE_PROTOBUF "Enable protobuf serializer" OFF)
if (IPC_ENABLE_PROTOBUF)
  find_package(Protobuf REQUIRED)
  target_sources(ipc PRIVATE src/ipc/src/protobuf_serializer.cpp)
  target_include_directories(ipc PRIVATE src/ipc/proto/gen)
  target_link_libraries(ipc PRIVATE protobuf::libprotobuf)
endif()

add_library(diagnostics
  src/diagnostics/src/health_monitor.cpp
  src/diagnostics/src/fault_manager.cpp
  src/diagnostics/src/watchdog.cpp
)
target_include_directories(diagnostics PUBLIC
  src/diagnostics/include
)

add_library(hal
  src/hal/src/can_bus.cpp
  src/hal/src/i2c_bus.cpp
  src/hal/src/spi_bus.cpp
)
target_include_directories(hal PUBLIC
  src/hal/include
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(hal PRIVATE
    src/hal/src/linux_i2c.cpp
    src/hal/src/linux_spi.cpp
    src/hal/src/linux_can.cpp
  )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "QNX")
  target_sources(hal PRIVATE
    src/hal/src/qnx_resource_manager.cpp
  )
endif()

add_library(rt
  src/rt/src/rt_scheduler.cpp
)
target_include_directories(rt PUBLIC
  src/rt/include
)

add_library(security
  src/security/src/kms_client.cpp
  src/security/src/mock_crypto_provider.cpp
  src/security/src/secure_boot.cpp
)
target_include_directories(security PUBLIC
  src/security/include
)

add_library(robotics
  src/robotics/src/control_loop.cpp
  src/robotics/src/sensor_fusion.cpp
)
target_include_directories(robotics PUBLIC
  src/robotics/include
)

add_executable(ipc_bus_test
  src/ipc/test/ipc_bus_test.cpp
)
target_link_libraries(ipc_bus_test PRIVATE ipc)

add_executable(shm_transport_test
  src/ipc/test/shm_transport_test.cpp
)
target_link_libraries(shm_transport_test PRIVATE ipc)

add_executable(diagnostics_cli
  src/diagnostics/app/diagnostics_cli.cpp
)
target_link_libraries(diagnostics_cli PRIVATE diagnostics)

add_executable(hal_polling
  src/hal/app/hal_polling.cpp
)
target_link_libraries(hal_polling PRIVATE hal rt)

add_executable(robot_control_demo
  src/robotics/app/robot_control_demo.cpp
)
target_link_libraries(robot_control_demo PRIVATE robotics rt)

add_executable(rt_pipeline_demo
  src/rt/app/rt_pipeline_demo.cpp
)
target_link_libraries(rt_pipeline_demo PRIVATE rt hal)

if (CMAKE_SYSTEM_NAME STREQUAL "QNX")
  add_executable(rt_qnx_test
    src/rt/test/rt_qnx_test.cpp
  )
  target_link_libraries(rt_qnx_test PRIVATE rt)
endif()
