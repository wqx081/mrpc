#ifndef MRPC_BASE_SCOPED_PTR_H_
#define MRPC_BASE_SCOPED_PTR_H_

#include "base/macros.h"
#include "base/template_util.h"

#include <memory>

namespace mrpc {

class RefCountedBase;
class RefCountedThreadSafeBase;

struct FreeDeleter {
  inline void operator()(void* ptr) const {
    free(ptr);
  }
};

namespace internal {

template<typename T>
struct IsNotRefCounted {
  enum {
    value = !is_convertible<T*, RefCountedBase*>::value &&
	    !is_convertible<T*, RefCountedThreadSafeBase*>::value
  };
};

template<typename T, typename D>
class scoped_ptr_impl {
 public:
 private:
  template<typename U, typename V> friend class scoped_ptr_impl;
  struct Data : public D {
    explicit Data(T* in_ptr) : ptr(in_ptr) {}
    Data(T* in_ptr, const D& other) : D(other), ptr(in_ptr) {}

    T* ptr;
  };

  Data data_;

  DISALLOW_COPY_AND_ASSIGN(scoped_ptr_impl);
};

} // namespace internal

} // namespace mrpc


template<typename T, class D = std::default_delete<T>>
class scoped_ptr {
  DISALLOW_COPY_AND_ASSIGN_WITH_MOVE_FOR_BUILD(scoped_ptr);
};

#endif // MRPC_BASE_SCOPED_PTR_H_
