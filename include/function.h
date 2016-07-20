/*
 * function.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_FUNCTION_H_
#define INCLUDE_FUNCTION_H_

#include <code.h>
#include <list.h>
#include <expression.h>
#include <value.h>

typedef Value* (*FunctionHandler)(List<Value*>* arguments);

class Function {
private:
	FunctionHandler handler;
public:
	String name;

	Function(String name, FunctionHandler handler);
	Function* destroy();

	Value* call(List<Value*>* arguments);
};

#endif /* INCLUDE_FUNCTION_H_ */
