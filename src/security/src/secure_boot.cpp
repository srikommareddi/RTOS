#include "../include/security/secure_boot.h"

namespace rtos {
namespace security {

SecureBootVerifier::SecureBootVerifier(CryptoProvider* crypto, KmsClient* kms)
    : crypto_(crypto), kms_(kms) {}

bool SecureBootVerifier::verify(const BootImage& image) {
  if (!crypto_ || !kms_) {
    return false;
  }
  if (image.payload.empty() || image.key_id.empty()) {
    return false;
  }
  auto key = kms_->fetch_key(image.key_id);
  if (key.key_id.empty() || key.public_key.empty()) {
    return false;
  }
  return crypto_->verify_signature(image.payload, image.signature, key.key_id);
}

}  // namespace security
}  // namespace rtos
