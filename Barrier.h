#ifndef BARRIER_H
#define BARRIER_H
#include <pthread.h>

// a multiple use barrier

class Barrier {
 public:
  Barrier(int numThreads);
  ~Barrier();
  void barrier();
  void shuffleLock();
  void shuffleUnlock();
  void reduceLock(); //todo check?
  void reduceUnlock(); //todo check?
  void threadsVecsLock();
  void threadsVecsUnlock();

 private:
  pthread_mutex_t mutex;
  pthread_cond_t cv;
  pthread_mutex_t shuffleMutex;
  pthread_mutex_t reduceMutex;
  pthread_mutex_t tvMutex;
  int count;
  int numThreads;
};

#endif //BARRIER_H
