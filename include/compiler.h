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

enum ParseMode {
	EXPECT_STATEMENT,
	EXPECT_STATEMENT_TERMINATOR,
	SYMBOL,
	SYMBOL_READY,
	FUNCTION,
	EXPECT_ARGUMENT,
	STRING,
	PARENTHISIS,
	COMMENT
};

class CompilerState {
public:
	CompilerState();
	~CompilerState();

	Code* code;

	Stack<ParseMode>* mode;

	Stack<List<Expression*>*>* argumentsStack;

	List<wchar_t>* symbol;
	Stack<String>* symbolStack;

	List<wchar_t>* string;bool escaping;
};

#endif /* INCLUDE_COMPILER_H_ */
