
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


//// ============================   forward declarations for helper funcs ==========================

void * threadFlow(void * arg);
void threadReduce(ThreadContext * tc);
bool areEqualK2(K2 &a, K2 &b);
void check_for_error();


//// ============================ framework functions ==============================================


/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit2 (K2* key, V2* value, void* context){
    ThreadContext * tc = (auto) context;
    IntermediatePair k2_pair = std::pair(&key, &value);
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

    // init semaphore so other threads would wait to it.
    sem_t * sem;  //todo destory at end.
    int semInitValue = sem_init(sem, 0, 0);    //todo check sem initialization.
    check_for_error(semInitValue, "Failed to initialize semaphore.");


    for (int i = 0; i < multiThreadLevel; ++i) {
        IntermediateVec threadIndVec = IntermediateVec();
        threadContexts[i] = {i, threadIndVec, &client, &inputVec, &outputVec,
                             &atomic_counter, sem, &barrier, &shufVec};
    }

    for (int i = 1; i < multiThreadLevel; ++i) {
        pthread_create(threads + i, nullptr, threadFlow, threadContexts + i);
    }

    //main thread should map and sort as well.
    threadFlow(threadContexts);

    //// Shuffle phase

    // init atomic to track output vec
    threadContexts[0].atomic_counter = 0;

    unsigned long numOfRemainingElementsToShuffle = threadContexts[0].inputVec->size();


    // shuffle

    while (true){
        // for each key
        K2 curKeyToMake = {};

        // find max key

        bool allEmptySoFar = true;
        K2 curMax = {};

        // iterate through thread's vectors
        for (int i = 0; i < multiThreadLevel ; ++i) {

            //ensure not empty
            if (!threadContexts[i].threadIndVec.empty()) {

                K2 thisKey = *threadContexts[i].threadIndVec.back().first;

                if(allEmptySoFar){
                    // take max (back)
                    curMax = thisKey;
                    allEmptySoFar= false;

                }else if(curMax<thisKey){
                    // update max if larger
                    curMax = thisKey;
                }

            }
        }

        // if we have not found a non-empty vector - all have been cleared.
        if(allEmptySoFar) break;

        bool NoneEqualSoFar = true;

        // this is our current key
        curKeyToMake = curMax;

        // make a vector of this key
        IntermediateVec curKeyVec;

        while(true) {

            // iterate through thread's vectors
            for (int i = 0; i < multiThreadLevel; ++i) {

                //ensure not empty
                if (!threadContexts[i].threadIndVec.empty()) {

                    K2 thisKey = *threadContexts[i].threadIndVec.back().first;

                    if (areEqualK2(thisKey, curKeyToMake)) {

                        NoneEqualSoFar = false;

                        // add it to our vector
                        curKeyVec.push_back(threadContexts[i].threadIndVec.back());
                        // erase it from the vector
                        threadContexts[i].threadIndVec.pop_back();

                    }

                }
            }

            // if we have found no equal at the back of any vector, we have finished with this key
            if(NoneEqualSoFar) {
                //todo send vector to a thread

                // break, and find the next key.
                break;

            }
        }

    }


        // make



//        // Finding the first not empty thread Intermediate Vector.
//        for (int i = 0; i < multiThreadLevel; i++) {
//            if (!threadContexts[i].threadIndVec.empty()) {
//                keyThreadId = i;
//                break;
//            }
//            //todo N:check if code can reach to the point of the last thread and it's empty.
//        }
//
//        auto key = threadContexts[keyThreadId].threadIndVec.back().first;
//        IntermediateVec currentKeyIndVec = IntermediateVec();
//
//        //todo need to fix the finding equals.
//
//        for (int i = keyThreadId; i < multiThreadLevel; i++) {
//            // Popping matching k2pairs from all thread's vectors.
//
//            while (areEqualK2(threadContexts[i].threadIndVec.back().first, key)) {
//                // Popping matching k2 pairs of current thread with tid i
//
//                currentKeyIndVec.push_back((threadContexts[i].threadIndVec.back()));
//                threadContexts[i].threadIndVec.pop_back();
//                numOfRemainingElementsToShuffle--;
//            }
//        }

        barrier.reducelock();   // blocking the mutex

        // feeding shared vector and increasing semaphore.
        threadContexts[0].shuffleVector->push_back(currentKeyIndVec);
        sem_post(threadContexts[0].semaphore_arg);
        barrier.reduceUnlock();

    }

    // main thread-reduce
    threadReduce(threadContexts);

    //finish
    //todo implement main thread exit? delete object and release memory.

}

////===============================  Helper Functions ==============================================

void * threadFlow(void * arg) {
    ThreadContext * tc = (auto) arg;

    //// Map phase
    bool shouldContinueMapping = true;
    while (shouldContinueMapping) {

        // check atomic for new items to be mapped (k1v1)
        unsigned int old_atom = (*(tc->atomic_counter))++;
        if (old_atom < (tc->inputVec->size())) {

            // calls client map func with pair at old_atom.
            tc->client->map(tc->inputVec->at(old_atom).first,
                            tc->inputVec->at(old_atom).second, tc);
        } else {
            //done parsing the input vector.
            shouldContinueMapping = false;    // todo maybe change to break?
        }
    }

    //// Sort phase
    if (!tc->threadIndVec.empty()) {
        std::sort(tc->threadIndVec.begin(), tc->threadIndVec.end());
    }

    // todo check if we can use the provided Barrier class
    // setting thread to wait at barrier.
    tc->barrier->barrier();

    // if not main thread, wait for semaphore to reduce
    if (tc->threadID != 0) {
        sem_wait(tc->semaphore_arg);
        threadReduce(tc);
    }
    // main thread (ID==0) continues without waiting to shuffle.
}

void threadReduce(ThreadContext * tc) {

    //reducing
    bool keepReduce = true;
    while (keepReduce) {        //todo check properly
        tc->barrier->reducelock();

        //todo create independent output mutex lock.
        int old_atom = (*(tc->atomic_counter))++;
        if (old_atom < (tc->inputVec->size())) {
            std::vector * pairs = &(tc->shuffleVector->back());
            tc->shuffleVector->pop_back();
            tc->client->reduce(pairs, tc);
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

bool areEqualK2(K2& a, K2& b){

  // neither a<b nor b<a means a==b
  return !((a<b)||(b<a));
}

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