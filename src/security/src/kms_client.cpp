#include "../include/security/kms_client.h"

namespace rtos {
namespace security {

KeyMaterial MockKmsClient::fetch_key(const std::string& key_id) {
  KeyMaterial material;
  material.key_id = key_id;
  material.public_key.assign({0x01, 0x02, 0x03});
  return material;
}

}  // namespace security
}  // namespace rtos
