/*
 * functioncallvoid.cpp
 *
 *  Created on: Jun 6, 2016
 *      Author: chris
 */

#include <list.h>
#include <expression.h>
#include <function.h>
#include <bytecode.h>
#include <functioncallvoid.h>
#include <functioncallreturn.h>

FunctionCallVoid::FunctionCallVoid(Function* function,
		List<Expression*>* arguments) :
		Bytecode(FUNC_CALL) {
	this->function = function;
	this->arguments = arguments;
}

FunctionCallVoid::~FunctionCallVoid() {
	Iterator<Expression*> iterator = arguments->iterator();
	while (iterator.hasNext()) {
		delete iterator.next();
	}

	delete arguments;
}

void FunctionCallVoid::call() {
	List<Value*> evaluations;

	Iterator<Expression*> expressionIterator = arguments->iterator();
	while (expressionIterator.hasNext()) {
		evaluations.add(expressionIterator.next()->evaluate());
	}

	function->call(&evaluations);
}
