
#include <atomic>
#include <algorithm>
#include <iostream>
#include <semaphore.h>

#include "MapReduceClient.h"
#include "MapReduceFramework.h"
#include "Barrier.h"


//// ============================   defines and const ==============================================


//// ===========================   typedefs & structs ==============================================

struct ThreadContext {
    // unique fields
    int threadID;
    IntermediateVec* threadIndVec;

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
void check_for_error(int & returnVal, const std::string &message);


//// ============================ framework functions ==============================================


/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit2 (K2* key, V2* value, void* context){
    auto tc = (ThreadContext *) context;
    IntermediatePair k2_pair = std::make_pair(key, value);
    tc->threadIndVec->push_back(k2_pair);    // todo check mem-leaks.
}

/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit3 (K3* key, V3* value, void* context){
    OutputPair k3_pair = std::make_pair(key, value);
    auto * tc = (ThreadContext *) context;
    tc->barrier->reduceLock();
    tc->outputVec->push_back(k3_pair);
    tc->barrier->reduceUnlock();
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
    sem_t * sem;
    int semInitValue = sem_init(sem, 0, 0);    //todo check sem initialization.
    check_for_error(semInitValue, "Failed to initialize semaphore.");


    for (int i = 0; i < multiThreadLevel; ++i) {
        IntermediateVec threadIndVec = IntermediateVec();
        threadContexts[i] = {i, &threadIndVec, &client, &inputVec, &outputVec,
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

    while (true){
        // for each key
        K2 *curKeyToMake;

        // find max key

        bool allEmptySoFar = true;
        K2 *curMax = {};

        // iterate through thread's vectors
        for (int i = 0; i < multiThreadLevel ; ++i) {

            //ensure not empty
            if (!threadContexts[i].threadIndVec->empty()) {

                K2 *thisKey = threadContexts[i].threadIndVec->back().first;

                if(allEmptySoFar){
                    // take max (back)
                    curMax = thisKey;
                    allEmptySoFar= false;

                }else if(*curMax<*thisKey){
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
                if (!threadContexts[i].threadIndVec->empty()) {

                    K2 *thisKey = threadContexts[i].threadIndVec->back().first;

                    if (areEqualK2(*thisKey, *curKeyToMake)) {

                        NoneEqualSoFar = false;

                        // add it to our vector
                        curKeyVec.push_back( ( threadContexts[i].threadIndVec->back() ) );

                        // erase it from the vector
                        threadContexts[i].threadIndVec->pop_back();

                    }
                }
            }

            // if we have found no equal at the back of any vector, we have finished with this key
            if(NoneEqualSoFar) {
                //todo send vector to a thread

                barrier.shuffleLock();   // blocking the mutex

                // feeding shared vector and increasing semaphore.
                threadContexts[0].shuffleVector->push_back(curKeyVec); //todo J why zero?

                sem_post(threadContexts[0].semaphore_arg);  //todo J why zero?

                barrier.shuffleUnlock(); // unblock mutex

                // break, and find the next key.
                break;
            }
        }

    }

    // main thread-reduce
    threadReduce(threadContexts);

    //finish
    //todo implement main thread exit? delete object and release memory.

}

////===============================  Helper Functions ==============================================

void * threadFlow(void * arg) {
    auto tc = (ThreadContext *) arg;

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
    if (!tc->threadIndVec->empty()) {
        std::sort(tc->threadIndVec->begin(), tc->threadIndVec->end());
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
    //todo return
}

void threadReduce(ThreadContext * tc) {

    //reducing
    bool shouldContinueReducing = true;
    while (shouldContinueReducing) {        //todo check properly
        tc->barrier->shuffleLock();

        unsigned int old_atom = (*(tc->atomic_counter))++;
        if (old_atom < (tc->inputVec->size())) {
            std::vector<IntermediatePair> * pairs = &(tc->shuffleVector->back());
            tc->shuffleVector->pop_back();
            tc->client->reduce(pairs, tc);
        } else {
            shouldContinueReducing = false;
        }
        tc->barrier->shuffleUnlock();
    }
    if (tc->threadID != 0) { pthread_exit(0); }
}


bool areEqualK2(K2& a, K2& b){

    // neither a<b nor b<a means a==b
    return !((a<b)||(b<a));
}

void exitFramework(ThreadContext * tc) {
    delete (tc->barrier);
    sem_destroy(tc->semaphore_arg);
    //todo verify with valgrind
}
////=================================  Error Function ==============================================

void check_for_error(int & returnVal, const std::string &message) {

    if (returnVal == 0) return;

    // set prefix
    std::string title = "Library error: ";

    // print error message with prefix
    std::cerr << title << message << "\n";

    // exit
    exit(1);  // todo is this what we want for errors?

}