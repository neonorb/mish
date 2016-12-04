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
#include <callback.h>

class Function {
public:
	String name;
	List<ValueType>* parameterTypes;
	ValueType returnType;

	Callback<Value*(List<Value*>*)> native;
	Code* code;

	Function(String name, List<ValueType>* parameterTypes, ValueType returnType,
			Code* code, Callback<Value*(List<Value*>*)> native);
	Function(String name, List<ValueType>* parameterTypes, ValueType returnType,
			Code* code);
	Function(String name, List<ValueType>* parameterTypes, ValueType returnType,
			Callback<Value*(List<Value*>*)> native);
	~Function();
};

#endif /* INCLUDE_FUNCTION_H_ */
