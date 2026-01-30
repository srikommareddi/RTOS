#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rtos {
namespace hal {

struct CanFrame {
  uint32_t id = 0;
  std::vector<uint8_t> data;
};

class CanBus {
 public:
  virtual ~CanBus() = default;
  virtual bool open(const std::string& device) = 0;
  virtual void close() = 0;
  virtual bool write_frame(const CanFrame& frame) = 0;
  virtual bool read_frame(CanFrame* frame) = 0;
};

class DummyCanBus final : public CanBus {
 public:
  bool open(const std::string& device) override;
  void close() override;
  bool write_frame(const CanFrame& frame) override;
  bool read_frame(CanFrame* frame) override;

 private:
  std::string device_;
  bool open_ = false;
};

}  // namespace hal
}  // namespace rtos
