#ifndef MRPC_BASE_MUTEX_H_
#define MRPC_BASE_MUTEX_H_

#include "base/lazy_instance.h"

#include <pthread.h>
#include <assert.h>
#include <stdlib.h>

typedef pthread_mutex_t MutexType;

namespace mrpc {

class Mutex final {
 public:
  typedef pthread_mutex_t NativeHandle;

  Mutex();
  ~Mutex();
  void Lock();
  void Unlock();
  bool TryLock();
  NativeHandle& native_handle() { return native_handle_; }
  const NativeHandle& native_handle() const {
    return native_handle_;
  }

 private:
  NativeHandle native_handle_;
  friend class ConditionVariable;
  DISALLOW_COPY_AND_ASSIGN(Mutex);
};

typedef LazyStaticInstance<Mutex,
	                   DefaultConstructTrait<Mutex>,
			   ThreadSafeInitOnceTrait>::type LazyMutex;
#define LAZY_MUTEX_INITIALIZER LAZY_STATIC_INSTANCE_INITIALIZER


class RecursiveMutex final {
 public:
  typedef Mutex::NativeHandle NativeHandle;
  RecursiveMutex();
  ~RecursiveMutex();

  void Lock();
  void Unlock();
  bool TryLock();
  NativeHandle& native_handle() { return native_handle_; }
  const NativeHandle& native_handle() const {
    return native_handle_;
  }

 private:
  NativeHandle native_handle_;

  DISALLOW_COPY_AND_ASSIGN(RecursiveMutex);
};

typedef LazyStaticInstance<RecursiveMutex,
	                   DefaultConstructTrait<RecursiveMutex>,
			   ThreadSafeInitOnceTrait>::type LazyRecursiveMutex;
#define LAZY_RECURSIVE_MUTEX_INITIALIZER LAZY_STATIC_INSTANCE_INITIALIZER

template<typename Mutex>
class LockGuard final {
 public:
  explicit LockGuard(Mutex* mutex) : mutex_(mutex) { mutex_->Lock(); }
  ~LockGuard() { mutex_->Unlock(); }

 private:
  Mutex* mutex_;
  DISALLOW_COPY_AND_ASSIGN(LockGuard);
};


} // namespace mrpc
#endif //MRPC_BASE_MUTEX_H_
