/*
 * executer.cpp
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#include <executer.h>

// ---- state ----

// ExecutionStackFrame
ExecutionStackFrame::ExecutionStackFrame(ExecutionStackFrameType type) {
	this->type = type;
}

ExecutionStackFrame::~ExecutionStackFrame() {

}

// BytecodeStackFrame
BytecodeStackFrame::BytecodeStackFrame(List<Bytecode*>* bytecodes) :
		ExecutionStackFrame(ExecutionStackFrameType::BYTECODE) {
	this->bytecodesIterator = new Iterator<Bytecode*>(bytecodes->iterator());
}

BytecodeStackFrame::~BytecodeStackFrame() {
	delete bytecodesIterator;
}

// FunctionCallStackFrame
FunctionCallStackFrame::FunctionCallStackFrame(Function* function,
		List<Expression*>* arguments, ValueCallback response) :
		ExecutionStackFrame(ExecutionStackFrameType::FUNCTION_CALL) {
	mode = FunctionCallStackFrameMode::EVALUATE;

	this->function = function;
	this->arguments = arguments;
	this->evaluations = new List<Value*>();
	this->response = response;
}

FunctionCallStackFrame::~FunctionCallStackFrame() {
	Iterator<Value*> evaluationIterator = evaluations->iterator();
	while (evaluationIterator.hasNext()) {
		Value* value = evaluationIterator.next();
		value->deleteReference();
	}
	delete evaluations;
}

// ArgumentStackFrame
ArgumentStackFrame::ArgumentStackFrame(List<Expression*>* expressions,
		List<Value*>* evaluations) :
		ExecutionStackFrame(ExecutionStackFrameType::ARGUMENT) {
	this->expressionIterator = new Iterator<Expression*>(
			expressions->iterator());
	this->evaluations = evaluations;
}

ArgumentStackFrame::~ArgumentStackFrame() {
	delete expressionIterator;
}

// IfStackFrame
IfStackFrame::IfStackFrame(List<IfConditionCode*>* ifs) :
		ExecutionStackFrame(ExecutionStackFrameType::IF) {
	mode = IfStackFrameMode::EVALUATE;

	ifsIterator = new Iterator<IfConditionCode*>(ifs->iterator());

	lastEvaluation = NULL;
}

IfStackFrame::~IfStackFrame() {
	delete ifsIterator;

	if (lastEvaluation != NULL) {
		lastEvaluation->deleteReference();
	}
}

// WhileStackFrame
WhileStackFrame::WhileStackFrame(Expression* condition, Code* code,
		bool isDoWhile) :
		ExecutionStackFrame(ExecutionStackFrameType::WHILE) {
	this->condition = condition;
	this->code = code;

	if (isDoWhile) {
		mode = WhileStackFrameMode::RUN;
	} else {
		mode = WhileStackFrameMode::EVALUATE;
	}

	lastEvaluation = NULL;
}

WhileStackFrame::~WhileStackFrame() {
	if (lastEvaluation != NULL) {
		lastEvaluation->deleteReference();
	}
}

// ExecuterState
ExecuterState::ExecuterState() {
	executionStack = new Stack<ExecutionStackFrame*>();
}

ExecuterState::~ExecuterState() {
	while (executionStack->size() > 0) {
		delete executionStack->pop();
	}
	delete executionStack;
}

// ---- execution ----

static void evaluateExpression(Expression* expression, ExecuterState* state,
		ValueCallback response) {
	switch (expression->expressionType) {
	case VALUE_EXPRESSION: {
		Value* constant = (Value*) expression;
		response(constant);
	}
		break;
	case FUNCTION_EXPRESSION:
		FunctionCallReturn* functionCallReturn =
				(FunctionCallReturn*) expression;

		state->executionStack->push(
				new FunctionCallStackFrame(functionCallReturn->function,
						functionCallReturn->arguments, response));

		break;
	}
}

ExecuteStatus mish_execute(ExecuterState* state) {
	if (state->executionStack->peek()->type
			== ExecutionStackFrameType::BYTECODE) {
		BytecodeStackFrame* stackFrame =
				(BytecodeStackFrame*) state->executionStack->peek();
		if (stackFrame->bytecodesIterator->hasNext()) {
			Bytecode* bytecode = stackFrame->bytecodesIterator->next();
			if (bytecode->type == BytecodeType::FUNC_CALL) {
				FunctionCallVoid* functionCallVoid =
						(FunctionCallVoid*) bytecode;

				state->executionStack->push(
						new FunctionCallStackFrame(functionCallVoid->function,
								functionCallVoid->arguments,
								ValueCallback(NULL,
										[](void* scope, Value* response) -> void* {
											/* discard the return value */
											delete response;
											return NULL;
										})));
			} else if (bytecode->type == BytecodeType::IF) {
				IfBytecode* ifBytecode = (IfBytecode*) bytecode;
				state->executionStack->push(new IfStackFrame(ifBytecode->ifs));
			} else if (bytecode->type == BytecodeType::WHILE) {
				WhileBytecode* whileBytecode = (WhileBytecode*) bytecode;
				state->executionStack->push(
						new WhileStackFrame(whileBytecode->condition,
								whileBytecode->code, whileBytecode->isDoWhile));
			} else {
				crash("unknown BytecodeType");
			}
		} else {
			delete state->executionStack->pop();

			// TODO end of function... if non-void function, crash system, if void function, then return
			// for now, just end the execution
			if (state->executionStack->size() == 0) {
				return ExecuteStatus::DONE;
			}
		}
	} else if (state->executionStack->peek()->type
			== ExecutionStackFrameType::FUNCTION_CALL) {
		FunctionCallStackFrame* stackFrame =
				(FunctionCallStackFrame*) state->executionStack->peek();
		if (stackFrame->mode == FunctionCallStackFrameMode::EVALUATE) {
			ArgumentStackFrame* argumentStackFrame = new ArgumentStackFrame(
					stackFrame->arguments, stackFrame->evaluations);
			state->executionStack->push(argumentStackFrame);

			stackFrame->mode = FunctionCallStackFrameMode::CALL;
		} else if (stackFrame->mode == FunctionCallStackFrameMode::CALL) {
			// gather infos
			Function* function = stackFrame->function;
			List<Value*>* evaluations = stackFrame->evaluations;

			// prepare return mode
			stackFrame->mode = FunctionCallStackFrameMode::RETURN;

			// call function
			if (function->native != NULL) {
				Value* returnValue = function->native(evaluations);
				stackFrame->response(returnValue);
			} else {
				state->executionStack->push(
						new BytecodeStackFrame(function->code->bytecodes));
				// TODO evaluate and call function
				// TODO remember to create return reference
				crash("regular functions not implemented yet");
				return ExecuteStatus::NOT_DONE;		// don't delete evaluations
			}
		} else if (stackFrame->mode == FunctionCallStackFrameMode::RETURN) {
			delete state->executionStack->pop();
		} else {
			crash("unknown FunctionCallStackFrameMode");
		}
	} else if (state->executionStack->peek()->type
			== ExecutionStackFrameType::ARGUMENT) {
		ArgumentStackFrame* stackFrame =
				(ArgumentStackFrame*) state->executionStack->peek();
		if (stackFrame->expressionIterator->hasNext()) {
			Expression* expression = stackFrame->expressionIterator->next();
			evaluateExpression(expression, state,
					ValueCallback(stackFrame,
							[](void* stackFrame, Value* value) -> void* {
								value->createReference();
								((ArgumentStackFrame*) stackFrame)->evaluations->add(value);
								return NULL;
							}));
		} else {
			// out of evaluations
			delete state->executionStack->pop();
		}
	} else if (state->executionStack->peek()->type
			== ExecutionStackFrameType::IF) {
		IfStackFrame* stackFrame =
				(IfStackFrame*) state->executionStack->peek();
		if (stackFrame->mode == IfStackFrameMode::EVALUATE) {
			if (stackFrame->ifsIterator->hasNext()) {
				evaluateExpression(
						stackFrame->ifsIterator->peekNext()->condition, state,
						ValueCallback(stackFrame,
								[](void* stackFrameArgument, Value* value) -> void* {
									IfStackFrame* stackFrame = (IfStackFrame*) stackFrameArgument;

									// delete old value
									if(stackFrame->lastEvaluation!= NULL) {
										stackFrame->lastEvaluation->deleteReference();
									}

									// set new value
									stackFrame->lastEvaluation = value;
									value->createReference();

									return NULL;
								}));
				stackFrame->mode = IfStackFrameMode::TEST;
			} else {
				delete state->executionStack->pop();
			}
		} else if (stackFrame->mode == IfStackFrameMode::TEST) {
			if (((BooleanValue*) stackFrame->lastEvaluation)->value) {
				// evaluation is true, run body
				stackFrame->mode = IfStackFrameMode::RUN;
			} else {
				// evaluation is false, go to next condition (else/elseif)
				stackFrame->ifsIterator->next();
				stackFrame->mode = IfStackFrameMode::EVALUATE;
			}
		} else if (stackFrame->mode == IfStackFrameMode::RUN) {
			// test was successful, run and be done
			stackFrame->mode = IfStackFrameMode::DONE;
			state->executionStack->push(
					new BytecodeStackFrame(
							stackFrame->ifsIterator->next()->code->bytecodes));
		} else if (stackFrame->mode == IfStackFrameMode::DONE) {
			// previous call was successful, exit
			delete state->executionStack->pop();
		} else {
			crash("unknown IfStackFrameMode");
		}
	} else if (state->executionStack->peek()->type
			== ExecutionStackFrameType::WHILE) {
		WhileStackFrame* stackFrame =
				(WhileStackFrame*) state->executionStack->peek();
		if (stackFrame->mode == WhileStackFrameMode::EVALUATE) {
			evaluateExpression(stackFrame->condition, state,
					ValueCallback(stackFrame,
							[](void* stackFrameArgument, Value* value) -> void* {
								WhileStackFrame* stackFrame = (WhileStackFrame*) stackFrameArgument;

								// delete old value
								if(stackFrame->lastEvaluation!= NULL) {
									stackFrame->lastEvaluation->deleteReference();
								}

								// set new value
								stackFrame->lastEvaluation = value;
								value->createReference();

								return NULL;
							}));
			stackFrame->mode = WhileStackFrameMode::TEST;
		} else if (stackFrame->mode == WhileStackFrameMode::TEST) {
			if (((BooleanValue*) stackFrame->lastEvaluation)->value) {
				stackFrame->mode = WhileStackFrameMode::RUN;
			} else {
				delete state->executionStack->pop();
			}
		} else if (stackFrame->mode == WhileStackFrameMode::RUN) {
			state->executionStack->push(
					new BytecodeStackFrame(stackFrame->code->bytecodes));
			stackFrame->mode = WhileStackFrameMode::EVALUATE;
		} else {
			crash("unknown WhileStackFrameMode");
		}
	} else {
// TODO fault as error in Mish and continue
		crash("unknown execution mode");
	}

	return ExecuteStatus::NOT_DONE;
}

void mish_execute(Code* code) {
	ExecuterState* state = new ExecuterState();

// start executing on first bytecode
// TODO change this so that execution can resume and exit after every cycle
	state->executionStack->push(new BytecodeStackFrame(code->bytecodes));

	while (true) {
		ExecuteStatus status = mish_execute(state);
		if (status == ExecuteStatus::DONE) {
			break;
		}
	}

	delete state;
}
