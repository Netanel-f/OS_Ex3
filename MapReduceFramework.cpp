
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
//    IntermediateVec * threadVec;

    // shared data among threads
    const MapReduceClient* client;
    const InputVec* inputVec;
    OutputVec* outputVec;
    std::atomic<unsigned int>* atomic_counter;
    sem_t * semaphore_arg;
    Barrier* barrier;
    std::vector<IntermediateVec> * threadsVectors;
    std::vector<IntermediateVec> * shuffleVector;
};


//// ============================   forward declarations for helper funcs ==========================

void * threadFlow(void * arg);
void shuffle(ThreadContext * tc, int multiThreadLevel);
void threadReduce(ThreadContext * tc);
bool areEqualK2(K2 &a, K2 &b);
void check_for_error(int & returnVal, const std::string &message);
void exitFramework(ThreadContext * tc);
void deleteThreadIndVec(ThreadContext * tc);



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
    tc->threadsVectors->at(tc->threadID).push_back(k2_pair);  // todo check mem-leaks.
}

/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit3 (K3* key, V3* value, void* context){
    auto * tc = (ThreadContext *) context;
    OutputPair k3_pair = std::make_pair(key, value);
    tc->outputVec->push_back(k3_pair);
}

//todo check for error return handles - maybe i mishandled some of them

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
    std::vector<IntermediateVec> threadsVectors(multiThreadLevel, IntermediateVec(0));
    std::vector<IntermediateVec> shuffleVector(0);

    // init semaphore so other threads would wait to it.
    sem_t * sem = new sem_t;
    int semInitValue = sem_init(sem, 0, 0);    //todo check sem initialization.
    check_for_error(semInitValue, "Failed to initialize semaphore.");


    for (int i = 0; i < multiThreadLevel; ++i) {
        threadContexts[i] = {i, &client, &inputVec, &outputVec,
                             &atomic_counter, sem, &barrier, &threadsVectors, &shuffleVector};
    }

    for (int i = 1; i < multiThreadLevel; ++i) {
        pthread_create(threads + i, nullptr, threadFlow, threadContexts + i);
    }

    //main thread should map and sort as well.
    threadFlow(threadContexts);

    //// Shuffle phase

    // init atomic to track output vec
    (*threadContexts[0].atomic_counter) = 0;

    shuffle(&threadContexts[0], multiThreadLevel);

    // main thread-reduce
    threadReduce(threadContexts);

    // main thread will wait for all other threads to terminate
    for (int i = 1; i < multiThreadLevel; ++i) {
        // check for error of pthread
        pthread_join(threads[i], NULL);
    }

    //finish

//    for (int i = 0; i < multiThreadLevel; ++i) { //todo J depends if we use new for inthreadvec
//        deleteThreadIndVec(&threadContexts[i]);
//    }

    //todo implement main thread exit? delete object and release memory.
    exitFramework(threadContexts);

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
    if (!tc->threadsVectors->at(tc->threadID).empty()) {
        std::sort(tc->threadsVectors->at(tc->threadID).begin(),
                  tc->threadsVectors->at(tc->threadID).end());
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
    return nullptr; //todo need to check properly
}

void shuffle(ThreadContext * tc, int multiThreadLevel) {
    while (true){
    // for each key
    K2 *curKeyToMake;

    // find max key

    bool allEmptySoFar = true;
    K2 *curMax = {};

    // iterate through thread's vectors, and find max at back
    for (int i = 0; i < multiThreadLevel ; ++i) {

        //ensure not empty
        if (!tc->threadsVectors->at(i).empty()) {

            K2 *thisKey = tc->threadsVectors->at(i).back().first;

            if(allEmptySoFar){
                // take max (back)
                curMax = thisKey;
                allEmptySoFar = false;

            }else if(*curMax<*thisKey){
                // update max if larger
                curMax = thisKey;
            }

        }
    }

    // if we have not found a non-empty vector after iterating - all have been cleared.
    if(allEmptySoFar) break;


    // this is our current key
    curKeyToMake = curMax;
    // make a vector of this key
    IntermediateVec curKeyVec;

    while(true) {

        bool NoneEqualSoFar = true;

        // iterate through thread's vectors
        for (int i = 0; i < multiThreadLevel; ++i) {

            //ensure not empty
            if (!tc->threadsVectors->at(i).empty()) {

                K2 *thisKey = tc->threadsVectors->at(i).back().first;

                if (areEqualK2(*thisKey, *curKeyToMake)) {

                    NoneEqualSoFar = false;

                    // add it to our vector
                    curKeyVec.push_back( ( tc->threadsVectors->at(i).back() ) );

                    // erase it from the vector
                    tc->threadsVectors->at(i).pop_back();

                }
            }
        }

        // if we have found no equal at the back of any vector, we have finished with this key
        if(NoneEqualSoFar) {
            //todo send vector to a thread

            tc->barrier->shuffleLock();   // blocking the mutex

            // feeding shared vector and increasing semaphore.
            tc->shuffleVector->push_back(curKeyVec); //todo J why zero?

            sem_post(tc->semaphore_arg);  //todo J why zero?

            tc->barrier->shuffleUnlock(); // unblock mutex

            // break, and find the next key.
            break;
        }
    }

}
}
//void shuffle(ThreadContext * tc, int multiThreadLevel) {
//
//    bool continueShuffle = true;
//
//    // Looping to create
//    while (continueShuffle) {
//        int maxKeyThreadId = -1;
//        K2 * key = nullptr;
//
//        // Finding the first not empty thread Intermediate Vector.
//        for (int i = 0; i < multiThreadLevel; i++) {
//            if (!tc->threadsVectors->at(i).empty()) {
//                maxKeyThreadId = i;
//                key = tc->threadsVectors->at(i).back().first;
//                break;
//            }
//            //todo N:check if code can reach to the point of the last thread and it's empty.
//        }
//
//        // if didn't find non-empty thread Vector
//        if (maxKeyThreadId == -1) {
//            continueShuffle = false;
//            break;
//        }
//
//        // Finding the max key
//        for (int i = maxKeyThreadId; i < multiThreadLevel; i++) {
//            if (!tc->threadsVectors->at(i).empty() &&
//                !areEqualK2(*key, *tc[i].threadsVectors->at(i).back().first) &&
//                (key < tc->threadsVectors->at(i).back().first)) {
//                    maxKeyThreadId = i;
//                    key = tc->threadsVectors->at(i).back().first;
//            }
//        }
//
//        IntermediateVec currentKeyIndVec(0);
//        for (int i = maxKeyThreadId; i < multiThreadLevel; i++) {
//            // Popping matching k2pairs from all thread's vectors.
//
//            while (!tc->threadsVectors->at(i).empty() &&
//                   areEqualK2(*tc->threadsVectors->at(i).back().first, *key)) {
//                // Popping matching k2 pairs of current thread with tid i
//
//                currentKeyIndVec.push_back((tc->threadsVectors->at(i).back()));
//                tc->threadsVectors->at(i).pop_back();
//            }
//        }
//
//        tc->barrier->shuffleLock();   // blocking the mutex
//
//        // feeding shared vector and increasing semaphore.
//        tc->shuffleVector->emplace_back(currentKeyIndVec);
//        sem_post(tc->semaphore_arg);
//        (*(tc->atomic_counter))++;
//        tc->barrier->shuffleUnlock();
//    }
//}

void threadReduce(ThreadContext * tc) {
//    unsigned int old_atom = (*(tc->atomic_counter))--;

    //reducing
    bool shouldContinueReducing = true;
    while (shouldContinueReducing) {        //todo check properly

//        sem_wait(tc->semaphore_arg);
        unsigned int old_atom = (*(tc->atomic_counter))--;
//        printf("tid %d old atom%d\n", tc->threadID, old_atom); //todo DEBUG Print
        if (tc->threadID == 0 && old_atom < 1) { break; }
        tc->barrier->shuffleLock();



        IntermediateVec * pairs = &(tc->shuffleVector->back());
        tc->client->reduce(pairs, tc);
        tc->shuffleVector->pop_back();

        tc->barrier->shuffleUnlock();
        if (old_atom <= 1) {
            shouldContinueReducing = false;
        }
    }

    if (tc->threadID != 0) {
        printf("tid %d is exited\n", tc->threadID);
        pthread_exit(nullptr);
    }
}


bool areEqualK2(K2& a, K2& b){

    // neither a<b nor b<a means a==b
    return !((a<b)||(b<a));
}

void deleteThreadIndVec(ThreadContext * tc) {
//    delete (tc->threadsVectors->at(tc->threadID));
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