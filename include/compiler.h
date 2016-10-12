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

enum ParseMode {
	EXPECT_STATEMENT,
	EXPECT_STATEMENT_TERMINATOR,
	SYMBOL,
	SYMBOL_READY,
	FUNCTION,
	STRING,
	PARENTHISIS,
	COMMENT,
	LOOP,
	EXPECT_BLOCK,
	EXPRESSION,
	REQUIRE_CLOSE_EXPRESSION
};

class CompilerState {
public:
	CompilerState();
	~CompilerState();

	Stack<Code*>* codeStack;

	Stack<ParseMode>* mode;

	bool requireExpression;
	Stack<List<Expression*>*>* argumentsStack;

	List<strchar>* symbol;
	Stack<String>* symbolStack;

	List<strchar>* string;bool escaping;

	Stack<ConditionalBytecodeType>* conditionalTypeStack;
};

Code* mish_compile(String code);
Code* mish_compile(String start, feta::size size);

#endif /* INCLUDE_COMPILER_H_ */
