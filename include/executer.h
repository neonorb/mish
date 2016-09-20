/*
 * executer.h
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_EXECUTER_H_
#define INCLUDE_EXECUTER_H_

#include <mish.h>
#include <stack.h>

void mish_execute(Code* code);

enum ExecuteMode {
	BYTECODE_MODE, ARGUMENT_MODE
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
	Value* returnValue;
};

#endif /* INCLUDE_EXECUTER_H_ */
