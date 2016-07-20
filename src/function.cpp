/*
 * function.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <function.h>

Function::Function(String name, FunctionHandler handler) {
	this->name = name;
	this->handler = handler;
}

Function* Function::destroy() {
	return this;
}

Value* Function::call(List<Value*>* arguments) {
	return handler(arguments);
}
