#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rtos {
namespace hal {

class SpiBus {
 public:
  virtual ~SpiBus() = default;
  virtual bool open(const std::string& device) = 0;
  virtual void close() = 0;
  virtual bool transfer(const std::vector<uint8_t>& tx,
                        std::vector<uint8_t>* rx) = 0;
};

class DummySpiBus final : public SpiBus {
 public:
  bool open(const std::string& device) override;
  void close() override;
  bool transfer(const std::vector<uint8_t>& tx,
                std::vector<uint8_t>* rx) override;

 private:
  std::string device_;
  bool open_ = false;
};

}  // namespace hal
}  // namespace rtos
