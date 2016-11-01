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
#include <lambda.h>

using namespace feta;

class CompilerState;

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

	CompilerStackFrameType type;
};

// BodyCompilerStackFrame
typedef Lambda<void*, Code*> CodeCallback;
enum class BodyCompilerStackFrameMode {
	READY, SYMBOL
};
class BodyCompilerStackFrame: public CompilerStackFrame {
public:
	BodyCompilerStackFrame(CodeCallback codeCallback);
	~BodyCompilerStackFrame();

	BodyCompilerStackFrameMode mode;
	String symbol;
	Code* code;
	CodeCallback codeCallback;
	bool lastWasTerminated;
};

// IfCompilerStackFrame
typedef Lambda<void*, IfBytecode*> IfBytecodeCallback;
enum class IfCompilerStackFrameMode {
	EXPECT_P, EXPECT_CONDITION, EXPECT_CP, EXPECT_BODY, DONE
};
enum class IfCompilerStackFrameType {
	IF, ELSEIF, ELSE
};
class IfCompilerStackFrame: public CompilerStackFrame {
private:
	IfCompilerStackFrame(IfCompilerStackFrameType type);
public:
	IfCompilerStackFrame(IfBytecodeCallback ifBytecodeCallback);
	IfCompilerStackFrame(IfCompilerStackFrameType type, IfBytecode* ifBytecode);
	~IfCompilerStackFrame();

	IfCompilerStackFrameMode mode;
	IfCompilerStackFrameType type;
	Expression* condition;
	Code* code;
	IfBytecodeCallback ifBytecodeCallback;
	IfBytecode* ifBytecode;
};

// WhileCompilerStackFrame
typedef Lambda<void*, WhileBytecode*> WhileBytecodeCallback;
enum class WhileCompilerStackFrameMode {
	EXPECT_P, EXPECT_CONDITION, EXPECT_CP, EXPECT_BODY, DONE
};
class WhileCompilerStackFrame: public CompilerStackFrame {
public:
	WhileCompilerStackFrame(bool isDoWhile,
			WhileBytecodeCallback whileBytecodeCallback);
	~WhileCompilerStackFrame();

	WhileCompilerStackFrameMode mode;
	bool isDoWhile;
	Expression* condition;
	Code* code;
	WhileBytecodeCallback whileBytecodeCallback;
};

typedef Lambda<void*, String> StringCallback;

// SymbolCompilerStackFrame
class SymbolCompilerStackFrame: public CompilerStackFrame {
public:
	SymbolCompilerStackFrame(StringCallback symbolCallback);
	~SymbolCompilerStackFrame();

	List<strchar>* symbol;
	StringCallback symbolCallback;
};

// StringCompilerStackFrame
class StringCompilerStackFrame: public CompilerStackFrame {
public:
	StringCompilerStackFrame(StringCallback stringCallback);
	~StringCompilerStackFrame();

	List<strchar>* string;
	bool escaping;
	StringCallback stringCallback;
};

// CommentCompilerStackFrame
enum class CommentCompilerStackFrameType {
	LINE
};
class CommentCompilerStackFrame: public CompilerStackFrame {
public:
	CommentCompilerStackFrame(CommentCompilerStackFrameType type);
	~CommentCompilerStackFrame();

	CommentCompilerStackFrameType type;
};

// FunctionCallCompilerStackFrame
enum class FunctionCallCompilerStackFrameMode {
	EXPECT_P, ARGUMENTS
};
enum class FunctionCallCompilerStackFrameType {
	BYTECODE, EXPRESSION
};
typedef Lambda<void*, FunctionCallVoid*> FunctionCallBytecodeCallback;
typedef Lambda<void*, FunctionCallReturn*> FunctionCallExpressionCallback;
class FunctionCallCompilerStackFrame: public CompilerStackFrame {
private:
	FunctionCallCompilerStackFrame(String name,
			FunctionCallCompilerStackFrameType type);
public:
	FunctionCallCompilerStackFrame(String name,
			FunctionCallBytecodeCallback functionCallBytecodeCallback);
	FunctionCallCompilerStackFrame(String name,
			FunctionCallExpressionCallback functionCallExpressionCallback);
	~FunctionCallCompilerStackFrame();

	FunctionCallCompilerStackFrameType type;
	String name;
	List<Expression*>* arguments;
	FunctionCallCompilerStackFrameMode mode;
	FunctionCallBytecodeCallback functionCallBytecodeCallback;
	FunctionCallExpressionCallback functionCallExpressionCallback;
};

typedef Lambda<void*, Expression*> ExpressionCallback;

// ExpressionCompilerStackFrame
enum class ExpressionCompilerStackFrameMode {
	READY, SYMBOL, DONE
};
class ExpressionCompilerStackFrame: public CompilerStackFrame {
public:
	ExpressionCompilerStackFrame(ExpressionCallback expressionCallback);
	~ExpressionCompilerStackFrame();

	ExpressionCallback expressionCallback;
	String symbol;
	ExpressionCompilerStackFrameMode mode;
};
class ExpressionCompilerStackFrameStringCallbackStruct {
public:
	ExpressionCompilerStackFrameStringCallbackStruct(ExpressionCompilerStackFrame* stackFrame, CompilerState* state);
	ExpressionCompilerStackFrame* stackFrame;
	CompilerState* state;
};

// ArgumentCompilerStackFrame
class ArgumentCompilerStackFrame: public CompilerStackFrame {
public:
	ArgumentCompilerStackFrame(ExpressionCallback argumentCallback);
	~ArgumentCompilerStackFrame();

	ExpressionCallback argumentCallback;

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

#endif /* INCLUDE_COMPILER_H_ */
