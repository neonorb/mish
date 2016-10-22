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

enum class ExecuteStatus {
	DONE, NOT_DONE
};

// frame
enum class ExecutionStackFrameType {
	BYTECODE, FUNCTION_CALL, ARGUMENT, IF, WHILE
};
class ExecutionStackFrame {
public:
	ExecutionStackFrame(ExecutionStackFrameType type);
	~ExecutionStackFrame();

	ExecutionStackFrameType type;
};

// frame types
class BytecodeStackFrame: public ExecutionStackFrame {
public:
	BytecodeStackFrame(List<Bytecode*>* bytecodesIterator);
	~BytecodeStackFrame();

	Iterator<Bytecode*>* bytecodesIterator;
};

// function call
enum class FunctionCallStackFrameMode {
	EVALUATE, CALL, RETURN
};
class FunctionCallStackFrame: public ExecutionStackFrame {
public:
	FunctionCallStackFrame(Function* function, List<Expression*>* arguments);
	~FunctionCallStackFrame();

	FunctionCallStackFrameMode mode;

	Function* function;
	List<Expression*>* arguments;
	List<Value*>* evaluations;
};

// argument
class ArgumentStackFrame: public ExecutionStackFrame {
public:
	ArgumentStackFrame(List<Expression*>* expressions,
			List<Value*>* evaluations);
	~ArgumentStackFrame();

	Iterator<Expression*>* expressionIterator;
	List<Value*>* evaluations;
};

// if
class IfConditionCode {
public:
	IfConditionCode(Expression* condition, Code* code);
	~IfConditionCode();

	Expression* condition;
	Code* code;
};
enum class IfStackFrameMode {
	EVALUATE, TEST, RUN, DONE
};
class IfStackFrame: public ExecutionStackFrame {
public:
	IfStackFrame(List<IfConditionCode*>* ifs);
	~IfStackFrame();

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
ExecuteStatus mish_execute(ExecuterState* state);

#endif /* INCLUDE_EXECUTER_H_ */
