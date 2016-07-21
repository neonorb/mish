/*
 * functioncallreturn.cpp
 *
 *  Created on: Jun 6, 2016
 *      Author: chris
 */

#include <list.h>
#include <expression.h>
#include <function.h>
#include <bytecode.h>
#include <functioncallreturn.h>

FunctionCallReturn::FunctionCallReturn(Function* function,
		List<Expression*>* arguments) : Expression(FUNCTION_EXPRESSION) {
	this->function = function;
	this->arguments = arguments;
}

FunctionCallReturn::~FunctionCallReturn() {
	Iterator<Expression*> iterator = arguments->iterator();
	Expression* expression;
	while ((expression = iterator.next()) != NULL) {
		delete expression;
	}

	delete arguments;
}

Value* FunctionCallReturn::call() {
	List<Value*> returns;

	Iterator<Expression*> expressionIterator = arguments->iterator();
	Expression* expression;
	while ((expression = expressionIterator.next()) != NULL) {
		if (expression->type == FUNCTION_EXPRESSION) {
			FunctionCallReturn* functionCallReturn =
					(FunctionCallReturn*) expression;
			returns.add(functionCallReturn->call());
		} else if (expression->type == VALUE_EXPRESSION) {
			returns.add((Value*) expression);
		} else {
			crash("unknown expression");
		}
	}

	Value* response = function->call(&returns);
	return response;
}
