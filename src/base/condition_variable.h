#ifndef MRPC_BASE_CONDITION_VARIABLE_H_
#define MRPC_BASE_CONDITION_VARIABLE_H_
#include "base/lazy_instance.h"
#include "base/mutex.h"

namespace mrpc {

class ConditionVariableEvent;
class TimeDelta;

class ConditionVariable final {
 public:
  typedef pthread_cond_t NativeHandle;

  ConditionVariable();
  ~ConditionVariable();

  void NotifyOne();
  void NotifyAll();
  void Wait(Mutex* mutex);
  bool WaitFor(Mutex* mutex, const TimeDelta& rel_time);

  NativeHandle& native_handle() { return native_handle_; }
  const NativeHandle& native_handle() const {
    return native_handle_;
  }

 private:
  NativeHandle native_handle_;

  DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};

typedef LazyStaticInstance<ConditionVariable,
	                   DefaultConstructTrait<ConditionVariable>,
			   ThreadSafeInitOnceTrait>::type LazyConditionVariable;
#define LAZY_CONDITION_VARIABLE_INITIALIZER LAZY_STATIC_INSTANCE_INITIALIZER

} // namespace mrpc
#endif // MRPC_BASE_CONDITION_VARIABLE_H_
