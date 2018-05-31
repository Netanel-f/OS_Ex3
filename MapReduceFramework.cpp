
#include <atomic>
#include <algorithm>
#include "MapReduceClient.h"
#include "MapReduceFramework.h"

//todo maybe implement data struct as class (instance created for each call to framework?)


//// ============================   defines and const ==============================================


//// ===========================   typedefs & structs =========================================================

//todo struct holding atomic and semaphore

struct data
{
  ////mutexes, semaphors
  //std::atomic<int>& atomic_counter;

  ////params
  //int& multiThreadLevel;

  ////data
  const MapReduceClient& client;
  const InputVec& inputVec;
  IntermediateVec& indVec;
  std::vector<IntermediateVec>& vecVec;
  OutputVec& outputVec;

  data(int lvl,
       const MapReduceClient& client, const InputVec& inputVec,
        OutputVec& outputVec) :

      client(client),
      inputVec(inputVec),
      indVec(),
      vecVec(),
      outputVec(outputVec)
      //multiThreadLevel(lvl)
  {}
};


//// ============================   forward declarations for helper funcs ==========================

void threadFlow();
void mainFlow();
void shuffle();

void noThreads(data &stuff);


bool areEqualK2(K2 *a, K2 *b);

//// ============================ framework functions ==============================================


/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit2 (K2* key, V2* value, void* context){
    IntermediatePair k2_pair = std::pair(&key, &value);
    auto * context_pointer = (data*) context;
    context_pointer->indVec.push_back(k2_pair);

}

/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit3 (K3* key, V3* value, void* context){
  OutputPair k3_pair = std::pair(&key, &value);
  auto * context_pointer = (data *) context;
  context_pointer->outputVec.push_back(k3_pair);
}

/**
 *
 * @param client
 * @param inputVec
 * @param outputVec
 * @param multiThreadLevel
 */
void runMapReduceFramework(const MapReduceClient& client,
	const InputVec& inputVec, OutputVec& outputVec,
	int multiThreadLevel){

  // initialise data
  data stuff(1,client,inputVec, outputVec);

  // call mainFlow()
  noThreads(stuff);

  // finish

}

////===============================  Helper Functions ==============================================

void threadFlow(){

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

void mainFlow(){
	// same as thread flow, but with additional control segments
}

void noThreads(data &stuff){ //todo remove

  //// Map

  // check atomic for new items to be mapped (k1v1)

  // map the items
    for (const InputPair &k1_pair : stuff.inputVec) {
        stuff.client.map(k1_pair.first, k1_pair.second, &stuff);
    }

  // emit the mapped items (k2v2) //todo N: emitted by client map func.

  ///// Sort

  // sort the items
  std::sort(stuff.indVec.begin(), stuff.indVec.end());

  //// Shuffle
    shuffle(stuff);

  //// Reduce
  for(IntermediateVec& vec: stuff.vecVec){
    stuff.client.reduce(&vec,&stuff);
  }

  // wait for new items to be come available

  // make output itemp (k3v3)

}
bool areEqualK2(K2* a, K2* b){

  // neither a<b nor b<a means a==b
  return !((a<b)||(b<a));
}


void shuffle(data stuff){


//    std::vector<IntermediateVec> vecVec; //todo N test
//
//    auto key = stuff.indVec.back().first;
//    IntermediatePair &first_pair = stuff.indVec.back();
//    stuff.indVec.pop_back();
//
//    IntermediateVec first_key_vec;
//    first_key_vec.push_back(first_pair);
//    vecVec.push_back(first_key_vec);
//
//    IntermediateVec current_key_indVec;
//
//
//    while (!stuff.indVec.empty()) {
//        auto current = &stuff.indVec.back();
//        stuff.indVec.pop_back();
//
//        if (current->first == key) {
//            vecVec.back().push_back(*current);
//        } else {
//
//            vecVec.emplace_back(current_key_indVec);
//            current_key_indVec.clear();
//            key = current->first;
//            current_key_indVec.push_back(*current);
//        }
//    }


  std::vector<IntermediateVec> vecVec;

  auto key = stuff.indVec.back().first;
  IntermediateVec current_key_indVec;

  while (!stuff.indVec.empty()) {
    auto current = &stuff.indVec.back();
    stuff.indVec.pop_back();

    if (areEqualK2(current->first, key)) {
      current_key_indVec.push_back(*current);
    } else {
      vecVec.emplace_back(current_key_indVec);
      current_key_indVec.clear();
      key = current->first;
      current_key_indVec.push_back(*current);
    }
  }
}

