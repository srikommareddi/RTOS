#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rtos {
namespace security {

struct Signature {
  std::vector<uint8_t> bytes;
};

class CryptoProvider {
 public:
  virtual ~CryptoProvider() = default;
  virtual bool verify_signature(const std::vector<uint8_t>& data,
                                const Signature& signature,
                                const std::string& key_id) = 0;
  virtual std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) = 0;
};

}  // namespace security
}  // namespace rtos
