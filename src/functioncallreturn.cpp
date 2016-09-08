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
		Expression(function->returnType, FUNCTION_EXPRESSION) {
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
