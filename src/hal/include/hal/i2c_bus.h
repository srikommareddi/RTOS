#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rtos {
namespace hal {

class I2cBus {
 public:
  virtual ~I2cBus() = default;
  virtual bool open(const std::string& device) = 0;
  virtual void close() = 0;
  virtual bool write(uint8_t address, const std::vector<uint8_t>& bytes) = 0;
  virtual bool read(uint8_t address, size_t length,
                    std::vector<uint8_t>* out) = 0;
};

class DummyI2cBus final : public I2cBus {
 public:
  bool open(const std::string& device) override;
  void close() override;
  bool write(uint8_t address, const std::vector<uint8_t>& bytes) override;
  bool read(uint8_t address, size_t length,
            std::vector<uint8_t>* out) override;

 private:
  std::string device_;
  bool open_ = false;
};

}  // namespace hal
}  // namespace rtos
