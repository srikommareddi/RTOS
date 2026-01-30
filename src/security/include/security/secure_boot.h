#pragma once

#include "crypto_provider.h"
#include "kms_client.h"

#include <string>
#include <vector>

namespace rtos {
namespace security {

struct BootImage {
  std::string version;
  std::string key_id;
  std::vector<uint8_t> payload;
  Signature signature;
};

class SecureBootVerifier {
 public:
  SecureBootVerifier(CryptoProvider* crypto, KmsClient* kms);

  bool verify(const BootImage& image);

 private:
  CryptoProvider* crypto_;
  KmsClient* kms_;
};

}  // namespace security
}  // namespace rtos
