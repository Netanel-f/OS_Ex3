#include "Barrier.h"
#include <cstdlib>
#include <cstdio>

Barrier::Barrier(int numThreads)
    : mutex(PTHREAD_MUTEX_INITIALIZER)
    , cv(PTHREAD_COND_INITIALIZER)
    , shuffleMutex(PTHREAD_MUTEX_INITIALIZER)
    , reduceMutex(PTHREAD_MUTEX_INITIALIZER)
    , tvMutex(PTHREAD_MUTEX_INITIALIZER)
    , count(0)
    , numThreads(numThreads)
{ }

//todo maybe can make this all at one array and avoid duplicate code.

Barrier::~Barrier()
{
	if (pthread_mutex_destroy(&mutex) != 0) {
		fprintf(stderr, "[[Barrier]] error on pthread_mutex_destroy");
		exit(1);
	}
	if (pthread_cond_destroy(&cv) != 0){
		fprintf(stderr, "[[Barrier]] error on pthread_cond_destroy");
		exit(1);
	}
    if (pthread_mutex_destroy(&shuffleMutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_destroy");
        exit(1);
    }
    if (pthread_mutex_destroy(&reduceMutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_destroy");
        exit(1);
    }
    if (pthread_mutex_destroy(&tvMutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_destroy");
        exit(1);
    }

}


void Barrier::barrier()
{
	if (pthread_mutex_lock(&mutex) != 0){
		fprintf(stderr, "[[Barrier]] error on pthread_mutex_lock");
		exit(1);
	}
	if (++count < numThreads) {
		if (pthread_cond_wait(&cv, &mutex) != 0){
			fprintf(stderr, "[[Barrier]] error on pthread_cond_wait");
			exit(1);
		}
	} else {
		count = 0;
		if (pthread_cond_broadcast(&cv) != 0) { //todo change from broadcast to signal??
			fprintf(stderr, "[[Barrier]] error on pthread_cond_broadcast");
			exit(1);
		}
	}
	if (pthread_mutex_unlock(&mutex) != 0) {
		fprintf(stderr, "[[Barrier]] error on pthread_mutex_unlock");
		exit(1);
	}
}

void Barrier::shuffleLock() {
    if (pthread_mutex_lock(&shuffleMutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_lock");
        exit(1);
    }
}

void Barrier::shuffleUnlock() {
    if (pthread_mutex_unlock(&shuffleMutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_unlock");
        exit(1);
    }
}

//todo del those ? or seperate shuffle and lock mutex
void Barrier::reduceLock() {
    if (pthread_mutex_lock(&reduceMutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_lock");
        exit(1);
    }
}

void Barrier::reduceUnlock() {
    if (pthread_mutex_unlock(&reduceMutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_unlock");
        exit(1);
    }
}
void Barrier::threadsVecsLock() {
    if (pthread_mutex_lock(&tvMutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_lock");
        exit(1);
    }
}

void Barrier::threadsVecsUnlock() {
    if (pthread_mutex_unlock(&tvMutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_unlock");
        exit(1);
    }
}
