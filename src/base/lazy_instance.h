#ifndef MRPC_BASE_LAZY_INSTANCE_H_
#define MRPC_BASE_LAZY_INSTANCE_H_

#include "base/macros.h"
#include "base/once.h"

namespace mrpc {

#define LAZY_STATIC_INSTANCE_INITIALIZER { MRPC_ONCE_INIT, { {} } }
#define LAZY_DYNAMIC_INSTANCE_INITIALIZER { MRPC_ONCE_INIT, 0 }

#define LAZY_INSTANCE_INITIALIZER LAZY_STATIC_INSTANCE_INITIALIZER

template<typename T>
struct LeakyInstanceTrait {
  static void Destroy(T* /* instance */) {}
};

// Traits that define how an instance is allocated and accessed.
//
template<typename T>
struct StaticallyAllocatedInstanceTrait {
  struct ALIGNAS(T, 16) StorageType {
    char x[sizeof(T)];
  };
  static_assert(ALIGNOF(StorageType) >= ALIGNOF(T), "must be same size");

  static T* MutableInstace(StorageType* storage) {
    return reinterpret_cast<T*>(storage);
  }

  template <typename ConstructTrait>
  static void InitStorageUsingTrait(StorageType* storage) {
    ConstructTrait::Construct(MutableInstace(storage));
  }
};

template <typename T>
struct DynamicallyAllocatedInstanceTrait {
  typedef T* StorageType;

  static T* MutableInstance(StorageType* storage) {
    return *storage;
  }

  template <typename CreateTrait>
  static void InitStorageUsingTrait(StorageType* storage) {
    *storage = CreateTrait::Create();
  }
};


template <typename T>
struct DefaultConstructTrait {
  static void Construct(T* allocated_ptr) {
    new(allocated_ptr) T();
  }
};


template <typename T>
struct DefaultCreateTrait {
  static T* Create() {
    return new T();
  }
};


struct ThreadSafeInitOnceTrait {
  template <typename Function, typename Storage>
  static void Init(OnceType* once, Function function, Storage storage) {
    CallOnce(once, function, storage);
  }
};


struct SingleThreadInitOnceTrait {
  template <typename Function, typename Storage>
  static void Init(OnceType* once, Function function, Storage storage) {
    if (*once == ONCE_STATE_UNINITIALIZED) {
      function(storage);
      *once = ONCE_STATE_DONE;
    }
}
};


template <typename T, 
	  typename AllocationTrait, 
	  typename CreateTrait,
	  typename InitOnceTrait, 
	  typename DestroyTrait  /* not used yet. */>
struct LazyInstanceImpl {
 public:
  typedef typename AllocationTrait::StorageType StorageType;

 private:
  static void InitInstance(StorageType* storage) {
    AllocationTrait::template InitStorageUsingTrait<CreateTrait>(storage);
  }

  void Init() const {
    InitOnceTrait::Init(&once_,
		        reinterpret_cast<void(*)(void*)>(&InitInstance), 
                        reinterpret_cast<void*>(&storage_));
  }
 public:
  T* Pointer() {
    Init();
    return AllocationTrait::MutableInstance(&storage_);
  }

  const T& Get() const {
    Init();
    return *AllocationTrait::MutableInstance(&storage_);
  }

  mutable OnceType once_;
  mutable StorageType storage_;
};

template <typename T,
	  typename CreateTrait = DefaultConstructTrait<T>,
	  typename InitOnceTrait = ThreadSafeInitOnceTrait,
	  typename DestroyTrait = LeakyInstanceTrait<T> >
struct LazyStaticInstance {
  typedef LazyInstanceImpl<T, StaticallyAllocatedInstanceTrait<T>,
             CreateTrait, InitOnceTrait, DestroyTrait> type;
};


template <typename T,
          typename CreateTrait = DefaultConstructTrait<T>,
	  typename InitOnceTrait = ThreadSafeInitOnceTrait,
	  typename DestroyTrait = LeakyInstanceTrait<T> >
struct LazyInstance {
  typedef typename LazyStaticInstance<T, CreateTrait, InitOnceTrait,
	                              DestroyTrait>::type type;
};

template <typename T,
          typename CreateTrait = DefaultCreateTrait<T>,
	  typename InitOnceTrait = ThreadSafeInitOnceTrait,
	  typename DestroyTrait = LeakyInstanceTrait<T> >
struct LazyDynamicInstance {
  typedef LazyInstanceImpl<T, DynamicallyAllocatedInstanceTrait<T>,
                           CreateTrait, InitOnceTrait, DestroyTrait> type;
};

} // namespace mrpc

#endif // MRPC_BASE_LAZY_INSTANCE_H_
