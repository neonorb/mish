/*
 * thread.h
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_THREAD_H_
#define INCLUDE_THREAD_H_

namespace mish {

class Thread;

}

#include <mish.h>
#include <callback.h>

using namespace feta;

namespace mish {

enum ThreadPriority {
	BACKGROUND, // will pause between executions
	ACTIVE, // will not pause between executions
};

class Thread {
public:
	Thread(Code* code, ThreadPriority priority);
	~Thread();

	Code* code;
	mish::execute::State* state;
	ThreadPriority priority;

	Callback<void(Thread*)> onThreadExit;
};

}

#endif /* INCLUDE_THREAD_H_ */
