#pragma once

#include "crypto_provider.h"

namespace rtos {
namespace security {

class MockCryptoProvider final : public CryptoProvider {
 public:
  bool verify_signature(const std::vector<uint8_t>& data,
                        const Signature& signature,
                        const std::string& key_id) override;
  std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) override;
};

}  // namespace security
}  // namespace rtos
