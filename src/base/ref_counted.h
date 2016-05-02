#ifndef MRPC_BASE_REF_COUNTED_H_
#define MRPC_BASE_REF_COUNTED_H_

#include <cassert>
#include <iosfwd>

#include "base/atomic_ref_count.h"
#include "base/macros.h"

namespace mrpc {

class RefCountedBase {
 public:
  bool HasOneRef() const { return ref_count_ == 1; }

 protected:
  RefCountedBase()
    : ref_count_(0) {}
  ~RefCountedBase() {}

  void AddRef() const { ++ref_count_; }
  bool Release() const { 
    if (--ref_count_ == 0) {
      return true;
    }
    return false;
  }

 private:
  mutable int ref_count_;
  DISALLOW_COPY_AND_ASSIGN(RefCountedBase);
};

class RefCountedThreadSafeBase {
 public:
  bool HasOneRef() const;

 protected:
  RefCountedThreadSafeBase();
  ~RefCountedThreadSafeBase();

  void AddRef() const;
  bool Release() const;

 private:
  mutable AtomicRefCount ref_count_;
  DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafeBase);
};

template<typename T>
class RefCounted : public RefCountedBase {
 public:
  RefCounted() {}

  void AddRef() const {
    RefCountedBase::AddRef();
  }

  void Release() const {
    if (RefCountedBase::Release()) {
      delete static_cast<const T*>(this);
    }
  }

 protected:
  ~RefCounted() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(RefCounted<T>);
};

template<typename T, typename Traits> class RefCountedThreadSafe;

template<typename T>
struct DefaultRefCountedThreadSafeTraits {
  static void Destruct(const T* x) {
    RefCountedThreadSafe<T,
	                 DefaultRefCountedThreadSafeTraits>::DeleteInternal(x);
  }
};

template<typename T, typename Traits = DefaultRefCountedThreadSafeTraits<T>>
class RefCountedThreadSafe : public RefCountedThreadSafeBase {
 public:
  RefCountedThreadSafe() {}

  void AddRef() const {
    RefCountedThreadSafeBase::AddRef();
  }

  void Release() const {
    if (RefCountedThreadSafeBase::Release()) {
      Traits::Destruct(static_cast<const T*>(this));
    }
  }

 protected:
  ~RefCountedThreadSafe() {}

 private:
  friend struct DefaultRefCountedThreadSafeTraits<T>;
  static void DeleteInternal(const T* x) { delete x; }

  DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafe);
};

template <typename T>
class RefCountedData
    : public RefCountedThreadSafe<RefCountedData<T>> {
 public:
  RefCountedData() : data() {}
  RefCountedData(const T& in_value) : data(in_value) {}

  T data;
 private:
  friend class RefCountedThreadSafe<RefCountedData<T>>;
  ~RefCountedData();
};

// Smart pointer
template<typename T>
class scoped_refptr {
 public:
  typedef T element_type;

  scoped_refptr() : ptr_(nullptr) {}
  scoped_refptr(T* p) : ptr_(p) {
    if (ptr_) {
      AddRef(ptr_);
    }
  }

  // Copy constructor
  scoped_refptr(const scoped_refptr<T>& r) : ptr_(r.ptr_) {
    if (ptr_) {
      AddRef(ptr_);
    }
  }

  // Copy conversion constructor
  template<typename U>
  scoped_refptr(const scoped_refptr<U>& r) : ptr_(r.get()) {
    if (ptr_) {
      AddRef(ptr_);
    }
  }

  scoped_refptr(scoped_refptr&& r) : ptr_(r.get()) {
    r.ptr_ = nullptr;
  }

  template<typename U>
  scoped_refptr(scoped_refptr<U>&& r) : ptr_(r.get()) {
    r.ptr_ = nullptr;
  }

  ~scoped_refptr() {
    if (ptr_) {
      Release(ptr_);
    }
  }

  T* get() const {
    return ptr_;
  }

  T& operator*() const {
    assert(ptr_ != nullptr);
    return *ptr_;
  }

  T* operator->() const {
    assert(ptr_ != nullptr);
    return ptr_;
  }

  scoped_refptr<T>& operator=(T* p) {
    if (p) {
      AddRef(p);
    }
    T* old_ptr = ptr_;
    ptr_ = p;
    if (old_ptr) {
      Release(old_ptr);
    }
    return *this;
  }

  scoped_refptr<T>& operator=(const scoped_refptr<T>& r) {
    return *this = r.ptr_;
  }

  template<typename U>
  scoped_refptr<T>& operator=(const scoped_refptr<U>& r) {
    return *this = r.get();
  }

  scoped_refptr<T>& operator=(scoped_refptr<T>&& r) {
    scoped_refptr<T>(std::move(r)).swap(*this);
    return *this;
  }

  template<typename U>
  scoped_refptr<T>& operator=(scoped_refptr<U>&& r) {
    scoped_refptr<T>(std::move(r)).swap(*this);
    return *this;
  }

  void swap(T** pp) {
    T* p = ptr_;
    ptr_ = *pp;
    *pp = p;
  }

  void swap(scoped_refptr<T>& r) {
    swap(&r.ptr_);
  }

 private:
  template<typename U> friend class scoped_refptr;

  typedef T* scoped_refptr::*Testable;
 public:
  operator Testable() const {
    return ptr_ ? &scoped_refptr::ptr_ : nullptr;
  }

  template<typename U>
  bool operator==(const scoped_refptr<U>& rhs) const {
    return ptr_ == rhs.get();
  }

  template<typename U>
  bool operator!=(const scoped_refptr<U>& rhs) const {
    return !operator==(rhs);
  }

  template<typename U>
  bool operator<(const scoped_refptr<U>& rhs) const {
    return ptr_ < rhs.get();
  }

 protected:
  T* ptr_;

 private:
  static void AddRef(T* ptr);
  static void Release(T* ptr);
};

template<typename T>
void scoped_refptr<T>::AddRef(T* ptr) {
  ptr->AddRef();
}

template<typename T>
void scoped_refptr<T>::Release(T* ptr) {
  ptr->Release();
}

template<typename T>
scoped_refptr<T> make_scoped_refptr(T* t) {
  return scoped_refptr<T>(t);
}

template<typename T, typename U>
bool operator==(const scoped_refptr<T>& lhs, const U* rhs) {
  return lhs.get() == rhs;
}

template<typename T, typename U>
bool operator==(const T* lhs, const scoped_refptr<U>& rhs) {
  return lhs == rhs.get();
}

template<typename T, typename U>
bool operator!=(const scoped_refptr<T>& lhs, const U* rhs) {
  return !operator==(lhs, rhs);
}

template<typename T, typename U>
bool operator!=(const T* lhs, const scoped_refptr<U>& rhs) {
  return !operator==(lhs, rhs);
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const scoped_refptr<T>& p) {
  return out << p.get();
}

} // namespace mrpc
#endif // MRPC_BASE_REF_COUNTED_H_
