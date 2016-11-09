/*
 * compiler.h
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_COMPILE_H_
#define INCLUDE_COMPILE_H_

#include <mish.h>
#include <memory.h>
#include <stack.h>
#include <int.h>
#include <string.h>
#include <callback.h>

using namespace feta;

namespace mish {

namespace compile {

class State;

class Status {
	enum class Type {
		OK, ERROR, WARNING, REPROCESS
	};
	Status(Type type);
	Status(Type type, String message);
public:
	static const Status OK;
	static const Status REPROCESS;

	static Status ERROR(String message) {
		return Status(Type::ERROR, message);
	}
	static Status WARNING(String message) {
		return Status(Type::WARNING, message);
	}

	Type type;
	String message;

	inline bool operator==(Status other) {
		if (type == other.type) {
			return true;
		} else {
			return false;
		}
	}

	inline bool operator!=(Status other) {
		return !other.operator==(*this);
	}
};

// ==== StackFrame =====
class StackFrame {
public:
	enum class Type {
		BODY,
		IF,
		WHILE,
		SYMBOL,
		STRING,
		COMMENT,
		FUNCTION_CALL,
		EXPRESSION,
		ARGUMENT
	};
	StackFrame(Type type);
	virtual ~StackFrame();

	virtual Status processCharacter(strchar c);

	void startFrame(StackFrame* frame);
	template<typename ... Args>
	Status callbackAndEndFrame(Callback<Status(Args...)> callback,
			Args ... args);
	Status endFrame();

	Type type;
	State* state;
};

// ==== BodyStackFrame ====
class BodyStackFrame: public StackFrame {
public:
	enum class Mode {
		READY
	};
	BodyStackFrame(Callback<Status(Code*)> callback);
	~BodyStackFrame();

	Status processCharacter(strchar c);
	Status bytecodeCallback(Bytecode* bytecode);
	Status symbolCallback(String symbol);

	Mode mode;
	Code* code;
	bool lastWasTerminated;
	Callback<Status(Code*)> codeCallback;
};

// ==== IfStackFrame ====
class IfStackFrame: public StackFrame {
public:
	enum class Mode {
		EXPECT_P, EXPECT_BODY
	};
	enum class Type {
		IF, ELSEIF, ELSE
	};
private:
	IfStackFrame(Type type);
public:
	IfStackFrame(Callback<Status(IfBytecode*)> ifBytecodeCallback);
	IfStackFrame(Type type, IfBytecode* ifBytecode);
	~IfStackFrame();

	Status processCharacter(strchar c);
	Status conditionCallback(List<Expression*>* condition);
	Status codeCallback(Code* code);

	Mode mode;
	Type type;
	Expression* condition;
	Callback<Status(IfBytecode*)> ifBytecodeCallback;
	IfBytecode* ifBytecode;
};

// ==== WhileStackFrame ====
class WhileStackFrame: public StackFrame {
public:
	enum class Mode {
		EXPECT_P, EXPECT_BODY
	};
	WhileStackFrame(bool isDoWhile,
			Callback<Status(WhileBytecode*)> whileBytecodeCallback);
	~WhileStackFrame();

	Status processCharacter(strchar c);
	Status conditionCallback(List<Expression*>* condition);
	Status codeCallback(Code* code);

	Mode mode;
	bool isDoWhile;
	Expression* condition;
	Callback<Status(WhileBytecode*)> whileBytecodeCallback;
};

// ==== SymbolStackFrame ====
class SymbolStackFrame: public StackFrame {
public:
	SymbolStackFrame(Callback<Status(String)> symbolCallback);
	~SymbolStackFrame();

	Status processCharacter(strchar c);

	List<strchar>* symbol;
	Callback<Status(String)> symbolCallback;
};

// ==== StringStackFrame ====
class StringStackFrame: public StackFrame {
public:
	StringStackFrame(Callback<Status(String)> stringCallback);
	~StringStackFrame();

	Status processCharacter(strchar c);

	List<strchar>* string;
	bool escaping;
	Callback<Status(String)> stringCallback;
};

// ==== CommentStackFrame ====
class CommentStackFrame: public StackFrame {
public:
	enum class Type {
		LINE
	};
	CommentStackFrame(Type type);
	~CommentStackFrame();

	Status processCharacter(strchar c);

	Type type;
};

// ==== FunctionCallStackFrame ====
class FunctionCallStackFrame: public StackFrame {
public:
	enum class Mode {
		EXPECT_P, ARGUMENTS
	};
	enum class Type {
		BYTECODE, EXPRESSION
	};
private:
	FunctionCallStackFrame(String name, Type type);
public:
	FunctionCallStackFrame(String name,
			Callback<Status(FunctionCallBytecode*)> functionCallBytecodeCallback);
	FunctionCallStackFrame(String name,
			Callback<Status(FunctionCallExpression*)> functionCallExpressionCallback);
	~FunctionCallStackFrame();

	Status processCharacter(strchar c);
	Status argumentsCallback(List<Expression*>* expression);

	Type type;
	String name;
	Mode mode;
	Callback<Status(FunctionCallBytecode*)> functionCallBytecodeCallback;
	Callback<Status(FunctionCallExpression*)> functionCallExpressionCallback;
};

// ==== ExpressionStackFrame ====
class ExpressionStackFrame: public StackFrame {
public:
	enum class Mode {
		READY
	};
	ExpressionStackFrame(bool hasParenthesis,
			Callback<Status(Expression*)> expressionCallback);
	~ExpressionStackFrame();

	Status processCharacter(strchar c);
	Status symbolCallback(String symbol);
	Status stringCallback(String string);
	Status subexpressionCallback(Expression* expression);
	Status functionCallback(FunctionCallExpression* funcCall);

	bool hasParenthesis;
	Callback<Status(Expression*)> expressionCallback;
	Mode mode;
};

// ==== ArgumentsStackFrame ====
class ArgumentsStackFrame: public StackFrame {
public:
	ArgumentsStackFrame(Callback<Status(List<Expression*>*)> argumentsCallback);
	~ArgumentsStackFrame();

	Status processCharacter(strchar c);
	Status argumentCallback(Expression* argument);

	Callback<Status(List<Expression*>*)> argumentsCallback;
	List<Expression*>* arguments;

	bool requireArgument;
};

class State {
public:
	State();
	~State();

	Stack<StackFrame*>* compilerStack;
};

template<typename ... Args>
Status StackFrame::callbackAndEndFrame(Callback<Status(Args...)> callback,
		Args ... args) {
	StackFrame* stackFrame = state->compilerStack->pop();
	callback(args...);
	delete stackFrame;
	return Status::OK;
}

Code* compile(String code);
Code* compile(String start, feta::size size);

}

}

#endif /* INCLUDE_COMPILE_H_ */