/*
 * executer.h
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_EXECUTE_H_
#define INCLUDE_EXECUTE_H_

namespace mish {
namespace execute {

class State;

}
}

#include <mish.h>

using namespace feta;

namespace mish {
namespace execute {

enum class Status {
	DONE, OK
};

// ==== StackFrame ====
class StackFrame {
public:
	enum class Type {
		BYTECODE, FUNCTION_CALL, ARGUMENT, IF, WHILE, SET_VARIABLE
	};
	StackFrame(Type type);
	virtual ~StackFrame();

	virtual void init();
	virtual Status execute();

	void startFrame(StackFrame* frame);
	template<typename ... Args>
	Status callbackAndEndFrame(Callback<Status(Args...)> callback,
			Args ... args);
	Status endFrame();
	void evaluateExpression(Expression* expression,
			Callback<Status(Value*)> response);
	Variable* getVariable(VariableDefinition* vDef);

	Type type;
	State* state;
	Variable** variables;
};

// ==== BodyStackFrame ====
class BodyStackFrame: public StackFrame {
public:
	BodyStackFrame(Code* code);
	~BodyStackFrame();

	Status execute();
	Status functionCallCallback(Value* ret);

	Iterator<Bytecode*>* bytecodesIterator;
	uinteger varCount;
};

// ==== FunctionCallStackFrame ====
class FunctionCallStackFrame: public StackFrame {
public:
	enum class FunctionCallStackFrameMode {
		EVALUATE, CALL, RETURN
	};
	FunctionCallStackFrame(Function* function, List<Expression*>* arguments,
			Callback<Status(Value*)> response);
	~FunctionCallStackFrame();

	Status execute();

	FunctionCallStackFrameMode mode;

	Function* function;
	List<Expression*>* arguments;
	List<Value*>* evaluations;
	Callback<Status(Value*)> functionCallback;
};

// ==== ArgumentStackFrame ====
class ArgumentStackFrame: public StackFrame {
public:
	ArgumentStackFrame(List<Expression*>* expressions,
			List<Value*>* evaluations);
	~ArgumentStackFrame();

	Status execute();
	Status evaluationCallback(Value* evaluation);

	Iterator<Expression*>* expressionIterator;
	List<Value*>* evaluations;
};

// ==== IfStackFrame ====
class IfStackFrame: public StackFrame {
public:
	enum class Mode {
		EVALUATE, TEST, RUN, DONE
	};
	IfStackFrame(List<IfConditionCode*>* ifs);
	~IfStackFrame();

	Status execute();
	Status conditionEvaluationCallback(Value* value);

	Mode mode;

	Iterator<IfConditionCode*>* ifsIterator;

	Value* lastEvaluation;
};

// ==== WhileStackFrame ====
class WhileStackFrame: public StackFrame {
public:
	enum class Mode {
		EVALUATE, TEST, RUN
	};
	WhileStackFrame(Expression* condition, Code* code, bool isDoWhile);
	~WhileStackFrame();

	Status execute();
	Status conditionEvaluationCallback(Value* value);

	Mode mode;

	Expression* condition;
	Code* code;

	Value* lastEvaluation;
};

// ==== SetVariableStackFrame ====
class SetVariableStackFrame: public StackFrame {
public:
	SetVariableStackFrame(Variable* variable, Expression* value);

	void init();
	Status execute();
	Status valueEvaluationCallback(Value* value);

	Variable* variable;
	Expression* value;
};

// ==== State ====
class State {
public:
	State();
	~State();

	Stack<StackFrame*>* executionStack;
};

void execute(Code* code);
Status execute(State* state);

}

}

#endif /* INCLUDE_EXECUTE_H_ */
