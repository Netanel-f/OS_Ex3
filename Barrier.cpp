#include "Barrier.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>

Barrier::Barrier(int numThreads)
    : mutex(PTHREAD_MUTEX_INITIALIZER),
      cv(PTHREAD_COND_INITIALIZER),
      shuffleMutex(PTHREAD_MUTEX_INITIALIZER),
      reduceMutex(PTHREAD_MUTEX_INITIALIZER),
      tvMutex(PTHREAD_MUTEX_INITIALIZER),
      count(0),
      numThreads(numThreads) {}

//todo maybe can make this all at one array and avoid duplicate code.

// forward declaration.
void BarrierErrCheck(int &returnVal, const std::string &message);

Barrier::~Barrier() {
    int retVal;

    retVal = pthread_mutex_destroy(&mutex);
    BarrierErrCheck(retVal, "pthread_mutex_destroy");

    retVal = pthread_cond_destroy(&cv);
    BarrierErrCheck(retVal, "pthread_cond_destroy");

    retVal = pthread_mutex_destroy(&shuffleMutex);
    BarrierErrCheck(retVal, "pthread_mutex_destroy");

    retVal = pthread_mutex_destroy(&reduceMutex);
    BarrierErrCheck(retVal, "pthread_mutex_destroy");

    retVal = pthread_mutex_destroy(&tvMutex);
    BarrierErrCheck(retVal, "pthread_mutex_destroy");
}

void Barrier::barrier() {
    int retVal;

    retVal = pthread_mutex_lock(&mutex);
    BarrierErrCheck(retVal, "pthread_mutex_lock");

    if (++count < numThreads) {
        retVal = pthread_cond_wait(&cv, &mutex);
        BarrierErrCheck(retVal, "pthread_cond_wait");
    } else {
        count = 0;
        retVal = pthread_cond_broadcast(&cv);
        BarrierErrCheck(retVal, "pthread_cond_broadcast"); //todo change from broadcast to signal??
    }
    retVal = pthread_mutex_unlock(&mutex);
    BarrierErrCheck(retVal, "pthread_mutex_unlock");
}

void Barrier::shuffleLock() {
    int retVal;
    retVal = pthread_mutex_lock(&shuffleMutex);
    BarrierErrCheck(retVal, "pthread_mutex_lock");
}

void Barrier::shuffleUnlock() {
    int retVal;
    retVal = pthread_mutex_unlock(&shuffleMutex);
    BarrierErrCheck(retVal, "pthread_mutex_unlock");
}

//todo del those ? or separate shuffle and lock mutex
void Barrier::reduceLock() {
    int retVal;
    retVal = pthread_mutex_lock(&reduceMutex);
    BarrierErrCheck(retVal, "pthread_mutex_lock");
}

void Barrier::reduceUnlock() {
    int retVal;
    retVal = pthread_mutex_unlock(&reduceMutex);
    BarrierErrCheck(retVal, "pthread_mutex_unlock");
}
void Barrier::threadsVecsLock() {
    int retVal;
    retVal = pthread_mutex_lock(&tvMutex);
    BarrierErrCheck(retVal, "pthread_mutex_lock");
}

void Barrier::threadsVecsUnlock() {
    int retVal;
    retVal = pthread_mutex_unlock(&tvMutex);
    BarrierErrCheck(retVal, "pthread_mutex_unlock");
}

////=================================  Error Function ==============================================

/**
 * Checks for failure of library functions, and handling them when they occur.
 */
void BarrierErrCheck(int &returnVal, const std::string &message) {

    // if no failure, return
    if (returnVal == 0) return;

    // set prefix
    std::string prefix = "[[Barrier]] error on ";

    // print error message with prefix
    std::cerr << prefix << message << "\n";

    // exit
    exit(1);

}