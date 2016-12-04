/*
 * function.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <function.h>

Function::Function(String name, List<ValueType>* parameterTypes,
		ValueType returnType, Code* code,
		Callback<Value*(List<Value*>*)> native) {
	this->name = name;
	this->parameterTypes = parameterTypes;
	this->returnType = returnType;
	this->code = code;
	this->native = native;
}

Function::Function(String name, List<ValueType>* parameterTypes,
		ValueType returnType, Code* code) :
		Function(name, parameterTypes, returnType, code, { }) {

}

Function::Function(String name, List<ValueType>* parameterTypes,
		ValueType returnType, Callback<Value*(List<Value*>*)> native) :
		Function(name, parameterTypes, returnType, NULL, native) {

}

Function::~Function() {
	delete name;
	delete parameterTypes;

	if (code != NULL) {
		delete code;
	}
}
