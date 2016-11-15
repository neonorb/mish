/*
 * thread.cpp
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#include <thread.h>

Thread::Thread(Code* code, ThreadPriority priority) {
	this->code = code;

	state = new mish::execute::State();
	mish::execute::BodyStackFrame* firstFrame =
			new mish::execute::BodyStackFrame(code);
	firstFrame->state = state;
	state->executionStack->push(firstFrame);

	this->priority = priority;

	onThreadExit = (Callback<void(Thread*)> ) { };
}

Thread::~Thread() {
	delete code;
	delete state;
}
