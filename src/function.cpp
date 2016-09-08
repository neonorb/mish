/*
 * function.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <function.h>

Function::Function(String name, FunctionHandler native, List<ValueType>* parameterTypes, ValueType returnType) {
	this->name = name;
	this->native = native;
	this->parameterTypes = parameterTypes;
	this->returnType = returnType;
}

Function::~Function() {
	delete parameterTypes;
}
