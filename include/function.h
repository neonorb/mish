/*
 * function.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_FUNCTION_H_
#define INCLUDE_FUNCTION_H_

class Function;

#include <code.h>
#include <mish.h>
#include <list.h>

typedef Value* (*FunctionHandler)(List<Value*>* arguments);

class Function {
public:
	String name;
	List<ValueType>* parameterTypes;
	ValueType returnType;

	FunctionHandler native;
	Code* code;

	Function(String name, List<ValueType>* parameterTypes,
			ValueType returnType, Code* code, FunctionHandler native);
	Function(String name, List<ValueType>* parameterTypes,
			ValueType returnType, Code* code);
	Function(String name, List<ValueType>* parameterTypes,
			ValueType returnType, FunctionHandler native);
	~Function();
};

#endif /* INCLUDE_FUNCTION_H_ */
