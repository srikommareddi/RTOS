#include "../include/hal/can_bus.h"
#include "../include/hal/i2c_bus.h"
#include "../include/hal/spi_bus.h"
#include "../../rt/include/rt/rt_queue.h"
#include "../../rt/include/rt/rt_scheduler.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace {

struct Sample {
  std::string name;
  std::vector<uint8_t> bytes;
};

}  // namespace

int main() {
  rtos::hal::DummyCanBus can;
  rtos::hal::DummyI2cBus i2c;
  rtos::hal::DummySpiBus spi;
  rtos::rt::RtQueue<Sample> queue(64);

  can.open("can0");
  i2c.open("/dev/i2c-0");
  spi.open("/dev/spidev0.0");

  rtos::rt::RtScheduler::set_thread_name("hal-poll");
  rtos::rt::RtScheduler::set_thread_realtime(20);

  for (int i = 0; i < 10; ++i) {
    rtos::hal::CanFrame frame;
    if (can.read_frame(&frame)) {
      queue.try_push({"can", frame.data});
    }

    std::vector<uint8_t> i2c_data;
    if (i2c.read(0x42, 4, &i2c_data)) {
      queue.try_push({"i2c", i2c_data});
    }

    std::vector<uint8_t> spi_rx;
    if (spi.transfer({0xAA, 0x55}, &spi_rx)) {
      queue.try_push({"spi", spi_rx});
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  Sample sample;
  while (queue.try_pop(&sample)) {
    std::cout << sample.name << " bytes=" << sample.bytes.size() << "\n";
  }

  can.close();
  i2c.close();
  spi.close();
  return 0;
}
