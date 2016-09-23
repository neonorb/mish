/*
 * thread.cpp
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#include <thread.h>

Thread::Thread(Code* code) {
	this->code = code;

	state = new ExecuterState();
	state->callStack->push(
			new Iterator<Bytecode*>(code->bytecodes->iterator()));
	state->modeStack->push(BYTECODE_MODE);

	onThreadExit = NULL;
}

Thread::~Thread() {
	delete code;
	delete state;
}
