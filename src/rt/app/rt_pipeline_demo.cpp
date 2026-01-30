#include "../include/rt/rt_pipeline.h"
#include "../../hal/include/hal/i2c_bus.h"

#include <chrono>
#include <iostream>
#include <thread>

struct RawSample {
  std::vector<uint8_t> bytes;
};

struct FilteredSample {
  double average = 0.0;
};

int main() {
  rtos::hal::DummyI2cBus i2c;
  i2c.open("/dev/i2c-0");

  rtos::rt::RtStage<RawSample, FilteredSample> filter_stage(
      "filter", 32, [](const RawSample& sample) {
        FilteredSample out;
        if (!sample.bytes.empty()) {
          double sum = 0.0;
          for (uint8_t value : sample.bytes) {
            sum += value;
          }
          out.average = sum / sample.bytes.size();
        }
        return out;
      });

  filter_stage.start(20);

  for (int i = 0; i < 20; ++i) {
    std::vector<uint8_t> data;
    i2c.read(0x40, 4, &data);
    filter_stage.push(RawSample{data});

    FilteredSample out;
    if (filter_stage.pop(&out)) {
      std::cout << "avg=" << out.average << "\n";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  filter_stage.stop();
  i2c.close();
  return 0;
}
