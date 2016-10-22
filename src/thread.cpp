/*
 * thread.cpp
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#include <thread.h>

Thread::Thread(Code* code, ThreadPriority priority) {
	this->code = code;

	state = new ExecuterState();
	state->executionStack->push(new BytecodeStackFrame(code->bytecodes));

	this->priority = priority;

	onThreadExit = NULL;
}

Thread::~Thread() {
	delete code;
	delete state;
}
