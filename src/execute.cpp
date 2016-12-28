/*
 * executer.cpp
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#include <execute.h>
#include <feta.h>

namespace mish {

namespace execute {

// ---- state ----

// ExecutionStackFrame
StackFrame::StackFrame(Type type) {
	this->type = type;
	state = NULL;
	variables = NULL;
}

StackFrame::~StackFrame() {

}

void StackFrame::init() {
}

Status StackFrame::execute() {
	return Status::OK;
}

void StackFrame::startFrame(StackFrame* frame) {
	frame->state = state;
	if (frame->type != Type::BYTECODE) {
		frame->variables = variables;
	}
	state->executionStack->push(frame);
	frame->init();
}

Status StackFrame::endFrame() {
	if (state->executionStack->size() == 1) {
		delete state->executionStack->pop();
		return Status::DONE;
	} else {
		delete state->executionStack->pop();
		return Status::OK;
	}
}

void StackFrame::evaluateExpression(Expression* expression,
		Callback<Status(Value*)> response) {
	switch (expression->expressionType) {
	case ExpressionType::VALUE: {
		Value* constant = (Value*) expression;
		response(constant);
	}
		break;
	case ExpressionType::FUNCTION: {
		FunctionCallExpression* functionCallReturn =
				(FunctionCallExpression*) expression;

		startFrame(
				new FunctionCallStackFrame(functionCallReturn->function,
						functionCallReturn->arguments, response));
	}
		break;
	case ExpressionType::VARIABLE: {
		VariableExpression* variableExpression =
				(VariableExpression*) expression;
		response(variables[variableExpression->definition->index]->value);
	}
		break;
	}
}

// BodyStackFrame
BodyStackFrame::BodyStackFrame(Code* code) :
		StackFrame(Type::BYTECODE) {
	this->bytecodesIterator = new Iterator<Bytecode*>(
			code->bytecodes->iterator());

	List<VariableDefinition*>* vDefs = code->scope->variables;
	variables = new Variable*[vDefs->size()];
	varCount = vDefs->size();
	Iterator<VariableDefinition*> vDefsIterator = vDefs->iterator();
	uinteger index = 0;
	while (vDefsIterator.hasNext()) {
		VariableDefinition* vDef = vDefsIterator.next();
		variables[index] = new Variable(vDef, NULL); // TODO needs value for reference tracking (currently checkfor for NULL, but later will not, maybe?)
		index++;
	}
}

BodyStackFrame::~BodyStackFrame() {
	delete bytecodesIterator;
	for (uinteger i = 0; i < varCount; i++) {
		delete variables[i];
	}
	delete variables;
}

Status BodyStackFrame::functionCallCallback(Value* ret) {
	/* discard the return value */
	delete ret;
	return Status::OK;
}

Status BodyStackFrame::execute() {
	if (bytecodesIterator->hasNext()) {
		Bytecode* bytecode = bytecodesIterator->next();
		if (bytecode->type == Bytecode::Type::FUNC_CALL) {
			FunctionCallBytecode* functionCallVoid =
					(FunctionCallBytecode*) bytecode;

			startFrame(
					new FunctionCallStackFrame(functionCallVoid->function,
							functionCallVoid->arguments,
							BIND_MEM_CB(&BodyStackFrame::functionCallCallback, this)));
		} else if (bytecode->type == Bytecode::Type::IF) {
			IfBytecode* ifBytecode = (IfBytecode*) bytecode;
			startFrame(new IfStackFrame(ifBytecode->ifs));
		} else if (bytecode->type == Bytecode::Type::WHILE) {
			WhileBytecode* whileBytecode = (WhileBytecode*) bytecode;
			startFrame(
					new WhileStackFrame(whileBytecode->condition,
							whileBytecode->code, whileBytecode->isDoWhile));
		} else if (bytecode->type == Bytecode::Type::SET_VARIABLE) {
			SetVariableBytecode* vBytecode = (SetVariableBytecode*) bytecode;
			startFrame(
					new SetVariableStackFrame(
							variables[vBytecode->variable->index],
							vBytecode->value));
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
		StackFrame(Type::FUNCTION_CALL) {
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
			startFrame(new BodyStackFrame(function->code));
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
		StackFrame(Type::ARGUMENT) {
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
		StackFrame(Type::IF) {
	mode = Mode::EVALUATE;

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
	if (mode == Mode::EVALUATE) {
		if (ifsIterator->hasNext()) {
			evaluateExpression(ifsIterator->peekNext()->condition,
			BIND_MEM_CB(&IfStackFrame::conditionEvaluationCallback, this));
			mode = Mode::TEST;
		} else {
			return endFrame();
		}
	} else if (mode == Mode::TEST) {
		if (((BooleanValue*) lastEvaluation)->value) {
			// evaluation is true, run body
			mode = Mode::RUN;
		} else {
			// evaluation is false, go to next condition (else/elseif)
			ifsIterator->next();
			mode = Mode::EVALUATE;
		}
	} else if (mode == Mode::RUN) {
		// test was successful, run and be done
		mode = Mode::DONE;
		startFrame(new BodyStackFrame(ifsIterator->next()->code));
	} else if (mode == Mode::DONE) {
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
		StackFrame(Type::WHILE) {
	this->condition = condition;
	this->code = code;

	if (isDoWhile) {
		mode = Mode::RUN;
	} else {
		mode = Mode::EVALUATE;
	}

	lastEvaluation = NULL;
}

WhileStackFrame::~WhileStackFrame() {
	if (lastEvaluation != NULL) {
		lastEvaluation->deleteReference();
	}
}

Status WhileStackFrame::execute() {
	if (mode == Mode::EVALUATE) {
		evaluateExpression(condition,
		BIND_MEM_CB(&WhileStackFrame::conditionEvaluationCallback, this));
		mode = Mode::TEST;
	} else if (mode == Mode::TEST) {
		if (((BooleanValue*) lastEvaluation)->value) {
			mode = Mode::RUN;
		} else {
			return endFrame();
		}
	} else if (mode == Mode::RUN) {
		startFrame(new BodyStackFrame(code));
		mode = Mode::EVALUATE;
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

// SetVariableStackFrame
SetVariableStackFrame::SetVariableStackFrame(Variable* variable,
		Expression* value) :
		StackFrame(Type::SET_VARIABLE) {
	this->variable = variable;
	this->value = value;
}

void SetVariableStackFrame::init() {
	evaluateExpression(value,
	BIND_MEM_CB(&SetVariableStackFrame::valueEvaluationCallback, this));
}

Status SetVariableStackFrame::execute() {
	NIMPL;
	return Status::OK;
}

Status SetVariableStackFrame::valueEvaluationCallback(Value* value) {
	variable->setValue(value);
	return endFrame();
}

// ExecuterState
State::State() {
	executionStack = new Stack<StackFrame*>();
}

State::~State() {
	while (executionStack->size() > 0) {
		delete executionStack->pop();
	}
	delete executionStack;
}

// ---- execution ----

Status execute(State* state) {
	return state->executionStack->peek()->execute();
}

void execute(Code* code) {
	if(code == NULL) {
		crash("executing NULL code");
	}
	State* state = new State();

	// start executing on first bytecode
	// TODO change this so that execution can resume and exit after every cycle
	BodyStackFrame* firstFrame = new BodyStackFrame(code);
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
