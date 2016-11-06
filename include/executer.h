/*
 * executer.h
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_EXECUTER_H_
#define INCLUDE_EXECUTER_H_

namespace mish {
namespace execute {

class ExecuterState;

}
}

#include <mish.h>
#include <stack.h>
#include <callback.h>

using namespace feta;

namespace mish {

namespace execute {

enum class Status {
	DONE, OK
};

// frame
enum class ExecutionStackFrameType {
	BYTECODE, FUNCTION_CALL, ARGUMENT, IF, WHILE
};
class ExecutionStackFrame {
public:
	ExecutionStackFrame(ExecutionStackFrameType type);
	virtual ~ExecutionStackFrame();

	ExecutionStackFrameType type;
};

// frame types
class BytecodeStackFrame: public ExecutionStackFrame {
public:
	BytecodeStackFrame(List<Bytecode*>* bytecodesIterator);
	~BytecodeStackFrame();

	Status functionCallCallback(Value* ret);

	Iterator<Bytecode*>* bytecodesIterator;
};

// function call
enum class FunctionCallStackFrameMode {
	EVALUATE, CALL, RETURN
};
class FunctionCallStackFrame: public ExecutionStackFrame {
public:
	FunctionCallStackFrame(Function* function, List<Expression*>* arguments,
			Callback<Status(Value*)> response);
	~FunctionCallStackFrame();

	FunctionCallStackFrameMode mode;

	Function* function;
	List<Expression*>* arguments;
	List<Value*>* evaluations;
	Callback<Status(Value*)> response;
};

// argument
class ArgumentStackFrame: public ExecutionStackFrame {
public:
	ArgumentStackFrame(List<Expression*>* expressions,
			List<Value*>* evaluations);
	~ArgumentStackFrame();

	Status evaluationCallback(Value* evaluation);

	Iterator<Expression*>* expressionIterator;
	List<Value*>* evaluations;
};

// if
enum class IfStackFrameMode {
	EVALUATE, TEST, RUN, DONE
};
class IfStackFrame: public ExecutionStackFrame {
public:
	IfStackFrame(List<IfConditionCode*>* ifs);
	~IfStackFrame();

	Status conditionEvaluationCallback(Value* value);

	IfStackFrameMode mode;

	Iterator<IfConditionCode*>* ifsIterator;

	Value* lastEvaluation;
};

// while
enum class WhileStackFrameMode {
	EVALUATE, TEST, RUN
};
class WhileStackFrame: public ExecutionStackFrame {
public:
	WhileStackFrame(Expression* condition, Code* code, bool isDoWhile);
	~WhileStackFrame();

	Status conditionEvaluationCallback(Value* value);

	WhileStackFrameMode mode;

	Expression* condition;
	Code* code;

	Value* lastEvaluation;
};

// state
class ExecuterState {
public:
	ExecuterState();
	~ExecuterState();

	Stack<ExecutionStackFrame*>* executionStack;
};

void mish_execute(Code* code);
Status mish_execute(ExecuterState* state);

}

}

#endif /* INCLUDE_EXECUTER_H_ */
