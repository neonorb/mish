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

class Code;
class Function {
public:
	Code* code;

	Function* destroy();

	Value* call(List<Value*>* arguments);
};

#endif /* INCLUDE_FUNCTION_H_ */
