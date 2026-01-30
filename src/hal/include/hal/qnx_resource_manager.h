#pragma once

#include <string>
#include <thread>

namespace rtos {
namespace hal {

class QnxResourceManager {
 public:
  QnxResourceManager();
  ~QnxResourceManager();

  bool start(const std::string& mount_point);
  void stop();

 private:
  bool running_ = false;
  std::string mount_point_;
  void* dispatch_handle_ = nullptr;
  void* dispatch_context_ = nullptr;
  int resmgr_id_ = -1;
  std::thread dispatch_thread_;
};

}  // namespace hal
}  // namespace rtos
