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

using namespace feta;

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
	~CompilerStackFrame();

	CompilerStackFrameType type;
};

// BodyCompilerStackFrame
enum class BodyCompilerStackFrameMode {
	READY, EXPECT_TERMINATOR, SYMBOL
};
class BodyCompilerStackFrame: public CompilerStackFrame {
public:
	BodyCompilerStackFrame();
	~BodyCompilerStackFrame();

	BodyCompilerStackFrameMode mode;
	String symbol;
};

// IfCompilerStackFrame
enum class IfCompilerStackFrameMode {
	EXPECT_P, EXPECT_CONDITION, EXPECT_CP, EXPECT_BODY, DONE
};
enum class IfCompilerStackFrameType {
	IF, ELSEIF, ELSE
};
class IfCompilerStackFrame: public CompilerStackFrame {
public:
	IfCompilerStackFrame(IfCompilerStackFrameType type);
	~IfCompilerStackFrame();

	IfCompilerStackFrameMode mode;
	IfCompilerStackFrameType type;
};

// WhileCompilerStackFrame
enum class WhileCompilerStackFrameMode {
	EXPECT_P, EXPECT_CONDITION, EXPECT_CP, EXPECT_BODY, DONE
};
class WhileCompilerStackFrame: public CompilerStackFrame {
public:
	WhileCompilerStackFrame(bool isDoWhile);
	~WhileCompilerStackFrame();

	WhileCompilerStackFrameMode mode;
	bool isDoWhile;
};

typedef void *StringCallback(String);

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
class FunctionCallCompilerStackFrame: public CompilerStackFrame {
public:
	FunctionCallCompilerStackFrame(String name);
	~FunctionCallCompilerStackFrame();

	String name;
};

typedef void *ExpressionCallback(Expression*);

// ExpressionCompilerStackFrame
class ExpressionCompilerStackFrame: public CompilerStackFrame {
public:
	ExpressionCompilerStackFrame(ExpressionCallback expressionCallback);
	~ExpressionCompilerStackFrame();

	ExpressionCallback expressionCallback;
};

// ArgumentCompilerStackFrame
class ArgumentCompilerStackFrame: public CompilerStackFrame {
public:
	ArgumentCompilerStackFrame(ExpressionCallback argumentCallback);
	~ArgumentCompilerStackFrame();

	ExpressionCallback argumentCallback;

	bool requireArgument;
};

enum ParseMode {
	EXPECT_STATEMENT,
	EXPECT_STATEMENT_TERMINATOR,
	SYMBOL,
	SYMBOL_READY,
	FUNCTION,
	STRING,
	PARENTHISIS,
	COMMENT,
	IF,
	WHILE,
	EXPECT_BLOCK,
	EXPRESSION,
	REQUIRE_CLOSE_EXPRESSION
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
