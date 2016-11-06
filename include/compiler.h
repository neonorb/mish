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
		return !(other operator==(this);
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
	CompilerStackFrame(CompilerStackFrameType type, CompilerState* state);
	virtual ~CompilerStackFrame();

	virtual Status processCharacter(strchar c);

	CompilerStackFrameType type;
	CompilerState* state;
};

// BodyCompilerStackFrame
enum class BodyCompilerStackFrameMode {
	READY, SYMBOL
};
class BodyCompilerStackFrame: public CompilerStackFrame {
public:
	BodyCompilerStackFrame(Callback<void(Code*)> callback,
			CompilerState* state);
	~BodyCompilerStackFrame();

	Status processCharacter(strchar c);
	void addBytecode(Bytecode* bytecode);
	void setSymbol(String symbol);

	BodyCompilerStackFrameMode mode;
	String symbol;
	Code* code;
	bool lastWasTerminated;
	Callback<void(Code*)> codeCallback;
};

// IfCompilerStackFrame
enum class IfCompilerStackFrameMode {
	EXPECT_P, EXPECT_CONDITION, EXPECT_CP, EXPECT_BODY, DONE
};
enum class IfCompilerStackFrameType {
	IF, ELSEIF, ELSE
};
class IfCompilerStackFrame: public CompilerStackFrame {
private:
	IfCompilerStackFrame(IfCompilerStackFrameType type, CompilerState* state);
public:
	IfCompilerStackFrame(Callback<void(IfBytecode*)> ifBytecodeCallback,
			CompilerState* state);
	IfCompilerStackFrame(IfCompilerStackFrameType type, IfBytecode* ifBytecode,
			CompilerState* state);
	~IfCompilerStackFrame();

	Status processCharacter(strchar c);
	void setCondition(Expression* condition);
	void setCode(Code* code);

	IfCompilerStackFrameMode mode;
	IfCompilerStackFrameType type;
	Expression* condition;
	Code* code;
	Callback<void(IfBytecode*)> ifBytecodeCallback;
	IfBytecode* ifBytecode;
};

// WhileCompilerStackFrame
enum class WhileCompilerStackFrameMode {
	EXPECT_P, EXPECT_CONDITION, EXPECT_CP, EXPECT_BODY, DONE
};
class WhileCompilerStackFrame: public CompilerStackFrame {
public:
	WhileCompilerStackFrame(bool isDoWhile,
			Callback<void(WhileBytecode*)> whileBytecodeCallback,
			CompilerState* state);
	~WhileCompilerStackFrame();

	Status processCharacter(strchar c);
	void setCondition(Expression* condition);
	void setCode(Code* code);

	WhileCompilerStackFrameMode mode;
	bool isDoWhile;
	Expression* condition;
	Code* code;
	Callback<void(WhileBytecode*)> whileBytecodeCallback;
};

// SymbolCompilerStackFrame
class SymbolCompilerStackFrame: public CompilerStackFrame {
public:
	SymbolCompilerStackFrame(Callback<void(String)> symbolCallback,
			CompilerState* state);
	~SymbolCompilerStackFrame();

	Status processCharacter(strchar c);

	List<strchar>* symbol;
	Callback<void(String)> symbolCallback;
};

// StringCompilerStackFrame
class StringCompilerStackFrame: public CompilerStackFrame {
public:
	StringCompilerStackFrame(Callback<void(String)> stringCallback,
			CompilerState* state);
	~StringCompilerStackFrame();

	Status processCharacter(strchar c);

	List<strchar>* string;
	bool escaping;
	Callback<void(String)> stringCallback;
};

// CommentCompilerStackFrame
enum class CommentCompilerStackFrameType {
	LINE
};
class CommentCompilerStackFrame: public CompilerStackFrame {
public:
	CommentCompilerStackFrame(CommentCompilerStackFrameType type,
			CompilerState* state);
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
			FunctionCallCompilerStackFrameType type, CompilerState* state);
public:
	FunctionCallCompilerStackFrame(String name,
			Callback<void(FunctionCallVoid*)> functionCallBytecodeCallback,
			CompilerState* state);
	FunctionCallCompilerStackFrame(String name,
			Callback<void(FunctionCallReturn*)> functionCallExpressionCallback,
			CompilerState* state);
	~FunctionCallCompilerStackFrame();

	Status processCharacter(strchar c);
	void argumentCallback(Expression* expression);

	FunctionCallCompilerStackFrameType type;
	String name;
	List<Expression*>* arguments;
	FunctionCallCompilerStackFrameMode mode;
	Callback<void(FunctionCallVoid*)> functionCallBytecodeCallback;
	Callback<void(FunctionCallReturn*)> functionCallExpressionCallback;
};

// ExpressionCompilerStackFrame
enum class ExpressionCompilerStackFrameMode {
	READY, SYMBOL, DONE
};
class ExpressionCompilerStackFrame: public CompilerStackFrame {
public:
	ExpressionCompilerStackFrame(Callback<void(Expression*)> expressionCallback,
			CompilerState* state);
	~ExpressionCompilerStackFrame();

	Status processCharacter(strchar c);
	void symbolCallback(String symbol);
	void stringCallback(String string);
	void subexpressionCallback(Expression* expression);
	void functionCallback(FunctionCallReturn* funcCall);

	Callback<void(Expression*)> expressionCallback;
	String symbol;
	ExpressionCompilerStackFrameMode mode;
};

// ArgumentCompilerStackFrame
class ArgumentCompilerStackFrame: public CompilerStackFrame {
public:
	ArgumentCompilerStackFrame(Callback<void(Expression*)> argumentCallback,
			CompilerState* state);
	~ArgumentCompilerStackFrame();

	Status processCharacter(strchar c);

	Callback<void(Expression*)> argumentCallback;

	bool requireArgument;
};

class CompilerState {
public:
	CompilerState();
	~CompilerState();

	Stack<CompilerStackFrame*>* compilerStack;
};

Code* mish_compile(String code);
Code* mish_compile(String start, feta::size size);

}

}

#endif /* INCLUDE_COMPILER_H_ */
