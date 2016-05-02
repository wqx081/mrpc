#ifndef MRPC_BASE_ARENA_H_
#define MRPC_BASE_ARENA_H_

#include "base/mutex.h"
#include "base/macros.h"
#include <assert.h>
#include <string.h>
#include <vector>

namespace mrpc {

class BaseArena {
 protected:
  BaseArena(char* first_block, const size_t block_size, 
            bool align_to_page);

 public:
  virtual ~BaseArena();
  virtual void Reset();
  class Handle {
   public:
    static const uint32_t kInvalidValue = 0xFFFFFFFF;
    Handle() : handle_(kInvalidValue) {}
    bool operator==(const Handle& h) const {
      return handle_ == h.handle_;
    }
    bool operator!=(const Handle& h) const {
      return !(*this == h);
    }

    uint32_t hash() const { return handle_; }
    bool valid() const { return handle_ != kInvalidValue; }

   private:
    friend class BaseArena;
    explicit Handle(uint32_t handle) : handle_(handle) {}
    uint32_t handle_;
  };
  class Status {
   public:
    Status() : bytes_allocated_(0) {}

    size_t bytes_allocated() const {
      return bytes_allocated_;
    }

   private:
    friend class BaseArena;
    size_t bytes_allocated_; 
  };
   
  virtual char* SlowAlloc(size_t size) = 0;
  virtual void  SlowFree(void* memory, size_t size) = 0;
  virtual char* SlowRealloc(char* memory, 
		            size_t old_size, 
		            size_t new_size) = 0;
  virtual char* SlowAllocWithHandle(const size_t size, Handle* handle) = 0;

  void set_handle_alignment(int align);
  void* HandleToPointer(const Handle& h) const;

  virtual BaseArena* arena() { return this; }
  size_t block_size() const { return block_size_; }
  int block_count() const;
  bool is_empty() const {
    return freestart_ == freestart_when_empty_ && 1 == block_count();
  }

//#ifdef __i386__
  static const int kDefaultAlignment = 4;
//#else
//  static const int kDefaultAlignment = 8;
//#endif
 protected:
  void MakeNewBlock();
  void* GetMemoryFallback(const size_t size, const int align);

  void* GetMemory(const size_t size, const int align) {
    assert(remaining_ <= block_size_);
    if (size > 0 && size < remaining_ && align == 1) {
      last_alloc_ = freestart_;
      freestart_ += size;
      remaining_ -= size;
      return reinterpret_cast<void*>(last_alloc_);
    }
    return GetMemoryFallback(size, align);
  }

  void ReturnMemory(void* memory, const size_t size) {
    size_t t = static_cast<size_t>(freestart_ - last_alloc_);
    if (memory == last_alloc_ && t == size) {
      remaining_ += size;
      freestart_ = last_alloc_;  
    }
  }


  bool AdjustLastAlloc(void* last_alloc, const size_t new_size);
  void* GetMemoryWithHandle(const size_t size, Handle* handle);

  Status status_;
  size_t remaining_;

 private: 
  struct AllocatedBlock {
    char* mem;
    size_t size;
  };
  AllocatedBlock* AllocNewBlock(const size_t block_size);
  const AllocatedBlock* IndexToBlock(int index) const;

  const int first_block_we_own_;
  const size_t block_size_;
  char* freestart_;
  char* freestart_when_empty_;
  char* last_alloc_;
  int blocks_alloced_;
  AllocatedBlock first_blocks_[16];
  std::vector<AllocatedBlock>* overflow_blocks_;
  const bool page_aligned_;
  int handle_alignment_;
  int handle_alignment_bits_;
  size_t block_size_bits_;
  void FreeBlocks();

  //
  // friend class ProtectableUnsafeArena;
  DISALLOW_COPY_AND_ASSIGN(BaseArena);
};


class UnsafeArena : public BaseArena {
 public:
  explicit UnsafeArena(const size_t block_size)
    : BaseArena(nullptr, block_size, false) {}
  UnsafeArena(const size_t block_size, bool align)
    : BaseArena(nullptr, block_size, align) {}
  UnsafeArena(char* first_block, const size_t block_size)
    : BaseArena(first_block, block_size, false) {}
  UnsafeArena(char* first_block, const size_t block_size, bool align)
    : BaseArena(first_block, block_size, align) {}

  char* Alloc(const size_t size) {
    return reinterpret_cast<char*>(GetMemory(size, 1));
  }

  void* AllocAligned(const size_t size, const int align) {
    return GetMemory(size, align);
  }

  char* Calloc(const size_t size) {
    void* return_value = Alloc(size);
    memset(return_value, 0, size);
    return reinterpret_cast<char*>(return_value);
  }
  void* CallocAligned(const size_t size, const int align) {
    void* return_value = AllocAligned(size, align);
    memset(return_value, 0, size);
    return return_value;
  }
  void Free(void* memory, size_t size) {
    ReturnMemory(memory, size);
  }
  typedef BaseArena::Handle Handle;
  char* AllocWithHandle(const size_t size, Handle* handle) {
    return reinterpret_cast<char*>(GetMemoryWithHandle(size, handle));
  }

  virtual char* SlowAlloc(size_t size) override {
    return Alloc(size);
  }
  virtual void SlowFree(void* memory, size_t size) override {
    Free(memory, size);
  }
  virtual char* SlowRealloc(char* memory, size_t old_size, 
		            size_t new_size) override {
    return Realloc(memory, old_size, new_size);
  }
  virtual char* SlowAllocWithHandle(const size_t size, 
		                    Handle* handle) override {
    return AllocWithHandle(size, handle);
  }

  char* Memdup(const char* s, size_t bytes) {
    char* new_str = Alloc(bytes);
    memcpy(new_str, s, bytes);
    return new_str;
  }
  char* MemdupPlusNULL(const char* s, size_t bytes) {
    char* new_str = Alloc(bytes + 1);
    memcpy(new_str, s, bytes);
    new_str[bytes] = '\0';
    return new_str;
  }
  Handle MemdupWithHandle(const char* s, size_t bytes) {
    Handle handle;
    char* new_str = AllocWithHandle(bytes, &handle);
    memcpy(new_str, s, bytes);
    return handle;
  }
  char* Strdup(const char* s) {
    return Memdup(s, strlen(s) + 1);
  }
  char* Strndup(const char* s, size_t n) {
    const char* eos = reinterpret_cast<const char*>(memchr(s, '\0', n));
    const size_t bytes = (eos == nullptr) ? n + 1 : eos - s + 1;
    char* ret = Memdup(s, bytes);
    ret[bytes - 1] = '\0';
    return ret;
  }

  char* Realloc(char* s, size_t old_size, size_t new_size);
  char* Shrink(char* s, size_t new_size) {
    AdjustLastAlloc(s, new_size);
    return s;
  }
  Status status() const { return status_; }
  size_t bytes_until_next_allocation() const { return remaining_; }

 private:
  DISALLOW_COPY_AND_ASSIGN(UnsafeArena);
};

class SafeArena : public BaseArena {
 public:
  explicit SafeArena(const size_t block_size)
    : BaseArena(nullptr, block_size, false) { }

  SafeArena(char* first_block, const size_t block_size)
    : BaseArena(first_block, block_size, false) { }

  virtual void Reset() override { // Lock
    LockGuard<Mutex> lock(&mutex_);
    BaseArena::Reset();
  }

  char* Alloc(const size_t size) { // Lock
    LockGuard<Mutex> lock(&mutex_);
    return reinterpret_cast<char*>(GetMemory(size, 1));
  }
  // Lock
  void* AllocAligned(const size_t size, const int align) {
    LockGuard<Mutex> lock(&mutex_);
    return GetMemory(size, align);
  }

  char* Calloc(const size_t size) {
    void* return_value = Alloc(size);
    memset(return_value, 0, size);
    return reinterpret_cast<char*>(return_value);
  }
  void* CallocAligned(const size_t size, const int align) {
    void* return_value = AllocAligned(size, align);
    memset(return_value, 0, size);
    return return_value;
  }
  void Free(void* memory, size_t size) {
    LockGuard<Mutex> lock(&mutex_);
    ReturnMemory(memory, size);
  }
  typedef BaseArena::Handle Handle;
  char* AllocWithHandle(const size_t size, Handle* handle) {
    LockGuard<Mutex> lock(&mutex_);
    return reinterpret_cast<char*>(GetMemoryWithHandle(size, handle));
  }

  virtual char* SlowAlloc(size_t size) override {
    return Alloc(size);
  }
  virtual void SlowFree(void* memory, size_t size) override {
    Free(memory, size);
  }
  
  virtual char* SlowRealloc(char* memory, 
		            size_t old_size, 
			    size_t new_size) override {
    return Realloc(memory, old_size, new_size);
  }
  virtual char* SlowAllocWithHandle(const size_t size, 
		                    Handle* handle) override {
    return AllocWithHandle(size, handle);
  }

  char* Memdup(const char* s, size_t bytes) {
    char* newstr = Alloc(bytes);
    memcpy(newstr, s, bytes);
    return newstr;
  }

  char* MemdupPlusNUL(const char* s, size_t bytes) {  // like "string(s  , len)"
    char* newstr = Alloc(bytes+1);
    memcpy(newstr, s, bytes);
    newstr[bytes] = '\0';
    return newstr;
  }
  Handle MemdupWithHandle(const char* s, size_t bytes) {
    Handle handle;
    char* newstr = AllocWithHandle(bytes, &handle);
    memcpy(newstr, s, bytes);
    return handle;
  }

  char* Strdup(const char* s) {
    return Memdup(s, strlen(s) + 1);
  }
  char* Strndup(const char* s, size_t n) {
    const char* eos = reinterpret_cast<const char*>(memchr(s, '\0', n))  ;
    const size_t bytes = (eos == NULL) ? n + 1 : eos - s + 1;
    char* ret = Memdup(s, bytes);
    ret[bytes-1] = '\0'; // make sure the string is NUL-termi  nated
    return ret;
  }


  char* Realloc(char* s, size_t old_size, size_t new_size);

  char* Shrink(char* s, size_t new_size) {
    LockGuard<Mutex> lock(&mutex_);
    AdjustLastAlloc(s, new_size);
    return s;
  } 

  Status status() {
    LockGuard<Mutex> lock(&mutex_);
    return status_;
  }

  size_t bytes_until_next_allocation() {
    LockGuard<Mutex> lock(&mutex_);
    return remaining_;
  }

 protected:
  Mutex mutex_;
 private:
  DISALLOW_COPY_AND_ASSIGN(SafeArena);
};

} // namespace mrpc
#endif // MRPC_BASE_ARENA_H_
