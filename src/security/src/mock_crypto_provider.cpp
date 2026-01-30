#include "../include/security/mock_crypto_provider.h"

#include <algorithm>

namespace rtos {
namespace security {

bool MockCryptoProvider::verify_signature(const std::vector<uint8_t>& data,
                                          const Signature& signature,
                                          const std::string& key_id) {
  if (key_id.empty()) {
    return false;
  }
  return !data.empty() && !signature.bytes.empty();
}

std::vector<uint8_t> MockCryptoProvider::sha256(
    const std::vector<uint8_t>& data) {
  std::vector<uint8_t> digest(32, 0);
  for (size_t i = 0; i < data.size(); ++i) {
    digest[i % digest.size()] ^= data[i];
  }
  return digest;
}

}  // namespace security
}  // namespace rtos
