/*
 * executer.h
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_EXECUTER_H_
#define INCLUDE_EXECUTER_H_

class ExecuterState;

#include <mish.h>
#include <stack.h>

using namespace feta;

enum ExecuteStatus {
	DONE, NOT_DONE
};

enum ExecuteMode {
	BYTECODE_MODE, ARGUMENT_MODE, LOOP_MODE, AFTERCONDITIONAL_MODE
};

class ExecuterState {
public:
	ExecuterState();
	~ExecuterState();

	Stack<ExecuteMode>* modeStack;

	Stack<Iterator<Bytecode*>*>* callStack;

	Stack<Iterator<Expression*>*>* argumentIteratorStack;
	Stack<List<Value*>*>* evaluationsStack;

	Stack<Function*>* functionStack;

	Stack<ConditionalBytecode*>* conditionalBytecodeStack;
};

void mish_execute(Code* code);
ExecuteStatus mish_execute(ExecuterState* state);

#endif /* INCLUDE_EXECUTER_H_ */
