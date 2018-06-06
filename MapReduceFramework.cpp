
#include <atomic>
#include <algorithm>
#include <iostream>
#include <semaphore.h>

#include "MapReduceClient.h"
#include "MapReduceFramework.h"
#include "Barrier.h"


//// ============================   defines and const ==============================================
#define DEBUG false
//todo delete define

//// ===========================   typedefs & structs ==============================================

struct ThreadContext {
  // unique fields
  int threadID;

  // shared data among threads
  const MapReduceClient *client;
  const InputVec *inputVec;
  OutputVec *outputVec;
  bool *stillShuffling;
  std::atomic<unsigned int> *atomic_counter;
  sem_t *semaphore_arg;
  Barrier *barrier;
  std::vector<IntermediateVec> *threadsVectors;
  std::vector<IntermediateVec> *shuffleVector;
};


//// ============================   forward declarations for helper funcs ==========================

// thread entry point.
void *threadFlow(void *arg);

// main stages
void map(ThreadContext *tc);
void sort(ThreadContext *tc);
void shuffle(ThreadContext *tc, int multiThreadLevel);
void reduce(ThreadContext *tc);

// cleanup
void exitFramework(ThreadContext *tc, sem_t *sem);

// K2 helpers
bool areEqualK2(K2 &a, K2 &b);
bool compareKeys(const IntermediatePair &lhs, const IntermediatePair &rhs);

// Success checker
void errCheck(int &returnVal, const std::string &message);



//// ============================ framework functions ==============================================


/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit2(K2 *key, V2 *value, void *context) {
    auto tc = (ThreadContext *) context;
    IntermediatePair k2_pair = std::make_pair(key, value);
    tc->barrier->threadsVecsLock();
    tc->threadsVectors->at(tc->threadID).push_back(k2_pair);
    tc->barrier->threadsVecsUnlock();
}

/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit3(K3 *key, V3 *value, void *context) {
    auto *tc = (ThreadContext *) context;
    OutputPair k3_pair = std::make_pair(key, value);
    tc->outputVec->push_back(k3_pair);
}

//todo check for error return handles - maybe i mishandled some of them

/**
 * Runs the framework according to specified input.
 * @param client - Ref to a client.
 * @param inputVec - Ref to input.
 * @param outputVec - Ref to output.
 * @param multiThreadLevel - Max # of threads to make.
 */
void runMapReduceFramework(const MapReduceClient &client, const InputVec &inputVec,
                           OutputVec &outputVec, int multiThreadLevel) {


    //// -------   Variables -------

    pthread_t threads[multiThreadLevel];
    ThreadContext threadContexts[multiThreadLevel];
    Barrier barrier(multiThreadLevel);
    bool everydayImShuffling = true;
    std::atomic<unsigned int> atomic_counter(0);
    std::vector<IntermediateVec> threadsVectors(multiThreadLevel, IntermediateVec(0));
    std::vector<IntermediateVec> shuffleVector(0);

    //// -------   Initialisation -------

    // init semaphore so other threads would wait to it.
    auto *sem = new sem_t;

    int retVal = sem_init(sem, 0, 0);
    errCheck(retVal, "sem_init");

    // init thread contexts
    for (int i = 0; i < multiThreadLevel; ++i) {
        threadContexts[i] = {i, &client, &inputVec, &outputVec, &everydayImShuffling,
                             &atomic_counter, sem, &barrier, &threadsVectors, &shuffleVector};
    }

    //// -------   Map & sort -------

    // init threads to star at threadFlow
    for (int i = 1; i < multiThreadLevel; ++i) {
        retVal = pthread_create(threads + i, nullptr, threadFlow, threadContexts + i);
        errCheck(retVal, "pthread_create");
    }

    //main thread should map and sort as well.
    threadFlow(threadContexts);

    //// -------   Shuffle & Reduce -------

    // init atomic to track output vec part of treduce method
    (*threadContexts[0].atomic_counter) = 0;

    // start shuffling
    shuffle(&threadContexts[0], multiThreadLevel);

    // main thread should reduce one shuffle is done
    for (int i = 0; i < multiThreadLevel; i++) {
        retVal = sem_post(threadContexts->semaphore_arg);
        errCheck(retVal, "sem_post");
    }

    // main thread should reduce one shuffle is done
    reduce(threadContexts);


    // main thread will wait for all other threads to terminate
    for (int i = 1; i < multiThreadLevel; ++i) {
        // check for error of pthread
        if (DEBUG) { printf("main thread join tid %d \n ", i); }
        retVal = pthread_join(threads[i], nullptr);
        errCheck(retVal, "pthread_join");
    }

    //// -------   Cleanup & Finish -------

    if (DEBUG) {
        printf("size of output vector is %d \n",
               (int) threadContexts->outputVec->size());
    }

    exitFramework(threadContexts, sem);

}

////===============================  Helper Functions ==============================================


/**
 * Handles the flow for a thread.
 * Sends regular threads to reduce, but sends main to shuffle.
 * @param arg - a pointer to a ThreadContext struct.
 * @return null.
 */
void *threadFlow(void *arg) {
    auto tc = (ThreadContext *) arg;
    if (DEBUG) { printf("tid %d is entering threadFlow\n", tc->threadID); }

    //// Map phase
    map(tc);

    //// Sort phase
    sort(tc);

    // setting thread to wait at barrier.
    if (DEBUG) { printf("tid %d is at barrier\n", tc->threadID); }
    tc->barrier->barrier();

    // if not main thread, go to reduce (reduce ends and kills thread)
    if (tc->threadID != 0) {
        reduce(tc);
    }

    // the main thread (ID==0) continues (without waiting) to shuffle.
    return nullptr; //todo N need to check properly
}

/**
 * Performs a shuffle. Called only by main thread.
 * @param tc
 * @param multiThreadLevel
 */
void shuffle(ThreadContext *tc, int multiThreadLevel) {
    int shufCounter = 0;
    bool continueShuffle = true;

    //// shuffling

    while (continueShuffle) {

        //// find max K2

        // flags
        int maxKeyThreadId = -1;
        K2 *key = nullptr;
        int val = -1; // todo J why is val unused?

        // find the first not empty thread Intermediate Vector.
        for (int i = 0; i < multiThreadLevel; i++) {
            if (DEBUG) { printf("~~~~~nonempty i is %d~~~~\n", i); }

            // take the back of this vector.
            if (!tc->threadsVectors->at(i).empty()) {
                maxKeyThreadId = i;
                key = tc->threadsVectors->at(i).back().first;
                break;
            }
            //todo N:check if code can reach to the point of the last thread and it's empty.
        }

        // if we didn't find non-empty thread Vector, we are done
        if (maxKeyThreadId == -1) {
            continueShuffle = false; //todo J is this necessary
            break;
        }

        // Finding the max key
        for (int i = maxKeyThreadId; i < multiThreadLevel; i++) {

            // for each non-empty vector
            if (tc->threadsVectors->at(i).empty()) { continue; }

            if (DEBUG) {
                printf("~~~~~max loop i is %d~~~~\n", i);
                K2 *other = tc->threadsVectors->at(i).back().first;
                bool a = key < tc->threadsVectors->at(i).back().first;
                bool b = key < other;
                bool c = *key < *other;
                printf("a:%d b:%d c:%d", a, b, c);
            }
            // replace max if this key is greater.
            if (!tc->threadsVectors->at(i).empty()
                && (*key) < *(tc->threadsVectors->at(i).back().first)) {
                maxKeyThreadId = i;
                key = tc->threadsVectors->at(i).back().first;
            }
        }

        //// create a vector from all the K2 s.t. K2=max
        IntermediateVec currentKeyIndVec(0);

        // for all vectors
        for (int i = maxKeyThreadId; i < multiThreadLevel; i++) {

            // while this thread's vector is not empty
            while (!tc->threadsVectors->at(i).empty() &&
                areEqualK2(*(tc->threadsVectors->at(i).back().first), *key)) {

                // take back iff the back is equal to max
                currentKeyIndVec.push_back((tc->threadsVectors->at(i).back()));
                tc->barrier->threadsVecsLock();
                tc->threadsVectors->at(i).pop_back();
                tc->barrier->threadsVecsUnlock();
            }
        }

        // blocking the mutex
        tc->barrier->shuffleLock();

        //// send vector to a thread, and wake using semaphore

        // feeding shared vector and increasing semaphore.
        tc->shuffleVector->emplace_back(currentKeyIndVec);
        int ret = (*(tc->atomic_counter))++; // todo J why is ret unused?
        shufCounter++;
        int retVal = sem_post(tc->semaphore_arg);
        errCheck(retVal, "sem_post");
        tc->barrier->shuffleUnlock();
    }

    if (DEBUG) { printf("size of shuffle vector is %d \n", shufCounter); }

    // notify threads shuffling has ended - threads should now move to reduce if semaphore is zero.
    *tc->stillShuffling = false;
}

/**
 * Performs map within a thread context.
 */
void map(ThreadContext *tc) {
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
            shouldContinueMapping = false;
        }
    }
}

/**
 * Performs sort within a thread context.
 */
void sort(ThreadContext *tc) {
    if (!tc->threadsVectors->at(tc->threadID).empty()) {
        std::sort(tc->threadsVectors->at(tc->threadID).begin(),
                  tc->threadsVectors->at(tc->threadID).end(), compareKeys);
    }
}

/**
 * Performs reduce within a thread context.
 */
void reduce(ThreadContext *tc) {
    while (true) {

        // call wait
        int retVal = sem_wait(tc->semaphore_arg);
        errCheck(retVal, "sem_wait");

        int atom = (*(tc->atomic_counter))--;

        tc->barrier->shuffleLock();

        if (atom <= 0) {
            tc->barrier->shuffleUnlock();
            break;
        }

        IntermediateVec *pairs = &(tc->shuffleVector->back());

        tc->client->reduce(pairs, tc);

        tc->shuffleVector->pop_back();

        tc->barrier->shuffleUnlock();

    }
}

/**
 * Simple by-K2 lesser-than comparator for intermediatePair.
 * @return true iff lhs's K2 is smaller.
 */
bool compareKeys(const IntermediatePair &lhs, const IntermediatePair &rhs) {
    return (*lhs.first) < (*rhs.first);
}

/**
 * Simple Equality checker for K2 keys.
 */
bool areEqualK2(K2 &a, K2 &b) {
    // neither a<b nor b<a means a==b
    return !((a < b) || (b < a));
}

/**
 * Cleans up framework before exiting
 */
void exitFramework(ThreadContext *tc, sem_t *sem) {

    int retVal = sem_destroy(tc->semaphore_arg);
    errCheck(retVal, "sem_destroy");

    delete sem;

    if (DEBUG) { printf("exiting framework"); };

    //delete (tc->barrier); // todo J this fails for some reason
    // todo J (later) - because new was not called

    //    todo implement & verify momory leaks with valgrind
}

////=================================  Error Function ==============================================

/**
 * Checks for failure of library functions, and handling them when they occur.
 */
void errCheck(int &returnVal, const std::string &message) {

    // if no failure, return
    if (returnVal == 0) return;

    // set prefix
    std::string prefix = "[[Framework]] error on ";

    // print error message with prefix
    std::cerr << prefix << message << "\n";

    // exit
    exit(1);  // todo J is this what we want for errors? maybe exit framework (pass *tc for access)
    // todo J is this what we want for errors? maybe exit framework (pass *tc for access)

}

