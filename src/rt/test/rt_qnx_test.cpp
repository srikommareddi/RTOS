#include "../include/rt/rt_scheduler.h"

#include <pthread.h>

int main() {
#if defined(__QNXNTO__)
  bool affinity_ok = rtos::rt::RtScheduler::set_thread_affinity(1u);
  bool realtime_ok = rtos::rt::RtScheduler::set_thread_realtime(10);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  bool inheritance_ok =
      rtos::rt::RtScheduler::enable_priority_inheritance(&attr);
  pthread_mutexattr_destroy(&attr);

  return (affinity_ok && realtime_ok && inheritance_ok) ? 0 : 1;
#else
  return 0;
#endif
}
