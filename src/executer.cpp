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

	conditionalBytecodeStack = new Stack<ConditionalBytecode*>();
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
			value->deleteReference();
		}
		delete evaluations;
	}
	delete evaluationsStack;

	delete functionStack;

	delete conditionalBytecodeStack;
}

ExecuteStatus mish_execute(ExecuterState* state) {
	if (state->modeStack->peek() == BYTECODE_MODE) {
		if (state->callStack->peek()->hasNext()) {
			Bytecode* bytecode = state->callStack->peek()->next();
			switch (bytecode->instruction) {
			case FUNC_CALL_INSTRUCTION: {
				FunctionCallVoid* functionCallVoid =
						(FunctionCallVoid*) bytecode;

				state->argumentIteratorStack->push(
						new Iterator<Expression*>(
								functionCallVoid->arguments->iterator()));
				state->evaluationsStack->push(new List<Value*>());
				state->functionStack->push(functionCallVoid->function);
				state->modeStack->push(ARGUMENT_MODE);
			}

				break;
			case CONDITIONAL_INSTRUCTION:
				ConditionalBytecode* conditionalBytecode =
						(ConditionalBytecode*) bytecode;

				state->conditionalBytecodeStack->push(conditionalBytecode);

				state->modeStack->push(AFTERCONDITIONAL_MODE);

				if (conditionalBytecode->type == IF_CONDITIONALTYPE) {
					state->argumentIteratorStack->push(
							new Iterator<Expression*>(
									conditionalBytecode->condition->iterator()));
					state->evaluationsStack->push(new List<Value*>());
					state->modeStack->push(ARGUMENT_MODE);

				} else {
					state->modeStack->push(LOOP_MODE);
				}

				break;
			}
		} else {
			delete state->callStack->pop();
			state->modeStack->pop();

			// TODO end of function... if non-void function, crash system, if void function, then return
			// for now, just end the execution
			if (state->callStack->size() == 0) {
				return DONE;
			}
		}
	} else if (state->modeStack->peek() == ARGUMENT_MODE) {
		if (state->argumentIteratorStack->peek()->hasNext()) {
			Expression* expression =
					state->argumentIteratorStack->peek()->next();
			switch (expression->expressionType) {
			case VALUE_EXPRESSION: {
				Value* constant = (Value*) expression;
				constant->createReference();
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
			// out of evaluations
			delete state->argumentIteratorStack->pop();
			List<Value*>* evaluations = state->evaluationsStack->pop();
			state->modeStack->pop();
			if (state->modeStack->peek() == BYTECODE_MODE
					|| state->modeStack->peek() == ARGUMENT_MODE) {
				Function* function = state->functionStack->pop();

				if (function->native != NULL) {
					Value* returnValue = function->native(evaluations);
					returnValue->createReference();
					// TODO delete the return value at some point
					if (state->modeStack->peek() == ARGUMENT_MODE) {
						state->evaluationsStack->peek()->add(
								returnValue);
					}
				} else {
					state->modeStack->push(BYTECODE_MODE);
					state->callStack->push(
							new Iterator<Bytecode*>(
									function->code->bytecodes->iterator()));
					// TODO evaluate and call function
					// TODO remember to create return reference
					crash(L"regular functions not implemented yet");
					return NOT_DONE; // don't delete evaluations
				}
			} else {
				BooleanValue* condition = (BooleanValue*) evaluations->get(0);
				if (condition->value) { // if the value is true
					state->modeStack->push(BYTECODE_MODE);
					state->callStack->push(
							new Iterator<Bytecode*>(
									state->conditionalBytecodeStack->peek()->code->bytecodes->iterator()));
				} else {
					if (state->modeStack->peek() == LOOP_MODE) {
						state->modeStack->pop(); // end loop
					}
				}
			}

			Iterator<Value*> evaluationIterator = evaluations->iterator();
			while (evaluationIterator.hasNext()) {
				Value* value = evaluationIterator.next();
				value->deleteReference();
			}
			delete evaluations;
		}
	} else if (state->modeStack->peek() == LOOP_MODE) {
		state->argumentIteratorStack->push(
				new Iterator<Expression*>(
						state->conditionalBytecodeStack->peek()->condition->iterator()));
		state->evaluationsStack->push(new List<Value*>());
		state->modeStack->push(ARGUMENT_MODE);
	} else if (state->modeStack->peek() == AFTERCONDITIONAL_MODE) {
		state->modeStack->pop();
		state->conditionalBytecodeStack->pop();
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
		ExecuteStatus status = mish_execute(state);
		if (status == DONE) {
			break;
		}
	}

	delete state;
}
