#ifndef MRPC_BASE_MACROS_H_
#define MRPC_BASE_MACROS_H_

#include <stddef.h>
#include <stdint.h>
#include <cstring>

#include <glog/logging.h>
#include <errno.h>

template<typename Dest, typename Source>
inline Dest bit_cast(const Source& source) {
  static_assert(sizeof(Dest) == sizeof(source), 
		  "source and dest must be same size");
  Dest dest;
  memcpy(&dest, &source, sizeof(dest));
  return dest;
}

#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

#define DISALLOW_ASSIGN(TypeName)	        \
  void operator=(const TypeName)
#define DISALLOW_COPY_AND_ASSIGN(TypeName)	\
  TypeName(const TypeName&) = delete;           \
  void operator=(const TypeName&) = delete

#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName() = delete;                           \
  DISALLOW_COPY_AND_ASSIGN(TypeName)


//#define ALIGNAS(byte_alignment) __attribute__((aligned(byte_alignment)))
#define ALIGNAS(type, alignment) __attribute__((aligned(__alignof__(type))))
#define ALIGNOF(type) __alignof__(type)

#define _HANDLE_EINTR(x) ({ \
  decltype(x) eintr_wrapper_result; \
  do { \
    eintr_wrapper_result = (x); \
  } while (eintr_wrapper_result == -1 && errno == EINTR); \
  eintr_wrapper_result; \
})

#define HANDLE_EINTR(x) DCHECK_EQ(0, _HANDLE_EINTR(x))



#define MOVE_ONLY_TYPE_FOR_CPP_03(type) \
  DISALLOW_COPY_AND_ASSIGN_WITH_MOVE_FOR_BIND(type)

#define DISALLOW_COPY_AND_ASSIGN_WITH_MOVE_FOR_BIND(type)       \
 private:                                                       \
  type(const type&) = delete;                                   \
  void operator=(const type&) = delete;                         \
                                                                \
 public:                                                        \
  typedef void MoveOnlyTypeForCPP03;                            \
                                                                \
 private:

#endif //MRPC_BASE_MACROS_H_
