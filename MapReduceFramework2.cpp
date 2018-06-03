
#include <atomic>
#include <algorithm>
#include <iostream>
#include <semaphore.h>
#include "MapReduceClient.h"
#include "MapReduceFramework.h"
#include "Barrier.h"

//todo maybe implement data struct as class (instance created for each call to framework?)


//// ============================   defines and const ==============================================


//// ===========================   typedefs & structs ==============================================

struct ThreadContext {
    // unique fields
    int threadID;
    IntermediateVec& threadIndVec;

    // shared data among threads
    const MapReduceClient* client;
    const InputVec* inputVec;
    OutputVec* outputVec;
    std::atomic<unsigned int>* atomic_counter;
    sem_t * semaphore_arg;
    Barrier* barrier;
    std::vector<IntermediateVec> * shuffleVector;
};

//todo struct holding atomic and semaphore

//// ============================   forward declarations for helper funcs ==========================

void threadFlow();
void shuffle();
bool areEqualK2(K2 *a, K2 *b);
void check_for_error();


//// ============================ framework functions ==============================================


/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit2 (K2* key, V2* value, void* context){
    IntermediatePair k2_pair = std::pair(&key, &value);
    auto * tc = (ThreadContext *) context;
    tc->threadIndVec.push_back(k2_pair);    // todo check mem-leaks.
}

/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit3 (K3* key, V3* value, void* context){
    OutputPair k3_pair = std::pair(&key, &value);
    auto * tc = (ThreadContext *) context;
//    tc->outputVec.push_back(k3_pair); //todo fix
    tc->outputVec->push_back(k3_pair);
}

//todo check for error return handles - maybe i miss-handled some of them

/**
 *
 * @param client
 * @param inputVec
 * @param outputVec
 * @param multiThreadLevel
 */
void runMapReduceFramework(const MapReduceClient& client, const InputVec& inputVec,
                           OutputVec& outputVec, int multiThreadLevel) {

    pthread_t threads[multiThreadLevel];
    ThreadContext threadContexts[multiThreadLevel];
    Barrier barrier(multiThreadLevel);
    std::atomic<unsigned int> atomic_counter(0);
    std::vector<IntermediateVec> shufVec = std::vector<IntermediateVec>();
    //init semaphore so other threads would wait to it.
    sem_t * sem;  //todo destory at end.
    int semInitValue = sem_init(sem, 0, 0);    //todo check sem initialization.
    check_for_error(semInitValue, "Failed to initialize semaphore.");


    for (int i = 0; i < multiThreadLevel; ++i) {
        IntermediateVec threadIndVec = IntermediateVec();
        threadContexts[i] = {i, threadIndVec, &client, &inputVec, &outputVec,
                             &atomic_counter, sem, &barrier, &shufVec};
    }

    for (int i = 1; i < multiThreadLevel; ++i) {
        pthread_create(threads + i, NULL, threadFlow, threadContexts + i);
    }


    //main thread should map and sort as well.
    threadFlow(threadContexts);

    //threadContexts[0].barrier->barrier(); //Happens on threadFlow. letting now main thread arrived at the barrier

    // init atomic to track output vec
    threadContexts[0].atomic_counter = 0;

    //shuffle
    int numOfNonEmptyThreadVec = multiThreadLevel;
    int shuffleCounter = 0;

    while (shuffleCounter < threadContexts->inputVec->size()) {
        int keyThreadId = 0;
        auto key;
        IntermediateVec currentKeyIndVec;

        // todo can merge those 2 loops.

        //finding the first not empty thread Intermediate Vector.
        for (int i = keyThreadId; i < multiThreadLevel; i++) {
            if (!threadContexts[i].threadIndVec.empty()) {
                keyThreadId = i;
                key = threadContexts[i].threadIndVec.back().first;
                break;
            }
            //todo check if code can reach to the point of the last thread and it's empty.
        }

        currentKeyIndVec = IntermediateVec();

        // popping matching k2pairs from all thread's vectors.
        for (int i = keyThreadId; i < multiThreadLevel; i++) {
            while (areEqualK2(threadContexts[i].threadIndVec.back().first, key)) {
                currentKeyIndVec.push_back(threadContexts[i].threadIndVec.back());
                threadContexts[i].threadIndVec.pop_back();
                shuffleCounter++;
            }
        }

        barrier.reducelock();
        // feeding shared vector and increasing semaphore.
        threadContexts[0].shuffleVector->push_back(currentKeyIndVec);
        sem_post(threadContexts[0].semaphore_arg);
        barrier.reduceUnlock();

    }

    // main thread-reduce
    tReduce(threadContexts);

    //finish
    //todo implement main thread exit? delete object and release memory.

}

////===============================  Helper Functions ==============================================

void * threadFlow(void * arg) {
//    ThreadContext * tc = (ThreadContext*) arg; //todo decide which one is nice.
    ThreadContext * tc = (auto) arg;

    // mapping
    bool keepMap = true;
    while (keepMap) {
        unsigned int old_atom = (*(tc->atomic_counter))++;
        if (old_atom < (tc->inputVec->size())) {
            tc->client->map(tc->inputVec->at(old_atom).first,
                            tc->inputVec->at(old_atom).second, tc);
        } else {
            keepMap = false;
        }
    }

    //sorting
    if (tc->threadIndVec)
    std::sort(tc->threadIndVec.begin(), tc->threadIndVec.end());

    //todo check if we can use the provided Barrier class
    //setting thread to wait at barrier.
    tc->barrier->barrier();

    //waiting to semaphore
    if (tc->threadID != 0) {
        sem_wait(tc->semaphore_arg);
        tReduce(&arg);
    }
}

void * tReduce(void * arg) {
    ThreadContext *tc = (ThreadContext *) arg;

    //reducing
    bool keepReduce = true;
    while (keepReduce) {    //todo check properly
        tc->barrier->reducelock();

        //todo create independent output mutex lock.
        int old_atom = (*(tc->atomic_counter))++;
        if (old_atom < tc->inputVec->size()) {
            tc->client->reduce(tc->shuffleVector->pop_back(), tc);
        } else {
            keepReduce = false;
        }
        tc->barrier->reduceUnlock();
    }
    if (tc->threadID != 0) { pthread_exit(0); }
}


void threadFlowORIG(){

	//MAP

		// check atomic for new items to be mapped (k1v1)
        // pull a pair
		// use emit (with context !) to map the items to this thread's own ind vector

	// SORT

		// sort the items within this threads indvec

	// BARRIER

		// get to the barrier
		// wait for main thread to tell me to keep going


	// Reduce

		// wait for new K2-specific vectors to become available
        // pull a vector
        // use emit (with context !) to map the items to this thread's own ind vector


}

bool areEqualK2(K2* a, K2* b){

  // neither a<b nor b<a means a==b
  return !((a<b)||(b<a));
}
//
//
//void shuffle(data stuff){
//
//
////    std::vector<IntermediateVec> vecVec; //todo N test
////
////    auto key = stuff.indVec.back().first;
////    IntermediatePair &first_pair = stuff.indVec.back();
////    stuff.indVec.pop_back();
////
////    IntermediateVec first_key_vec;
////    first_key_vec.push_back(first_pair);
////    vecVec.push_back(first_key_vec);
////
////    IntermediateVec current_key_indVec;
////
////
////    while (!stuff.indVec.empty()) {
////        auto current = &stuff.indVec.back();
////        stuff.indVec.pop_back();
////
////        if (current->first == key) {
////            vecVec.back().push_back(*current);
////        } else {
////
////            vecVec.emplace_back(current_key_indVec);
////            current_key_indVec.clear();
////            key = current->first;
////            current_key_indVec.push_back(*current);
////        }
////    }
//
//
//  std::vector<IntermediateVec> vecVec;
//
//  auto key = stuff.indVec.back().first;
//  IntermediateVec current_key_indVec;
//
//  while (!stuff.indVec.empty()) {
//    auto current = &stuff.indVec.back();
//    stuff.indVec.pop_back();
//
//    if (areEqualK2(current->first, key)) {
//      current_key_indVec.push_back(*current);
//    } else {
//      vecVec.emplace_back(current_key_indVec);
//      current_key_indVec.clear();
//      key = current->first;
//      current_key_indVec.push_back(*current);
//    }
//  }
//}

////=================================  Error Function ==============================================

void check_for_error(int & returnVal, const std::string &message) {

  if (returnVal == 0) return;

  // set prefix
  std::string title = "Library error: ";

  // print error message with prefix
  std::cerr << title << message << "\n";

  // exit
  exit(1);  // todo is this what we want?

}