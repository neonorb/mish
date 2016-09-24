/*
 * thread.h
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_THREAD_H_
#define INCLUDE_THREAD_H_

class Thread;

#include <mish.h>

typedef void (*OnThreadExit)(Thread*);

enum ThreadPriority {
	BACKGROUND, // will pause between executions
	ACTIVE, // will not pause between executions
};

class Thread {
public:
	Thread(Code* code, ThreadPriority priority);
	~Thread();

	Code* code;
	ExecuterState* state;
	ThreadPriority priority;

	OnThreadExit onThreadExit;
};

#endif /* INCLUDE_THREAD_H_ */
