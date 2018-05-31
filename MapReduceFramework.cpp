
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

void noThreads(){ //todo remove

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

bool areEqual(K3 &a, K3 &b){

	// neither a<b nor b<a means a==b
	return !((a<b)||(b<a));
}
