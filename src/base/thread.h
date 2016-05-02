#ifndef MRPC_BASE_THREAD_H_
#define MRPC_BASE_THREAD_H_

#include "base/macros.h"
#include "base/time.h"
#include "base/mutex.h"
#include "base/semaphore.h"



namespace mrpc {

class Thread {
 public:
  typedef pthread_t ThreadId;

  class Options {
   public:
    Options() : name_("mrpc:<unknown>"), 
	        stack_size_(0), 
		joinable_(true) {}

    explicit Options(const char* name, 
		     int stack_size = 0,
		     bool joinable = true)
      : name_(name),
	stack_size_(stack_size),
	joinable_(joinable) {}

    const char* name() const { return name_; }
    int stack_size() const { return stack_size_; }
    bool joinable() const { return joinable_; }
    //void set_joinable(bool joinable) { joinable_ = joinable; }
    //void set_detached(bool )
    void EnableJoinable() { joinable_ = true; }
    void EnableDetached() { joinable_ = false; }

   private:
    const char* name_;
    int stack_size_;
    bool joinable_;
  };
  
  // Create new thread.
  explicit Thread(const Options& options);
  virtual ~Thread();

  void Start();
  void Join();
  bool IsJoinable() { return joinable_; }
  bool IsDetached() { return !IsJoinable(); }

  const char* name() const {
    return name_;
  }

  virtual void Run() = 0;

  static ThreadId CurrentId();
  static void Sleep(TimeDelta duration);
  static void YieldCurrentThread();
  static Options DefaultOptions() { return Options(); }

  static const int kMaxThreadNameLength = 16; 

  class PlatformData;
  PlatformData* data() { return data_; }

  void NotifyStartedAndRun() {
    if (start_semaphore_) {
      start_semaphore_->Signal();
    }
    Run();
  }

 private:
  void set_name(const char* name);

  PlatformData* data_;

  char name_[kMaxThreadNameLength];
  int stack_size_;
  bool joinable_;
  Semaphore* start_semaphore_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

} // namespace mrpc
#endif // MRPC_BASE_THREAD_H_
