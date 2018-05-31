
#include <atomic>
#include "MapReduceClient.h"
#include "MapReduceFramework.h"


//// ============================   defines and const ==============================================


//// ===========================   typedefs & structs =========================================================

//todo struct holding atomic and semaphore

//todo struct holding all inpout and DAST's (?)
struct data{
  std::atomic<int>* atomic_counter;

  InputVec* vIn;
  OutputVec* vOut;

  MapReduceClient& client;
  InputVec& inputVec;
  IntermediateVec& indVec;
  OutputVec& outputVec;
  int& multiThreadLevel;

};


//// ============================   forward declarations for helper funcs ==========================

void threadFlow();
void mainFlow();
void initData();


bool areEqual(K3 a, K3 b);

//// ============================ framework functions ==============================================


/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit2 (K2* key, V2* value, void* context){
    IntermediatePair k2_pair = std::pair(&key, &value);
    auto * context_pointer = (data *) context;
    context_pointer->indVec.push_back(k2_pair);

}

/**
 *
 * @param key
 * @param value
 * @param context
 */
void emit3 (K3* key, V3* value, void* context){

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

  // todo init() (make DAST's ?)
  data stuff;
  initData(client,inputVec,outputVec,multiThreadLevel, stuff);


  // todo call mainFlow()

  // todo finish

}

////===============================  Helper Functions ==============================================

void initData(const MapReduceClient& client,
          const InputVec& inputVec, OutputVec& outputVec,
          int multiThreadLevel, data &stuff){
  stuff.client = client;
  stuff.inputVec = inputVec;
  stuff.outputVec = outputVec;
  stuff.multiThreadLevel  = multiThreadLevel;


}

void threadFlow(){

	//MAP

		// check atomic for new items to be mapped (k1v1)

		// map the items

		// emit the mapped items (k2v2)

	// SORT

		// sort the items

		// emit the sorted items (k2v2) (?)

	// BARRIER

		// get to the barrier
		// wait


	// Reduce

		// wait for new items to be come available

		// make output itemp (k3v3)


}

void mainFlow(){
	//todo same as thread flow, but with additional control segments
}

void noThreads(data &stuff){ //todo remove

  //MAP

  // check atomic for new items to be mapped (k1v1)

  // map the items
    for (InputPair &k1_pair : stuff.inputVec) {
        stuff.client.map(k1_pair.first, k1_pair.second, &stuff);
    }

  // emit the mapped items (k2v2) //todo N: emitted by client map func.

  // SORT

  // sort the items

  // emit the sorted items (k2v2) (?)

  // BARRIER

  // get to the barrier
  // wait
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

        if (current->first == key) {
            current_key_indVec.push_back(*current);
        } else {
//            IntermediateVec vec = current_key_indVec;
//            vecVec.push_back(vec);
            vecVec.emplace_back(current_key_indVec);
            current_key_indVec.clear();
            key = current->first;
            current_key_indVec.push_back(*current);
        }
    }
  // Reduce

  // wait for new items to be come available

  // make output itemp (k3v3)

}

bool areEqual(K3 &a, K3 &b){

	// neither a<b nor b<a means a==b
	return !((a<b)||(b<a));
}
