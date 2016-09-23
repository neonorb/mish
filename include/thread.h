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

class Thread {
public:
	Thread(Code* code);
	~Thread();

	Code* code;
	ExecuterState* state;

	OnThreadExit onThreadExit;
};

#endif /* INCLUDE_THREAD_H_ */
