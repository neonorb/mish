/*
 * function.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <function.h>

Function::Function(String name, FunctionHandler handler, List<ValueType>* parameterTypes, ValueType returnType) {
	this->name = name;
	this->handler = handler;
	this->parameterTypes = parameterTypes;
	this->returnType = returnType;
}

Function::~Function() {
	delete parameterTypes;
}

Value* Function::call(List<Value*>* arguments) {
	return handler(arguments);
}
