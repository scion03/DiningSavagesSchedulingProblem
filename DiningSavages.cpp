
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <atomic>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>

using namespace std;

class semaphore
{
private:
  atomic<int> myValue;
  queue<thread::id> q;
  volatile atomic_flag m;
  mutex forLock;
  mutex mut;
  condition_variable condition;
public:
  semaphore(int v)
  {
    myValue = v;
    if (!v)
    {
      atomic_flag_test_and_set(&m);
    }
  }
  void wait()
  {
    while (atomic_flag_test_and_set(&m))
      ;
    int oldValue = myValue.fetch_sub(1);
    if (oldValue > 0)
      return; // Semaphore was available, return immediately
    else
    {
      // Semaphore was not available, wait until signaled
      std::unique_lock<mutex> lock(mut);
      q.push(std::this_thread::get_id());
      condition.wait(lock, [this]()
                     { return q.front() == this_thread::get_id(); });
      q.pop();
    }
  }
  void signal()
  {
    int oldValue = myValue.fetch_add(1);
    atomic_flag_clear(&m);
    if (oldValue >= 0)
    {
      return;
      // No threads were waiting, return immediately
    }
    else
    {
      // Wake up the first waiting thread
      std::unique_lock<mutex> lock(mut);
      condition.notify_one();
    }
  }
};


signed main()
{
 
  return 0;
}

