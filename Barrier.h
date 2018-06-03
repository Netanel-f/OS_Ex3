#ifndef BARRIER_H
#define BARRIER_H
#include <pthread.h>

// a multiple use barrier

class Barrier {
public:
	Barrier(int numThreads);
	~Barrier();
	void barrier();
	void shufflelock();
    void shuffleUnlock();
    void reducelock();
	void reduceUnlock();

private:
	pthread_mutex_t mutex;
	pthread_cond_t cv;
    pthread_mutex_t shuffleMutex;
    pthread_cond_t shuffleCv;
    pthread_mutex_t reduceMutex;
	pthread_cond_t reduceCv;
	int count;
	int numThreads;
};

#endif //BARRIER_H
