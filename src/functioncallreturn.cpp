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
		List<Expression*>* arguments) :
		Expression(function->returnType) {
	this->function = function;
	this->arguments = arguments;
}

FunctionCallReturn::~FunctionCallReturn() {
	Iterator<Expression*> iterator = arguments->iterator();
	while (iterator.hasNext()) {
		delete iterator.next();
	}

	delete arguments;
}

Value* FunctionCallReturn::evaluate() {
	List<Value*> evaluations;

	Iterator<Expression*> expressionIterator = arguments->iterator();
	while (expressionIterator.hasNext()) {
		evaluations.add(expressionIterator.next()->evaluate());
	}

	Value* response = function->call(&evaluations);
	return response;
}
