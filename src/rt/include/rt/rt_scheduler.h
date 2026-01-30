#pragma once

#include <cstdint>
#include <pthread.h>
#include <string>

namespace rtos {
namespace rt {

class RtScheduler {
 public:
  static bool set_thread_realtime(int priority);
  static bool set_thread_name(const std::string& name);
  static bool set_thread_affinity(uint32_t cpu_mask);
  static bool enable_priority_inheritance(pthread_mutexattr_t* attr);
};

}  // namespace rt
}  // namespace rtos
