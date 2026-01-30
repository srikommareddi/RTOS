#include "../include/rt/rt_scheduler.h"

#include <pthread.h>
#include <sched.h>

#if defined(__QNXNTO__)
#include <sys/neutrino.h>
#endif

namespace rtos {
namespace rt {

bool RtScheduler::set_thread_realtime(int priority) {
#if defined(__linux__)
  sched_param params{};
  params.sched_priority = priority;
  return pthread_setschedparam(pthread_self(), SCHED_FIFO, &params) == 0;
#elif defined(__QNXNTO__)
  sched_param params{};
  params.sched_priority = priority;
  return pthread_setschedparam(pthread_self(), SCHED_RR, &params) == 0;
#else
  (void)priority;
  return false;
#endif
}

bool RtScheduler::set_thread_name(const std::string& name) {
#if defined(__linux__)
  return pthread_setname_np(pthread_self(), name.c_str()) == 0;
#elif defined(__QNXNTO__)
  return pthread_setname_np(pthread_self(), name.c_str()) == 0;
#else
  (void)name;
  return false;
#endif
}

bool RtScheduler::set_thread_affinity(uint32_t cpu_mask) {
#if defined(__linux__)
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  for (int i = 0; i < 32; ++i) {
    if (cpu_mask & (1u << i)) {
      CPU_SET(i, &cpuset);
    }
  }
  return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0;
#elif defined(__QNXNTO__)
  unsigned int runmask = cpu_mask;
  return ThreadCtl(_NTO_TCTL_RUNMASK, &runmask) == 0;
#else
  (void)cpu_mask;
  return false;
#endif
}

bool RtScheduler::enable_priority_inheritance(pthread_mutexattr_t* attr) {
  if (!attr) {
    return false;
  }
#if defined(__linux__) || defined(__QNXNTO__)
  return pthread_mutexattr_setprotocol(attr, PTHREAD_PRIO_INHERIT) == 0;
#else
  return false;
#endif
}

}  // namespace rt
}  // namespace rtos
