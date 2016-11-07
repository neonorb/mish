/*
 * compiler.h
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_COMPILER_H_
#define INCLUDE_COMPILER_H_

#include <mish.h>
#include <memory.h>
#include <stack.h>
#include <int.h>
#include <string.h>
#include <callback.h>

using namespace feta;

namespace mish {

namespace compile {

class CompilerState;

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

// CompilerStackFrame
enum class CompilerStackFrameType {
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
class CompilerStackFrame {
public:
	CompilerStackFrame(CompilerStackFrameType type);
	virtual ~CompilerStackFrame();

	virtual Status processCharacter(strchar c);

	void startFrame(CompilerStackFrame* frame);
	template<typename ... Args>
	Status callbackAndEndFrame(Callback<Status(Args...)> callback,
			Args ... args);
	Status endFrame();

	CompilerStackFrameType type;
	CompilerState* state;
};

// BodyCompilerStackFrame
enum class BodyCompilerStackFrameMode {
	READY
};
class BodyCompilerStackFrame: public CompilerStackFrame {
public:
	BodyCompilerStackFrame(Callback<Status(Code*)> callback);
	~BodyCompilerStackFrame();

	Status processCharacter(strchar c);
	Status bytecodeCallback(Bytecode* bytecode);
	Status symbolCallback(String symbol);

	BodyCompilerStackFrameMode mode;
	Code* code;
	bool lastWasTerminated;
	Callback<Status(Code*)> codeCallback;
};

// IfCompilerStackFrame
enum class IfCompilerStackFrameMode {
	EXPECT_P, EXPECT_BODY
};
enum class IfCompilerStackFrameType {
	IF, ELSEIF, ELSE
};
class IfCompilerStackFrame: public CompilerStackFrame {
private:
	IfCompilerStackFrame(IfCompilerStackFrameType type);
public:
	IfCompilerStackFrame(Callback<Status(IfBytecode*)> ifBytecodeCallback);
	IfCompilerStackFrame(IfCompilerStackFrameType type, IfBytecode* ifBytecode);
	~IfCompilerStackFrame();

	Status processCharacter(strchar c);
	Status conditionCallback(List<Expression*>* condition);
	Status codeCallback(Code* code);

	IfCompilerStackFrameMode mode;
	IfCompilerStackFrameType type;
	Expression* condition;
	Callback<Status(IfBytecode*)> ifBytecodeCallback;
	IfBytecode* ifBytecode;
};

// WhileCompilerStackFrame
enum class WhileCompilerStackFrameMode {
	EXPECT_P, EXPECT_BODY
};
class WhileCompilerStackFrame: public CompilerStackFrame {
public:
	WhileCompilerStackFrame(bool isDoWhile,
			Callback<Status(WhileBytecode*)> whileBytecodeCallback);
	~WhileCompilerStackFrame();

	Status processCharacter(strchar c);
	Status conditionCallback(List<Expression*>* condition);
	Status codeCallback(Code* code);

	WhileCompilerStackFrameMode mode;
	bool isDoWhile;
	Expression* condition;
	Callback<Status(WhileBytecode*)> whileBytecodeCallback;
};

// SymbolCompilerStackFrame
class SymbolCompilerStackFrame: public CompilerStackFrame {
public:
	SymbolCompilerStackFrame(Callback<Status(String)> symbolCallback);
	~SymbolCompilerStackFrame();

	Status processCharacter(strchar c);

	List<strchar>* symbol;
	Callback<Status(String)> symbolCallback;
};

// StringCompilerStackFrame
class StringCompilerStackFrame: public CompilerStackFrame {
public:
	StringCompilerStackFrame(Callback<Status(String)> stringCallback);
	~StringCompilerStackFrame();

	Status processCharacter(strchar c);

	List<strchar>* string;
	bool escaping;
	Callback<Status(String)> stringCallback;
};

// CommentCompilerStackFrame
enum class CommentCompilerStackFrameType {
	LINE
};
class CommentCompilerStackFrame: public CompilerStackFrame {
public:
	CommentCompilerStackFrame(CommentCompilerStackFrameType type);
	~CommentCompilerStackFrame();

	Status processCharacter(strchar c);

	CommentCompilerStackFrameType type;
};

// FunctionCallCompilerStackFrame
enum class FunctionCallCompilerStackFrameMode {
	EXPECT_P, ARGUMENTS
};
enum class FunctionCallCompilerStackFrameType {
	BYTECODE, EXPRESSION
};
class FunctionCallCompilerStackFrame: public CompilerStackFrame {
private:
	FunctionCallCompilerStackFrame(String name,
			FunctionCallCompilerStackFrameType type);
public:
	FunctionCallCompilerStackFrame(String name,
			Callback<Status(FunctionCallVoid*)> functionCallBytecodeCallback);
	FunctionCallCompilerStackFrame(String name,
			Callback<Status(FunctionCallReturn*)> functionCallExpressionCallback);
	~FunctionCallCompilerStackFrame();

	Status processCharacter(strchar c);
	Status argumentsCallback(List<Expression*>* expression);

	FunctionCallCompilerStackFrameType type;
	String name;
	FunctionCallCompilerStackFrameMode mode;
	Callback<Status(FunctionCallVoid*)> functionCallBytecodeCallback;
	Callback<Status(FunctionCallReturn*)> functionCallExpressionCallback;
};

// ExpressionCompilerStackFrame
enum class ExpressionCompilerStackFrameMode {
	READY
};
class ExpressionCompilerStackFrame: public CompilerStackFrame {
public:
	ExpressionCompilerStackFrame(bool hasParenthesis,
			Callback<Status(Expression*)> expressionCallback);
	~ExpressionCompilerStackFrame();

	Status processCharacter(strchar c);
	Status symbolCallback(String symbol);
	Status stringCallback(String string);
	Status subexpressionCallback(Expression* expression);
	Status functionCallback(FunctionCallReturn* funcCall);

	bool hasParenthesis;
	Callback<Status(Expression*)> expressionCallback;
	ExpressionCompilerStackFrameMode mode;
};

// ArgumentsCompilerStackFrame
class ArgumentsCompilerStackFrame: public CompilerStackFrame {
public:
	ArgumentsCompilerStackFrame(
			Callback<Status(List<Expression*>*)> argumentsCallback);
	~ArgumentsCompilerStackFrame();

	Status processCharacter(strchar c);
	Status argumentCallback(Expression* argument);

	Callback<Status(List<Expression*>*)> argumentsCallback;
	List<Expression*>* arguments;

	bool requireArgument;
};

class CompilerState {
public:
	CompilerState();
	~CompilerState();

	Stack<CompilerStackFrame*>* compilerStack;
};

template<typename ... Args>
Status CompilerStackFrame::callbackAndEndFrame(
		Callback<Status(Args...)> callback, Args ... args) {
	CompilerStackFrame* stackFrame = state->compilerStack->pop();
	callback(args...);
	delete stackFrame;
	return Status::OK;
}

Code* compile(String code);
Code* compile(String start, feta::size size);

}

}

#endif /* INCLUDE_COMPILER_H_ */
