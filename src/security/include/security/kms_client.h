#pragma once

#include <string>
#include <vector>

namespace rtos {
namespace security {

struct KeyMaterial {
  std::string key_id;
  std::vector<uint8_t> public_key;
};

class KmsClient {
 public:
  virtual ~KmsClient() = default;
  virtual KeyMaterial fetch_key(const std::string& key_id) = 0;
};

class MockKmsClient final : public KmsClient {
 public:
  KeyMaterial fetch_key(const std::string& key_id) override;
};

}  // namespace security
}  // namespace rtos
