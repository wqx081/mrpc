#ifndef MRPC_BASE_ONCE_H_
#define MRPC_BASE_ONCE_H_

#include <stddef.h>
#include "base/atomicops.h"

namespace mrpc {

typedef AtomicWord OnceType;

#define MRPC_ONCE_INIT 0
#define MRPC_DECLARE_ONCE(NAME) ::mrpc::OnceType NAME

enum {
  ONCE_STATE_UNINITIALIZED = 0,
  ONCE_STATE_EXECUTING_FUNCTION = 1,
  ONCE_STATE_DONE = 2
};

typedef void (*NoArgFunction)();
typedef void (*PointerArgFunction)(void* arg);

template<typename T>
struct OneArgFunction {
  typedef void (*type)(T);
};

void CallOnceImpl(OnceType* once, PointerArgFunction init_func, void* arg);
inline void CallOnce(OnceType* once, NoArgFunction init_func) {
  if (Acquire_Load(once) != ONCE_STATE_DONE) {
    CallOnceImpl(once, reinterpret_cast<PointerArgFunction>(init_func),
		 nullptr);
  }
}

template <typename Arg>
inline void CallOnce(OnceType* once,
		     typename OneArgFunction<Arg*>::type init_func,
		     Arg* arg) {
  if (Acquire_Load(once) != ONCE_STATE_DONE) {
    CallOnceImpl(once, reinterpret_cast<PointerArgFunction>(init_func),
		 static_cast<void*>(arg));
  }
}

} // namespace mrpc
#endif // MRPC_BASE_ONCE_H_
