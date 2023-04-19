
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
semaphore pot_mutex(1), pot1(1), pot_full_condn(0), pot_empty_condn(1);
int NumberOfSavages = 10;
int servingsPerPot = 20;
int remainingServings = 0;
void *savage_function(void *arg)
{
  int threadID = *((int *)arg);
  while (true)
  {
    pot_mutex.wait();
    while (remainingServings == 0)
    {
      printf("Savage %d is waiting for more food\n", threadID);
      pot_empty_condn.signal();
      pot_full_condn.wait();
    }
    remainingServings--;
    printf("Savage %d took a serving, %d servings left\n", threadID, remainingServings);
    pot_mutex.signal();
    sleep(3); // time taken to eat!
  }
}
void *cook_function(void *arg)
{
  while (true)
  {
    while (remainingServings != 0)
      pot_empty_condn.wait();
    remainingServings = servingsPerPot;
    printf("Cook refilled the pot, %d servings available\n", remainingServings);
    pot_full_condn.signal();
  }
}

signed main()
{
  pthread_t savage_threads[NumberOfSavages];
  pthread_t cook_thread;
  int savageThreadIDs[NumberOfSavages];
  for (int i = 0; i < NumberOfSavages; i++)
  {
    savageThreadIDs[i] = i;
    pthread_create(&savage_threads[i], NULL, savage_function, &savageThreadIDs[i]);
  }
  pthread_create(&cook_thread, NULL, cook_function, NULL);
  for (int i = 0; i < NumberOfSavages; i++)
  {
    pthread_join(savage_threads[i], NULL);
  }
  pthread_join(cook_thread, NULL);

  return 0;
}

