#include "base/mutex.h"
#include <errno.h>

namespace mrpc {

static void InitializeNativeHandle(pthread_mutex_t* mutex) {
  int result;
  result = pthread_mutex_init(mutex, nullptr);
  DCHECK_EQ(0, result);
}

static void InitializeRecursiveNativeHandle(pthread_mutex_t* mutex) {
  pthread_mutexattr_t attr;
  int result = pthread_mutexattr_init(&attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  DCHECK_EQ(0, result);
  result = pthread_mutex_init(mutex, &attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_destroy(&attr);
  DCHECK_EQ(0, result);
}

static void DestroyNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_destroy(mutex);
  DCHECK_EQ(0, result);
}

static void LockNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_lock(mutex);
  DCHECK_EQ(0, result);
}

static void UnlockNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_unlock(mutex);
  DCHECK_EQ(0, result);
}

static bool TryLockNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_trylock(mutex);
  if (result == EBUSY) {
    return false;
  }
  DCHECK_EQ(0, result);
  return true;
}


//////////////////////////////////////
//
Mutex::Mutex() {
  InitializeNativeHandle(&native_handle_);
}

Mutex::~Mutex() {
  DestroyNativeHandle(&native_handle_);
}

void Mutex::Lock() {
  LockNativeHandle(&native_handle_);
}

void Mutex::Unlock() {
  UnlockNativeHandle(&native_handle_);
}

bool Mutex::TryLock() {
  if (!TryLockNativeHandle(&native_handle_)) {
    return false;
  }
  return true;
}

// RecursiveMutex
RecursiveMutex::RecursiveMutex() {
  InitializeRecursiveNativeHandle(&native_handle_);
}

RecursiveMutex::~RecursiveMutex() {
  DestroyNativeHandle(&native_handle_);
}

void RecursiveMutex::Lock() {
  LockNativeHandle(&native_handle_);
}

void RecursiveMutex::Unlock() {
  UnlockNativeHandle(&native_handle_);
}

bool RecursiveMutex::TryLock() {
  if (!TryLockNativeHandle(&native_handle_)) {
    return false;
  }
  return true;
}

} // namespace mrpc
