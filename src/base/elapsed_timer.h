#ifndef MRPC_BASE_ELAPSED_TIMER_H_
#define MRPC_BASE_ELAPSED_TIMER_H_

#include "base/macros.h"
#include "base/time.h"

namespace mrpc {

class ElapsedTimer final {
 public:
  void Start() {
    DCHECK(!IsStarted());
    start_ticks_ = Now();
    DCHECK(IsStarted());
  }

  void Stop() {
    DCHECK(IsStarted()); 
    start_ticks_ = TimeTicks();
    DCHECK(!IsStarted());
  }

  bool IsStarted() const {
    return !start_ticks_.IsNull();
  }

  TimeDelta Restart() {
    DCHECK(IsStarted());
    TimeTicks ticks = Now();
    TimeDelta elapsed = ticks - start_ticks_;
    DCHECK(elapsed.InMicroseconds() >= 0);
    start_ticks_ = ticks;
    DCHECK(IsStarted());
    return elapsed;
  }

  TimeDelta Elapsed() const {
    DCHECK(IsStarted());
    TimeDelta elapsed = Now() - start_ticks_;
    DCHECK(elapsed.InMicroseconds() >= 0);
    return elapsed;
  }

  bool HasExpired(TimeDelta time_delta) const {
    DCHECK(IsStarted());
    return Elapsed() >= time_delta;
  }

 private:
  static TimeTicks Now() {
    TimeTicks now = TimeTicks::HighResolutionNow();
    DCHECK(!now.IsNull());
    return now;
  }

  TimeTicks start_ticks_;
};

} // namespace mrpc
#endif // MRPC_BASE_ELAPSED_TIMER_H_
