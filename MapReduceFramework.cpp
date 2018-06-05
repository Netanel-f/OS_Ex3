
#include <atomic>
#include <algorithm>
#include <iostream>
#include <semaphore.h>


#include "MapReduceClient.h"
#include "MapReduceFramework.h"
#include "Barrier.h"


//// ============================   defines and const ==============================================
#define DEBUG true
//todo delete define

//// ===========================   typedefs & structs ==============================================

struct ThreadContext {
    // unique fields
    int threadID;

    // shared data among threads
    const MapReduceClient* client;
    const InputVec* inputVec;
    OutputVec* outputVec;
    bool * stillShuffling;
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
//void errorPrint(ThreadContext* tc);



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
    tc->barrier->threadsVecsLock();
    tc->threadsVectors->at(tc->threadID).push_back(k2_pair);  // todo check mem-leaks.
    tc->barrier->threadsVecsUnlock();
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
    bool everydayImShuffling = true;
    std::atomic<unsigned int> atomic_counter(0);
    std::vector<IntermediateVec> threadsVectors(multiThreadLevel, IntermediateVec(0));
    std::vector<IntermediateVec> shuffleVector(0);

    // init semaphore so other threads would wait to it.
    sem_t * sem = new sem_t;
    int semInitValue = sem_init(sem, 0, 0);    //todo check sem initialization.
    check_for_error(semInitValue, "Failed to initialize semaphore.");


    for (int i = 0; i < multiThreadLevel; ++i) {
        threadContexts[i] = {i, &client, &inputVec, &outputVec, &everydayImShuffling,
                             &atomic_counter, sem, &barrier, &threadsVectors, &shuffleVector};
    }

    for (int i = 1; i < multiThreadLevel; ++i) {
        pthread_create(threads + i, nullptr, threadFlow, threadContexts + i); //todo error check
    }

    //main thread should map and sort as well.
    threadFlow(threadContexts);

    //// Shuffle phase
    (*threadContexts[0].atomic_counter) = 0; // init atomic to track output vec part of treduce method
    shuffle(&threadContexts[0], multiThreadLevel);

    // main thread-reduce
    for (int i=0; i< multiThreadLevel; i++) {
        sem_post(threadContexts->semaphore_arg);
    }
    threadReduce(threadContexts);


        // main thread will wait for all other threads to terminate
    for (int i = 1; i < multiThreadLevel; ++i) {
        // check for error of pthread
        if (DEBUG) { printf("main thread join tid %d \n ", i); }
        pthread_join(threads[i], NULL);
    }

    //finish
    if (DEBUG) { printf("size of output vector is %d \n", (int)threadContexts->outputVec->size()); }
    //todo implement main thread exit? delete object and release memory.
//    exitFramework(threadContexts);
}

////===============================  Helper Functions ==============================================
bool compareKeys(const IntermediatePair& lhs, const IntermediatePair& rhs) {
    return (*lhs.first) < (*rhs.first);
}

void * threadFlow(void * arg) {
    auto tc = (ThreadContext *) arg;
    if (DEBUG) { printf("tid %d is entering threadFlow\n", tc->threadID); }
//    if (DEBUG) errorPrint(tc);

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
                  tc->threadsVectors->at(tc->threadID).end(), compareKeys);
    }

    // setting thread to wait at barrier.
    if (DEBUG) { printf("tid %d is at barrier\n", tc->threadID);}
    tc->barrier->barrier();

    // if not main thread, wait for semaphore to reduce
    if (tc->threadID != 0) {
//        sem_wait(tc->semaphore_arg);
        threadReduce(tc);
    }
    // main thread (ID==0) continues without waiting to shuffle.
    return nullptr; //todo need to check properly
}

//void shuffle(ThreadContext * tc, int multiThreadLevel) {
//    if (DEBUG) { printf("tid %d is entering shuffle\n", tc->threadID); }
//    while (true){
//
//      //// find max key
//        bool allEmptySoFar = true;
//        K2 *curMax = {};
//        // iterate through thread's vectors, and find max at back
//        for (int i = 0; i < multiThreadLevel ; ++i) {
//            //ensure not empty
//            if (!tc->threadsVectors->at(i).empty()) {
//                K2 *thisKey = tc->threadsVectors->at(i).back().first;
//                if(allEmptySoFar){
//                  allEmptySoFar = false;
//                    // take max (back)
//                    curMax = thisKey;
//                }else if(*curMax<*thisKey){
//                    // update max if larger
//                    curMax = thisKey;
//                }
//            }
//        }
//
//        //// exit condition
//        // if we have not found a non-empty vector after iterating - all have been cleared.
//        if(allEmptySoFar) break;
//
//        //// make vector
//        IntermediateVec curKeyVec(0);
//        while(true) {
//            // iterate through thread's vectors
//            for (int i = 0; i < multiThreadLevel; ++i) {
//                //ensure not empty
//                if (!tc->threadsVectors->at(i).empty()) {
//                  bool mightHaveMoreMax;
//                  do{
//                    K2 *thisKey = tc->threadsVectors->at(i).back().first;
//                    if (areEqualK2(*thisKey, *curMax)) {
//                      // if max is found at the back, there might be more
//                      mightHaveMoreMax = true;
//                      // add it to our vector
//                      curKeyVec.push_back((tc->threadsVectors->at(i).back()));
//                      // erase it from the vector
//                      tc->threadsVectors->at(i).pop_back();
//                    }else{
//                      // if top is not max, we are done with ths vector
//                      mightHaveMoreMax = false;
//                    }
//                  }while(mightHaveMoreMax);
//
//                }
//            }
//
//            // we have assembled a vector of all curMax
//
//            //todo send vector to a thread
//
//            tc->barrier->shuffleLock();   // blocking the mutex
//
//            // feeding shared vector and increasing semaphore.
//            tc->shuffleVector->push_back(curKeyVec);
//            int atom_up = (*(tc->atomic_counter))++;  //todo we might need to remove. that depends on reduce design
//            if (DEBUG) { printf("shuffle old_atom is %d\n", atom_up);}
//
//            sem_post(tc->semaphore_arg);
//
//            tc->barrier->shuffleUnlock(); // unblock mutex
//
//            // break, and find the next key.
//            break;
//
//        }
//
//    }
//    *tc->stillShuffling = false;
//}

void shuffle(ThreadContext * tc, int multiThreadLevel) {
    int shufCounter = 0;
//    if (DEBUG) {
//        for (int i = 0; i < multiThreadLevel; i++) {
//            if (DEBUG) { printf("vec #%d, ", i); }
//            for (int j = 0; j < tc->threadsVectors->at(i).size(); j++) {
//                if (DEBUG) { printf("elem #%d: ", j); }
////                char key = tc->threadsVectors->at(i).at(j).first->getC();
////                int val = tc->threadsVectors->at(i).at(j).second->getV();
////                printf("key: %c, val:%d\n", key, val);
//            }
//            if (DEBUG) { printf("----------\n"); }
//        }
//    }
    bool continueShuffle = true;

    // Looping to create
    while (continueShuffle) {
        int maxKeyThreadId = -1;
        K2 * key = nullptr;
        int val = -1;

        // Finding the first not empty thread Intermediate Vector.
        for (int i = 0; i < multiThreadLevel; i++) {
            if (DEBUG) { printf("~~~~~nonempty i is %d~~~~\n", i);}
            if (!tc->threadsVectors->at(i).empty()) {
                maxKeyThreadId = i;
                key = tc->threadsVectors->at(i).back().first;
//                val = int(key->getC());
//                if (DEBUG) { printf("first non empty is in vec #%d and key is %c with val of: %d\n", i, key->getC(), val);}
                break;
            }
            //todo N:check if code can reach to the point of the last thread and it's empty.
        }

        // if didn't find non-empty thread Vector
        if (maxKeyThreadId == -1) {
            continueShuffle = false;
            break;
        }

        // Finding the max key
        for (int i = maxKeyThreadId; i < multiThreadLevel; i++) {
            if (tc->threadsVectors->at(i).empty()) { continue; }
            if (DEBUG) { printf("~~~~~max loop i is %d~~~~\n", i); }
            K2 * other = tc->threadsVectors->at(i).back().first;
            bool a = key < tc->threadsVectors->at(i).back().first;
            bool b = key < other;
            bool c = *key < *other;
            if (DEBUG) { printf("a:%d b:%d c:%d", a,b,c); }
            if (!tc->threadsVectors->at(i).empty() && (*key) < *(tc->threadsVectors->at(i).back().first)) {
//            if (!tc->threadsVectors->at(i).empty() &&
//                !areEqualK2(*key, *tc[i].threadsVectors->at(i).back().first) &&
//                (key < tc->threadsVectors->at(i).back().first)) {
                    maxKeyThreadId = i;
                    key = tc->threadsVectors->at(i).back().first;
//                    val = int(key->getC());
//                if (DEBUG) { printf("found bigger key inin vec #%d and key is %c with val of:%d\n", i, key->getC(), val);}
            }
        }

        IntermediateVec currentKeyIndVec(0);
        for (int i = maxKeyThreadId; i < multiThreadLevel; i++) {
            // Popping matching k2pairs from all thread's vectors.

            while (!tc->threadsVectors->at(i).empty() &&
                   areEqualK2(*(tc->threadsVectors->at(i).back().first), *key)) {
                // Popping matching k2 pairs of current thread with tid i
//                if (DEBUG) { printf("popping from vec #%d the key is %c\n", i, tc->threadsVectors->at(i).back().first->getC()); }
                currentKeyIndVec.push_back((tc->threadsVectors->at(i).back()));
                tc->barrier->threadsVecsLock();
                tc->threadsVectors->at(i).pop_back();
                tc->barrier->threadsVecsUnlock();
            }
        }

        tc->barrier->shuffleLock();   // blocking the mutex

        // feeding shared vector and increasing semaphore.
        tc->shuffleVector->emplace_back(currentKeyIndVec);
        int ret = (*(tc->atomic_counter))++;
        shufCounter++;
        sem_post(tc->semaphore_arg);
//        if (DEBUG) { printf("shuffle increment atomic to: %d \n", ++ret); }
        tc->barrier->shuffleUnlock();
    }
    if (DEBUG) { printf("size of shuffle vector is %d \n", shufCounter); }
    *tc->stillShuffling = false;
}


void threadReduce(ThreadContext * tc) {
    while (true){

        sem_wait(tc->semaphore_arg);

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
    return;

}

//        if (tc->stillShuffling) {
//            sem_wait(tc->semaphore_arg);
//
//            int oatom = (*(tc->atomic_counter))--;
//
//            //can take one
//            tc->barrier->shuffleLock();
//
//            if (DEBUG) { printf("whiel shuffles tid %d is reduce loop and atom before decrease is: %d \n", tc->threadID, oatom); }
//            IntermediateVec * pairs = &(tc->shuffleVector->back());
//            tc->client->reduce(pairs, tc);
//            tc->shuffleVector->pop_back();
//            tc->barrier->shuffleUnlock();
//            sem_wait(tc->semaphore_arg);
//        } else {
//            int oatom = (*(tc->atomic_counter))--;
//            if (oatom >= 1) {
//
//                //can take one
//                tc->barrier->shuffleLock();
//
//                if (DEBUG) { printf("DONE SHUFFLE tid %d is reduce loop and atom before decrease is: %d \n", tc->threadID, oatom); }
//                IntermediateVec * pairs = &(tc->shuffleVector->back());
//                tc->client->reduce(pairs, tc);
//                tc->shuffleVector->pop_back();
//                tc->barrier->shuffleUnlock();
//                if (oatom==1) { break; }
//                sem_wait(tc->semaphore_arg);
//            } else {
//                // there is nothing left.
//                break;
//            }
//        }



//        int old_atom = (*(tc->atomic_counter));
//        if (old_atom <= 0) {
//            break;
//        }
//
//        tc->barrier->shuffleLock();
//        if (DEBUG) { printf("tid %d is reduce loop and cur atom is: %d \n", tc->threadID, old_atom); }
//        IntermediateVec * pairs = &(tc->shuffleVector->back());
//
//        tc->client->reduce(pairs, tc);
//        tc->shuffleVector->pop_back();
//
//        old_atom = (*(tc->atomic_counter))--;
//        tc->barrier->shuffleUnlock();
//
//        if (*tc->stillShuffling) {
//            sem_wait(tc->semaphore_arg);
//        } else {
//            tc->barrier->shuffleLock();
//            if (tc->shuffleVector->empty()) {
//                tc->barrier->shuffleUnlock();
//                break;
//            }
//            tc->barrier->shuffleUnlock();
//            sem_wait(tc->semaphore_arg);
//        }
//    }
//
//
//
//    bool shouldContinueReducing = true;
//    while (shouldContinueReducing) {        //todo check properly
//        if (tc->threadID == 0) {
//            int oldat = (*(tc->atomic_counter))--;
//            if (oldat < 1) { break; }
//        }
////        sem_wait(tc->semaphore_arg);
////        unsigned int old_atom = (*(tc->atomic_counter))--;
//
////        if (DEBUG) { printf("tid %d old atom%d\n", tc->threadID, old_atom); }
//
////        if (tc->threadID == 0 && old_atom < 1) { break; }
//        tc->barrier->shuffleLock();
//
//
//        IntermediateVec * pairs = &(tc->shuffleVector->back());
//        tc->client->reduce(pairs, tc);
//        tc->shuffleVector->pop_back();
//
//        unsigned int old_atom = (*(tc->atomic_counter))--;
//        if (DEBUG) { printf("tid %d old atom%d\n", tc->threadID, old_atom); }
//        tc->barrier->shuffleUnlock();
//
//        if (old_atom <= 1) {
//            shouldContinueReducing = false;
//        }
//    }
//
//    if (tc->threadID != 0) {
//        printf("tid %d is exited\n", tc->threadID);
//        pthread_exit(nullptr);
//    }
//}


bool areEqualK2(K2& a, K2& b){

    // neither a<b nor b<a means a==b
    return !((a<b)||(b<a));
}

void exitFramework(ThreadContext * tc) {
    if (DEBUG) { printf("exiting framework");};
//    exit(0);
//    delete (tc->barrier);
//    sem_destroy(tc->semaphore_arg);
//    todo verify with valgrind
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

////=================================  debug Function ==============================================

//void errorPrint(ThreadContext* tc) {
//    int i = 0;
//
//    for (IntermediateVec vic: *tc->threadsVectors) {
//        std::cout << "Vector " << i << "  :  ";
//        ++i;
//
//        for (std::pair<*K2, *V2> par: vic) {
//            std::cout << "(  " <<  << "  )";
//        }
//        std::cout << "\n";
//    }
//}
