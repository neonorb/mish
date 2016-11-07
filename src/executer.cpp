/*
 * executer.cpp
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#include <executer.h>
#include <feta.h>
#include <memory.h>

namespace mish {

namespace execute {

// ---- state ----

// ExecutionStackFrame
ExecutionStackFrame::ExecutionStackFrame(ExecutionStackFrameType type) {
	this->type = type;
	state = NULL;
}

ExecutionStackFrame::~ExecutionStackFrame() {

}

Status ExecutionStackFrame::execute() {
	return Status::OK;
}

void ExecutionStackFrame::startFrame(ExecutionStackFrame* frame) {
	frame->state = state;
	state->executionStack->push(frame);
}

Status ExecutionStackFrame::endFrame() {
	if (state->executionStack->size() == 1) {
		delete state->executionStack->pop();
		return Status::DONE;
	} else {
		delete state->executionStack->pop();
		return Status::OK;
	}
}

void ExecutionStackFrame::evaluateExpression(Expression* expression,
		Callback<Status(Value*)> response) {
	switch (expression->expressionType) {
	case VALUE_EXPRESSION: {
		Value* constant = (Value*) expression;
		response(constant);
	}
		break;
	case FUNCTION_EXPRESSION:
		FunctionCallReturn* functionCallReturn =
				(FunctionCallReturn*) expression;

		startFrame(
				new FunctionCallStackFrame(functionCallReturn->function,
						functionCallReturn->arguments, response));

		break;
	}
}

// BytecodeStackFrame
BytecodeStackFrame::BytecodeStackFrame(List<Bytecode*>* bytecodes) :
		ExecutionStackFrame(ExecutionStackFrameType::BYTECODE) {
	this->bytecodesIterator = new Iterator<Bytecode*>(bytecodes->iterator());
}

BytecodeStackFrame::~BytecodeStackFrame() {
	delete bytecodesIterator;
}

Status BytecodeStackFrame::functionCallCallback(Value* ret) {
	/* discard the return value */
	delete ret;
	return Status::OK;
}

Status BytecodeStackFrame::execute() {
	if (bytecodesIterator->hasNext()) {
		Bytecode* bytecode = bytecodesIterator->next();
		if (bytecode->type == BytecodeType::FUNC_CALL) {
			FunctionCallVoid* functionCallVoid = (FunctionCallVoid*) bytecode;

			startFrame(
					new FunctionCallStackFrame(functionCallVoid->function,
							functionCallVoid->arguments,
							BIND_MEM_CB(&BytecodeStackFrame::functionCallCallback, this)));
		} else if (bytecode->type == BytecodeType::IF) {
			IfBytecode* ifBytecode = (IfBytecode*) bytecode;
			startFrame(new IfStackFrame(ifBytecode->ifs));
		} else if (bytecode->type == BytecodeType::WHILE) {
			WhileBytecode* whileBytecode = (WhileBytecode*) bytecode;
			startFrame(
					new WhileStackFrame(whileBytecode->condition,
							whileBytecode->code, whileBytecode->isDoWhile));
		} else {
			crash("unknown BytecodeType");
		}
	} else {
		return endFrame();

		// TODO end of function... if non-void function, crash system, if void function, then return
		// for now, just end the execution
	}

	return Status::OK;
}

// FunctionCallStackFrame
FunctionCallStackFrame::FunctionCallStackFrame(Function* function,
		List<Expression*>* arguments, Callback<Status(Value*)> functionCallback) :
		ExecutionStackFrame(ExecutionStackFrameType::FUNCTION_CALL) {
	mode = FunctionCallStackFrameMode::EVALUATE;

	this->function = function;
	this->arguments = arguments;
	this->evaluations = new List<Value*>();
	this->functionCallback = functionCallback;
}

FunctionCallStackFrame::~FunctionCallStackFrame() {
	Iterator<Value*> evaluationIterator = evaluations->iterator();
	while (evaluationIterator.hasNext()) {
		Value* value = evaluationIterator.next();
		value->deleteReference();
	}
	delete evaluations;
}

Status FunctionCallStackFrame::execute() {
	if (mode == FunctionCallStackFrameMode::EVALUATE) {
		ArgumentStackFrame* argumentStackFrame = new ArgumentStackFrame(
				arguments, evaluations);
		startFrame(argumentStackFrame);

		mode = FunctionCallStackFrameMode::CALL;
	} else if (mode == FunctionCallStackFrameMode::CALL) {
		// prepare return mode
		mode = FunctionCallStackFrameMode::RETURN;

		// call function
		if (function->native != NULL) {
			Value* returnValue = function->native(evaluations);
			functionCallback(returnValue);
		} else {
			startFrame(new BytecodeStackFrame(function->code->bytecodes));
			// TODO evaluate and call function
			// TODO remember to create return reference
			crash("regular functions not implemented yet");
			return Status::OK;		// don't delete evaluations
		}
	} else if (mode == FunctionCallStackFrameMode::RETURN) {
		return endFrame();
	} else {
		crash("unknown FunctionCallStackFrameMode");
	}

	return Status::OK;
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

Status ArgumentStackFrame::execute() {
	if (expressionIterator->hasNext()) {
		Expression* expression = expressionIterator->next();
		evaluateExpression(expression,
		BIND_MEM_CB(&ArgumentStackFrame::evaluationCallback, this));
	} else {
		// out of evaluations
		return endFrame();
	}

	return Status::OK;
}

Status ArgumentStackFrame::evaluationCallback(Value* evaluation) {
	evaluation->createReference();
	evaluations->add(evaluation);
	return Status::OK;
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

Status IfStackFrame::execute() {
	if (mode == IfStackFrameMode::EVALUATE) {
		if (ifsIterator->hasNext()) {
			evaluateExpression(ifsIterator->peekNext()->condition,
			BIND_MEM_CB(&IfStackFrame::conditionEvaluationCallback, this));
			mode = IfStackFrameMode::TEST;
		} else {
			return endFrame();
		}
	} else if (mode == IfStackFrameMode::TEST) {
		if (((BooleanValue*) lastEvaluation)->value) {
			// evaluation is true, run body
			mode = IfStackFrameMode::RUN;
		} else {
			// evaluation is false, go to next condition (else/elseif)
			ifsIterator->next();
			mode = IfStackFrameMode::EVALUATE;
		}
	} else if (mode == IfStackFrameMode::RUN) {
		// test was successful, run and be done
		mode = IfStackFrameMode::DONE;
		startFrame(
				new BytecodeStackFrame(ifsIterator->next()->code->bytecodes));
	} else if (mode == IfStackFrameMode::DONE) {
		// previous call was successful, exit
		return endFrame();
	} else {
		crash("unknown IfStackFrameMode");
	}

	return Status::OK;
}

Status IfStackFrame::conditionEvaluationCallback(Value* value) {
	// delete old value
	if (lastEvaluation != NULL) {
		lastEvaluation->deleteReference();
	}

	// set new value
	lastEvaluation = value;
	value->createReference();

	return Status::OK;
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

Status WhileStackFrame::execute() {
	if (mode == WhileStackFrameMode::EVALUATE) {
		evaluateExpression(condition,
		BIND_MEM_CB(&WhileStackFrame::conditionEvaluationCallback, this));
		mode = WhileStackFrameMode::TEST;
	} else if (mode == WhileStackFrameMode::TEST) {
		if (((BooleanValue*) lastEvaluation)->value) {
			mode = WhileStackFrameMode::RUN;
		} else {
			return endFrame();
		}
	} else if (mode == WhileStackFrameMode::RUN) {
		startFrame(new BytecodeStackFrame(code->bytecodes));
		mode = WhileStackFrameMode::EVALUATE;
	} else {
		crash("unknown WhileStackFrameMode");
	}

	return Status::OK;
}

Status WhileStackFrame::conditionEvaluationCallback(Value* value) {
	// delete old value
	if (lastEvaluation != NULL) {
		lastEvaluation->deleteReference();
	}

	// set new value
	lastEvaluation = value;
	value->createReference();

	return Status::OK;
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

Status execute(ExecuterState* state) {
	return state->executionStack->peek()->execute();
}

void execute(Code* code) {
	ExecuterState* state = new ExecuterState();

	// start executing on first bytecode
	// TODO change this so that execution can resume and exit after every cycle
	BytecodeStackFrame* firstFrame = new BytecodeStackFrame(code->bytecodes);
	firstFrame->state = state;
	state->executionStack->push(firstFrame);

	while (true) {
		Status status = execute(state);
		if (status == Status::DONE) {
			break;
		}
	}

	delete state;
}

}

}
