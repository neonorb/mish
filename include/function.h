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
public:
	String name;
	List<ValueType>* parameterTypes;
	ValueType returnType;

	Function(String name, FunctionHandler native, List<ValueType>* parameterTypes, ValueType returnType);
	~Function();

	FunctionHandler native;
};

#endif /* INCLUDE_FUNCTION_H_ */
