#include "base/semaphore.h"
#include <errno.h>

#include "base/elapsed_timer.h"
#include "base/time.h"

namespace mrpc {

Semaphore::Semaphore(int count) {
  DCHECK(count >= 0);
  memset(&native_handle_, 0, sizeof(native_handle_));
  HANDLE_EINTR(sem_init(&native_handle_, 0, count));
}

Semaphore::~Semaphore() {
  HANDLE_EINTR(sem_destroy(&native_handle_));
}

void Semaphore::Signal() {
  HANDLE_EINTR(sem_post(&native_handle_));
}

void Semaphore::Wait() {
  while (true) {
    int result = _HANDLE_EINTR(sem_wait(&native_handle_));
    if (result == 0) {
      return;
    }
    DCHECK_EQ(-1, result);
    DCHECK_EQ(EINTR, errno);
  }
}

bool Semaphore::WaitFor(const TimeDelta& rel_time) {
  const Time time = Time::NowFromSystemTime() + rel_time;
  const struct timespec ts = time.ToTimespec();

  while (true) {
    int result = sem_timedwait(&native_handle_, &ts);
    if (result == 0) {
      return true;
    }

    if (result > 0) {
      errno = result;
      result = -1;
    }
    if (result == -1 && errno == ETIMEDOUT) {
      return false;
    }
    DCHECK_EQ(-1, result);
    DCHECK_EQ(EINTR, errno);
  }
}

} // namespace mrpc
