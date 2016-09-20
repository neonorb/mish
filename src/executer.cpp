/*
 * executer.cpp
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#include <executer.h>

ExecuterState::ExecuterState() {
	modeStack = new Stack<ExecuteMode>();

	callStack = new Stack<Iterator<Bytecode*>*>();

	argumentIteratorStack = new Stack<Iterator<Expression*>*>();
	evaluationsStack = new Stack<List<Value*>*>();

	functionStack = new Stack<Function*>();
	returnValue = NULL;
}

ExecuterState::~ExecuterState() {
	delete modeStack;

	while (callStack->size() > 0) {
		delete callStack->pop();
	}
	delete callStack;

	while (argumentIteratorStack->size() > 0) {
		delete argumentIteratorStack->pop();
	}
	delete argumentIteratorStack;

	while (evaluationsStack->size() > 0) {
		List<Value*>* evaluations = evaluationsStack->pop();
		Iterator<Value*> evaluationIterator = evaluations->iterator();
		while (evaluationIterator.hasNext()) {
			Value* value = evaluationIterator.next();
			if (!value->isConstant) {
				delete value;
			}
		}
		delete evaluations;
	}
	delete evaluationsStack;

	delete functionStack;

	if (returnValue != NULL) {
		delete returnValue;
	}
}

enum ExecuteStatus {
	DONE, NOT_DONE
};

ExecuteStatus execute(ExecuterState* state) {
	if (state->modeStack->peek() == BYTECODE_MODE) {
		if (state->callStack->peek()->hasNext()) {
			Bytecode* bytecode = state->callStack->peek()->next();
			switch (bytecode->instruction) {
			case FUNC_CALL:
				FunctionCallVoid* functionCallVoid =
						(FunctionCallVoid*) bytecode;

				state->argumentIteratorStack->push(
						new Iterator<Expression*>(
								functionCallVoid->arguments->iterator()));
				state->evaluationsStack->push(new List<Value*>());
				state->functionStack->push(functionCallVoid->function);
				state->modeStack->push(ARGUMENT_MODE);

				break;
			}
		} else {
			// TODO end of function... if non-void function, crash system, if void function, then return
			// for now, just end the execution
			delete state->callStack->pop();
			state->modeStack->pop();
			return DONE;
		}
	} else if (state->modeStack->peek() == ARGUMENT_MODE) {
		if (state->argumentIteratorStack->peek()->hasNext()) {
			Expression* expression =
					state->argumentIteratorStack->peek()->next();
			switch (expression->expressionType) {
			case VALUE_EXPRESSION: {
				Value* constant = (Value*) expression;
				constant->isConstant = true;
				state->evaluationsStack->peek()->add(constant);
			}
				break;
			case FUNCTION_EXPRESSION:
				FunctionCallReturn* functionCallReturn =
						(FunctionCallReturn*) expression;

				state->argumentIteratorStack->push(
						new Iterator<Expression*>(
								functionCallReturn->arguments->iterator()));
				state->evaluationsStack->push(new List<Value*>());
				state->functionStack->push(functionCallReturn->function);
				state->modeStack->push(ARGUMENT_MODE);

				break;
			}
		} else {
			// out of evaluations, call the function
			delete state->argumentIteratorStack->pop();
			List<Value*>* evaluations = state->evaluationsStack->pop();
			Function* function = state->functionStack->pop();

			if (function->native != NULL) {
				state->returnValue = function->native(evaluations);
				// TODO delete the return value at some point
				state->modeStack->pop();
				if (state->modeStack->peek() == ARGUMENT_MODE) {
					state->evaluationsStack->peek()->add(state->returnValue);
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
		// TODO fault as error in Mish and continue
		crash(L"unknown execution mode");
	}

	return NOT_DONE;
}

void mish_execute(Code* code) {
	ExecuterState* state = new ExecuterState();

	// start executing on first bytecode
	// TODO change this so that execution can resume and exit after every cycle
	state->callStack->push(
			new Iterator<Bytecode*>(code->bytecodes->iterator()));
	state->modeStack->push(BYTECODE_MODE);

	while (true) {
		ExecuteStatus status = execute(state);
		if (status == DONE) {
			break;
		}
	}

	delete state;
}
