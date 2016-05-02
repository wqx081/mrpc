#include "base/thread.h"

#include <errno.h>
#include <limits.h>

#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <memory>

namespace mrpc {


const Thread::ThreadId kInvalidThread = (pthread_t) 0;

class Thread::PlatformData {
 public:
  PlatformData() : thread_(kInvalidThread) {}

  ThreadId thread_;
  Mutex thread_creation_mutex_;
};

namespace {

void SetThreadName(const char* name) {
  prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name), 
	0,0,0);
}

void* ThreadEntry(void* arg) {
  Thread* thread = reinterpret_cast<Thread*>(arg);  
  {
    LockGuard<Mutex> lock_guard(&thread->data()->thread_creation_mutex_);
  }
  SetThreadName(thread->name());
  DCHECK(thread->data()->thread_ != kInvalidThread);
  thread->NotifyStartedAndRun();
  return nullptr;
}

} // namespace

Thread::Thread(const Options& options) 
  : data_(new PlatformData),
    stack_size_(options.stack_size()),
    joinable_(options.joinable()) {
  set_name(options.name());
}

Thread::~Thread() {
  delete data_;
}

void Thread::set_name(const char* name) {
  strncpy(name_, name, sizeof(name_));
  name_[sizeof(name_) - 1] = '\0';
}

void Thread::Start() {
  //int result;
  pthread_attr_t attr;
  memset(&attr, 0, sizeof(attr));
  HANDLE_EINTR(pthread_attr_init(&attr));

  if (!joinable_) {
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  }

  if (stack_size_ == 0) {
    size_t default_stack_size;
    struct rlimit stack_rlimit;
    if (pthread_attr_getstacksize(&attr, &default_stack_size) == 0 &&
        getrlimit(RLIMIT_STACK, &stack_rlimit) == 0 &&
	stack_rlimit.rlim_cur != RLIM_INFINITY) {
      stack_size_ = std::max(std::max(default_stack_size,
			              static_cast<size_t>(PTHREAD_STACK_MIN)),
		             static_cast<size_t>(stack_rlimit.rlim_cur));
    }
  }
  if (stack_size_ > 0) {
    pthread_attr_setstacksize(&attr, stack_size_);
  }

  {
    LockGuard<Mutex> lock_guard(&data_->thread_creation_mutex_);
    HANDLE_EINTR(pthread_create(&data_->thread_,
			        &attr,
				ThreadEntry,
				this));
  }
  HANDLE_EINTR(pthread_attr_destroy(&attr));
  DCHECK(data_->thread_ != kInvalidThread);
}

void Thread::Join() {
  pthread_join(data_->thread_, nullptr);
}

// statics
Thread::ThreadId Thread::CurrentId() {
  return syscall(__NR_gettid);
}

void Thread::YieldCurrentThread() {
  sched_yield();
}

void Thread::Sleep(TimeDelta duration) {
  struct timespec sleep_time, remaining;
  sleep_time = duration.ToTimespec();
  while (nanosleep(&sleep_time, &remaining) == -1 && errno == EINTR) {
    sleep_time = remaining;
  }
}

} // namespace mrpc
