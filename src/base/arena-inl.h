#ifndef MRPC_BASE_ARENA_INL_H_
#define MRPC_BASE_ARENA_INL_H_

#include "base/arena.h"
#include <assert.h>
#include <stddef.h>
#include <new>
#include <memory>

namespace mrpc {

template<typename T, typename C>
class ArenaAllocator {
 public:
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
  
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    pointer address(reference r) const  { return &r; }
    const_pointer address(const_reference r) const  { return &r; }
    size_type max_size() const  { return size_t(-1) / sizeof(T); }
  
    ArenaAllocator() : arena_(0) { }
    ArenaAllocator(C* arena) : arena_(arena) { }  // NOLINT
    ~ArenaAllocator() { }
  
    pointer allocate(size_type n,
                     std::allocator<void>::const_pointer /*hint*/ = 0) {
      assert(arena_ && "No arena to allocate from!");
      return reinterpret_cast<T*>(arena_->AllocAligned(n * sizeof(T),
                                                       kAlignment));
    }
    void deallocate(pointer p, size_type n) {
      arena_->Free(p, n * sizeof(T));
    }
    void construct(pointer p, const T & val) {
      new(reinterpret_cast<void*>(p)) T(val);
    }
    void construct(pointer p) {
      new(reinterpret_cast<void*>(p)) T();
    }
    void destroy(pointer p) { p->~T(); }
  
    C* arena(void) const { return arena_; }
  
    template<class U> struct rebind {
      typedef ArenaAllocator<U, C> other;
    };
   template<class U> ArenaAllocator(const ArenaAllocator<U, C>& other)
      : arena_(other.arena()) { }
  
    template<class U> bool operator==(const ArenaAllocator<U, C>& other)   const {
      return arena_ == other.arena();
    }
  
    template<class U> bool operator!=(const ArenaAllocator<U, C>& other)   const {
      return arena_ != other.arena();
    }
  
   protected:
    static const int kAlignment;
    C* arena_;
  };
  
  template<class T, class C> const int ArenaAllocator<T, C>::kAlignment =
      (1 == sizeof(T) ? 1 : BaseArena::kDefaultAlignment);

// 'new' must be in the global namespace.
} // namespace mrpc

using mrpc::UnsafeArena;

enum AllocateInArenaType { AllocateInArena };
  
inline void* operator new(size_t size,
                          AllocateInArenaType /* unused */,
                          UnsafeArena *arena) {
  return arena->Alloc(size);
}
  
inline void* operator new[](size_t size,
                            AllocateInArenaType /* unused */,
                            UnsafeArena *arena) {
  return arena->Alloc(size);
}

namespace mrpc {

class Gladiator {
   public:
    Gladiator() { }
    virtual ~Gladiator() { }
    void* operator new(size_t size) {
      void* ret = ::operator new(1 + size);
      static_cast<char *>(ret)[size] = 1;     // mark as heap-allocated
      return ret;
    }
    template<class T> void* operator new(size_t size, const int ignored,
                                         T* allocator) {
      if (allocator) {
        void* ret = allocator->AllocAligned(1 + size,
                                            BaseArena::kDefaultAlignment)  ;   
        static_cast<char*>(ret)[size] = 0;  // mark as arena-allocated
        return ret;
      } else {
        return operator new(size);          // this is the function above
      }
    }   
    void operator delete(void* memory, size_t size) {
      if (static_cast<char*>(memory)[size]) {
        assert (1 == static_cast<char *>(memory)[size]);
        ::operator delete(memory);
      } else {
      }
    }   
    template<class T> void operator delete(void* memory, size_t size,
                                           const int ign, T* allocator) {
      if (allocator) {
        allocator->Free(memory, 1 + size);
      } else {
        ::operator delete(memory);
      }
    }
};

class ArenaOnlyGladiator {
   public:
    ArenaOnlyGladiator() { }
    void* operator new(size_t /*size*/) {
      assert(0);
      return reinterpret_cast<void *>(1);
    }
    void* operator new[](size_t /*size*/) {
      assert(0);
      return reinterpret_cast<void *>(1);
    }
    template<class T> void* operator new(size_t size, const int ignored,
                                         T* allocator) {
      assert(allocator);
      return allocator->AllocAligned(size, BaseArena::kDefaultAlignment);
    }
    template<class T> void* operator new[](size_t size,
                             const int ignored, T* allocator) {
      assert(allocator);
      return allocator->AllocAligned (size, BaseArena::kDefaultAlignment)  ;
    }
    void operator delete(void* /*memory*/, size_t /*size*/) { }
    template<class T> void operator delete(void* memory, size_t size,
                                           const int ign, T* allocator) {   }
    void operator delete [](void* /*memory*/) { }
    template<class T> void operator delete(void* memory,
                                           const int ign, T* allocator) {   }
};

} // namespace mrpc

#endif // MRPC_BASE_ARENA_INL_H_
