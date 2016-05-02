#include "base/arena.h"
#include "base/arena-inl.h"
#include <assert.h>
#include <algorithm>
#include <unistd.h>
#include <vector>
#include <sys/types.h>         // one place uintptr_t might be
#include <inttypes.h>
#include "base/macros.h"       // for uint64
#include "base/mutex.h"

using std::min;
using std::vector;

static void* aligned_malloc(size_t size, size_t alignment) {
  void* ptr = nullptr;
  const int required_alignment = sizeof(void*);
  if (alignment < required_alignment) {
    return malloc(size);
  }
  if (posix_memalign(&ptr, alignment, size) != 0) {
    return nullptr;
  } else {
    return ptr;
  }
}

// The value here doesn't matter until page_aligned_ is supported.
static const int kPageSize = 8192;   // should be getpagesize()

namespace mrpc {

// We used to only keep track of how much space has been allocated in
// debug mode. Now we track this for optimized builds, as well. If you
// want to play with the old scheme to see if this helps performance,
// change this ARENASET() macro to a NOP. However, NOTE: some
// applications of arenas depend on this space information (exported
// via bytes_allocated()).
#define ARENASET(x) (x)

// ----------------------------------------------------------------------
// BaseArena::BaseArena()
// BaseArena::~BaseArena()
//    Destroying the arena automatically calls Reset()
// ----------------------------------------------------------------------


BaseArena::BaseArena(char* first, const size_t block_size, bool align_to_page)
  : remaining_(0),
    first_block_we_own_(first ? 1 : 0),
    block_size_(block_size),
    freestart_(NULL),                   // set for real in Reset()
    last_alloc_(NULL),
    blocks_alloced_(1),
    overflow_blocks_(NULL),
    page_aligned_(align_to_page),
    handle_alignment_(1),
    handle_alignment_bits_(0),
    block_size_bits_(0) {
  assert(block_size > kDefaultAlignment);

  while ((static_cast<size_t>(1) << block_size_bits_) < block_size_) {
    ++block_size_bits_;
  }

  if (page_aligned_) {
    // kPageSize must be power of 2, so make sure of this.
    CHECK(kPageSize > 0 && 0 == (kPageSize & (kPageSize - 1)))
                              << "kPageSize[ " << kPageSize << "] is not "
                              << "correctly initialized: not a power of 2.";
  }

  if (first) {
    CHECK(!page_aligned_ ||
          (reinterpret_cast<uintptr_t>(first) & (kPageSize - 1)) == 0);
    first_blocks_[0].mem = first;
  } else {
    if (page_aligned_) {
      // Make sure the blocksize is page multiple, as we need to end on a page
      // boundary.
      CHECK_EQ(block_size & (kPageSize - 1), 0) << "block_size is not a"
                                                << "multiple of kPageSize";
      first_blocks_[0].mem = reinterpret_cast<char*>(aligned_malloc(block_size_,
                                                                    kPageSize));
      PCHECK(NULL != first_blocks_[0].mem);
    } else {
      first_blocks_[0].mem = reinterpret_cast<char*>(malloc(block_size_));
    }
  }
  first_blocks_[0].size = block_size_;

  Reset();
}

BaseArena::~BaseArena() {
  FreeBlocks();
  assert(overflow_blocks_ == NULL);    // FreeBlocks() should do that
  for ( int i = first_block_we_own_; i < blocks_alloced_; ++i )
    free(first_blocks_[i].mem);
}

int BaseArena::block_count() const {
  return (blocks_alloced_ +
          (overflow_blocks_ ? static_cast<int>(overflow_blocks_->size()) : 0));
}

void BaseArena::Reset() {
  FreeBlocks();
  freestart_ = first_blocks_[0].mem;
  remaining_ = first_blocks_[0].size;
  last_alloc_ = NULL;

  ARENASET(status_.bytes_allocated_ = block_size_);

  const int overage = reinterpret_cast<uintptr_t>(freestart_) &
                      (kDefaultAlignment-1);
  if (overage > 0) {
    const int waste = kDefaultAlignment - overage;
    freestart_ += waste;
    remaining_ -= waste;
  }
  freestart_when_empty_ = freestart_;
  assert(!(reinterpret_cast<uintptr_t>(freestart_)&(kDefaultAlignment-1)));
}

void BaseArena::MakeNewBlock() {
  AllocatedBlock *block = AllocNewBlock(block_size_);
  freestart_ = block->mem;
  remaining_ = block->size;
}

BaseArena::AllocatedBlock*  BaseArena::AllocNewBlock(const size_t block_size) {
  AllocatedBlock *block;
  if ( blocks_alloced_ < ARRAYSIZE(first_blocks_) ) {
    block = &first_blocks_[blocks_alloced_++];
  } else {                   // oops, out of space, move to the vector
    if (overflow_blocks_ == NULL) overflow_blocks_ = new vector<AllocatedBlock>;
    overflow_blocks_->resize(overflow_blocks_->size()+1);
    block = &overflow_blocks_->back();
  }

  if (page_aligned_) {
    size_t num_pages = ((block_size - 1) / kPageSize) + 1;
    size_t new_block_size = num_pages * kPageSize;
    block->mem = reinterpret_cast<char*>(aligned_malloc(new_block_size,
                                                        kPageSize));
    PCHECK(NULL != block->mem);
    block->size = new_block_size;
  } else {
    block->mem = reinterpret_cast<char*>(malloc(block_size));
    block->size = block_size;
  }

  ARENASET(status_.bytes_allocated_ += block_size);

  return block;
}

const BaseArena::AllocatedBlock *BaseArena::IndexToBlock(int index) const {
  if (index < ARRAYSIZE(first_blocks_)) {
    return &first_blocks_[index];
  }
  CHECK(overflow_blocks_ != NULL);
  int index_in_overflow_blocks = index - ARRAYSIZE(first_blocks_);
  CHECK_GE(index_in_overflow_blocks, 0);
  CHECK_LT(static_cast<size_t>(index_in_overflow_blocks),
           overflow_blocks_->size());
  return &(*overflow_blocks_)[index_in_overflow_blocks];
}

void* BaseArena::GetMemoryFallback(const size_t size, const int align_as_int) {
  if (0 == size) {
    return NULL;             // stl/stl_alloc.h says this is okay
  }
  const size_t align = static_cast<size_t>(align_as_int);

  assert(align_as_int > 0 && 0 == (align & (align - 1))); // must be power of 2
  if (block_size_ == 0 || size > block_size_/4) {
    assert(align <= kDefaultAlignment);   // because that's what new gives us
    return AllocNewBlock(size)->mem;
  }

  const size_t overage =
    (reinterpret_cast<uintptr_t>(freestart_) & (align-1));
  if (overage) {
    const size_t waste = align - overage;
    freestart_ += waste;
    if (waste < remaining_) {
      remaining_ -= waste;
    } else {
      remaining_ = 0;
    }
  }
  if (size > remaining_) {
    MakeNewBlock();
  }
  remaining_ -= size;
  last_alloc_ = freestart_;
  freestart_ += size;
  assert(0 == (reinterpret_cast<uintptr_t>(last_alloc_) & (align-1)));
  return reinterpret_cast<void*>(last_alloc_);
}

void BaseArena::FreeBlocks() {
  for ( int i = 1; i < blocks_alloced_; ++i ) {  // keep first block alloced
    free(first_blocks_[i].mem);
    first_blocks_[i].mem = NULL;
    first_blocks_[i].size = 0;
  }
  blocks_alloced_ = 1;
  if (overflow_blocks_ != NULL) {
    vector<AllocatedBlock>::iterator it;
    for (it = overflow_blocks_->begin(); it != overflow_blocks_->end(); ++it) {
      free(it->mem);
    }
    delete overflow_blocks_;             // These should be used very rarely
    overflow_blocks_ = NULL;
  }
}

bool BaseArena::AdjustLastAlloc(void *last_alloc, const size_t newsize) {
  if (last_alloc == NULL || last_alloc != last_alloc_)  return false;
  assert(freestart_ >= last_alloc_ && freestart_ <= last_alloc_ + block_size_);
  assert(remaining_ >= 0);   // should be: it's a size_t!
  if (newsize > (freestart_ - last_alloc_) + remaining_)
    return false;  // not enough room, even after we get back last_alloc_ space
  const char* old_freestart = freestart_;   // where last alloc used to end
  freestart_ = last_alloc_ + newsize;       // where last alloc ends now
  remaining_ -= (freestart_ - old_freestart); // how much new space we've taken
  return true;
}

void* BaseArena::GetMemoryWithHandle(
    const size_t size, BaseArena::Handle* handle) {
  CHECK(handle != NULL);
  void* p = GetMemory(size, (1 << handle_alignment_bits_));
  int block_index;
  const AllocatedBlock* block = NULL;
  for (block_index = block_count() - 1; block_index >= 0; --block_index) {
    block = IndexToBlock(block_index);
    if ((p >= block->mem) && (p < (block->mem + block->size))) {
      break;
    }
  }
  CHECK_GE(block_index, 0) << "Failed to find block that was allocated from";
  CHECK(block != NULL) << "Failed to find block that was allocated from";
  const uint64_t offset = reinterpret_cast<char*>(p) - block->mem;
  DCHECK_LT(offset, block_size_);
  DCHECK((offset & ((1 << handle_alignment_bits_) - 1)) == 0);
  DCHECK((block_size_ & ((1 << handle_alignment_bits_) - 1)) == 0);
  uint64_t handle_value =
      ((static_cast<uint64_t>(block_index) << block_size_bits_) + offset) >>
      handle_alignment_bits_;
  if (handle_value >= static_cast<uint64_t>(0xFFFFFFFF)) {
    // We ran out of space to be able to return a handle, so return an invalid
    // handle.
    handle_value = Handle::kInvalidValue;
  }
  handle->handle_ = static_cast<uint32_t>(handle_value);
  return p;
}

void BaseArena::set_handle_alignment(int align) {
  CHECK(align > 0 && 0 == (align & (align - 1)));  // must be power of 2
  CHECK(static_cast<size_t>(align) < block_size_);
  CHECK((block_size_ % align) == 0);
  CHECK(is_empty());
  handle_alignment_ = align;
  handle_alignment_bits_ = 0;
  while ((1 << handle_alignment_bits_) < handle_alignment_) {
    ++handle_alignment_bits_;
  }
}

void* BaseArena::HandleToPointer(const Handle& h) const {
  CHECK(h.valid());
  uint64_t handle = static_cast<uint64_t>(h.handle_) << handle_alignment_bits_;
  int block_index = static_cast<int>(handle >> block_size_bits_);
  size_t block_offset =
      static_cast<size_t>(handle & ((1 << block_size_bits_) - 1));
  const AllocatedBlock* block = IndexToBlock(block_index);
  CHECK(block != NULL);
  return reinterpret_cast<void*>(block->mem + block_offset);
}

char* UnsafeArena::Realloc(char* s, size_t oldsize, size_t newsize) {
  assert(oldsize >= 0 && newsize >= 0);
  if ( AdjustLastAlloc(s, newsize) )             // in case s was last alloc
    return s;
  if ( newsize <= oldsize ) {
    return s;  // no need to do anything; we're ain't reclaiming any memory!
  }
  char * newstr = Alloc(newsize);
  memcpy(newstr, s, min(oldsize, newsize));
  return newstr;
}

char* SafeArena::Realloc(char* s, size_t oldsize, size_t newsize) {
  assert(oldsize >= 0 && newsize >= 0);
  { LockGuard<Mutex> lock(&mutex_);
    if ( AdjustLastAlloc(s, newsize) )           // in case s was last alloc
      return s;
  }
  if ( newsize <= oldsize ) {
    return s;  // no need to do anything; we're ain't reclaiming any memory!
  }
  char * newstr = Alloc(newsize);
  memcpy(newstr, s, min(oldsize, newsize));
  return newstr;
}

}
