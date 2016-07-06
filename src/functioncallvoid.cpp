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
		List<Expression*>* arguments) : Bytecode(FUNC_CALL) {
	this->function = function;
	this->arguments = arguments;
}

void FunctionCallVoid::call() {
	List<Value*> returns;

	Iterator<Expression*>* expressionIterator = arguments->iterator();
	Expression* expression;
	while ((expression = expressionIterator->next()) != NULL) {
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

	delete expressionIterator;

	function->call(&returns);

	delete returns.destroy();
}
