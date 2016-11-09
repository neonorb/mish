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
	mish::execute::BytecodeStackFrame* firstFrame =
			new mish::execute::BytecodeStackFrame(code->bytecodes);
	firstFrame->state = state;
	state->executionStack->push(firstFrame);

	this->priority = priority;

	onThreadExit = (Callback<void(Thread*)> ) { };
}

Thread::~Thread() {
	delete code;
	delete state;
}
