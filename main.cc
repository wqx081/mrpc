#include <iostream>
//#include <memory>

#include "base/thread.h"
#include "base/scoped_ptr.h"

using namespace mrpc;

namespace mrpc {

class SimpleThread : public Thread {
 public:
  SimpleThread() : Thread(Thread::DefaultOptions()) {}
  virtual void Run() override {
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
      sum += i;
    }  
    std::cout << "SimpleThread::Run(): " << sum << std::endl;
  }

}; 

} // namespace mrpc

int main() { 
  scoped_ptr<Thread> thread(new SimpleThread);
  thread->Start();
  std::cout << "Current Id: " << Thread::CurrentId() << std::endl;
  //Thread::Sleep(TimeDelta::FromSeconds(30));
  thread->Join();
  return 0; 
}
