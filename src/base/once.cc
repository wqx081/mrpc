#include "base/once.h"
#include <sched.h>
#include "base/atomicops.h"

namespace mrpc {

void CallOnceImpl(OnceType* once, PointerArgFunction init_func,
		  void* arg) {
  AtomicWord state = Acquire_Load(once);
  if (state == ONCE_STATE_DONE) {
    return;
  }

  state = Acquire_CompareAndSwap(once,
		                 ONCE_STATE_UNINITIALIZED,
				 ONCE_STATE_EXECUTING_FUNCTION);
  if (state == ONCE_STATE_UNINITIALIZED) {
    init_func(arg);
    Release_Store(once, ONCE_STATE_DONE);
  } else {
    while (state == ONCE_STATE_EXECUTING_FUNCTION) {
      sched_yield();
      state = Acquire_Load(once);
    }
  }
}

} // namespace mrpc
