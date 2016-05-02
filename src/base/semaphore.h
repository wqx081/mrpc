#ifndef MRPC_BASE_SEMAPHORE_H_
#define MRPC_BASE_SEMAPHORE_H_
#include "base/lazy_instance.h"
#include <semaphore.h>

namespace mrpc {

class TimeDelta;

class Semaphore final {
 public:
  typedef sem_t NativeHandle;

  explicit Semaphore(int count);
  ~Semaphore();

  void Signal();
  void Wait();
  bool WaitFor(const TimeDelta& rel_time);
  NativeHandle& native_handle() { return native_handle_; }
  const NativeHandle& native_handle() const {
    return native_handle_;
  }

 private:
  NativeHandle native_handle_;

  DISALLOW_COPY_AND_ASSIGN(Semaphore);  
};

template<int N>
struct CreateSemaphoreTrait {
  static Semaphore* Create() {
    return new Semaphore(N);
  }
};

template<int N>
struct LazySemaphore {
  typedef typename LazyDynamicInstance<Semaphore,
	                               CreateSemaphoreTrait<N>,
				       ThreadSafeInitOnceTrait>::type type;

};

#define LAZY_SEMAPHORE_INITIALIZER LAZY_DYNAMIC_INSTANCE_INITIALIZER

} // namespace mrpc
#endif // MRPC_BASE_SEMAPHORE_H_
