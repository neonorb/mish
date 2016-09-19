/*
 * executer.cpp
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#include <executer.h>

enum ExecuteMode {
	BYTECODE_MODE, ARGUMENT_MODE
};

void mish_execute(Code* code) {
	// stacks
	Stack<Iterator<Bytecode*>*> callStack;
	Stack<Iterator<Expression*>*> argumentIteratorStack;
	Stack<List<Value*>*> evaluationsStack;
	Stack<Function*> functionStack;
	Stack<ExecuteMode> modeStack;
	Value* returnValue;

	// start executing on first bytecode
	// TODO change this so that execution can resume and exit after every cycle
	callStack.push(new Iterator<Bytecode*>(code->bytecodes->iterator()));
	modeStack.push(BYTECODE_MODE);

	while (true) {
		if (modeStack.peek() == BYTECODE_MODE) {
			if (callStack.peek()->hasNext()) {
				Bytecode* bytecode = callStack.peek()->next();
				switch (bytecode->instruction) {
				case FUNC_CALL:
					FunctionCallVoid* functionCallVoid =
							(FunctionCallVoid*) bytecode;

					argumentIteratorStack.push(
							new Iterator<Expression*>(
									functionCallVoid->arguments->iterator()));
					evaluationsStack.push(new List<Value*>());
					functionStack.push(functionCallVoid->function);
					modeStack.push(ARGUMENT_MODE);

					break;
				}
			} else {
				// TODO end of function... if non-void function, crash system, if void function, then return
				// for now, just end the execution
				delete callStack.pop();
				modeStack.pop();
				break;
			}
		} else if (modeStack.peek() == ARGUMENT_MODE) {
			if (argumentIteratorStack.peek()->hasNext()) {
				Expression* expression = argumentIteratorStack.peek()->next();
				switch (expression->expressionType) {
				case VALUE_EXPRESSION: {
					Value* constant = (Value*) expression;
					constant->isConstant = true;
					evaluationsStack.peek()->add(constant);
				}
					break;
				case FUNCTION_EXPRESSION:
					FunctionCallReturn* functionCallReturn =
							(FunctionCallReturn*) expression;

					argumentIteratorStack.push(
							new Iterator<Expression*>(
									functionCallReturn->arguments->iterator()));
					evaluationsStack.push(new List<Value*>());
					functionStack.push(functionCallReturn->function);
					modeStack.push(ARGUMENT_MODE);

					break;
				}
			} else {
				// out of evaluations, call the function
				delete argumentIteratorStack.pop();
				List<Value*>* evaluations = evaluationsStack.pop();
				Function* function = functionStack.pop();

				if (function->native != NULL) {
					returnValue = function->native(evaluations);
					// TODO delete the return value at some point
					modeStack.pop();
					if (modeStack.peek() == ARGUMENT_MODE) {
						evaluationsStack.peek()->add(returnValue);
					}
				} else {
					// TODO push callStack
					crash(L"non-native function calls not implemented yet");
				}

				Iterator<Value*> evaluationIterator = evaluations->iterator();
				while (evaluationIterator.hasNext()) {
					Value* value = evaluationIterator.next();
					if (!value->isConstant) {
						delete value;
					}
				}
				delete evaluations;
			}
		} else {
			crash(L"unknown execution mode");
		}
	}
}
