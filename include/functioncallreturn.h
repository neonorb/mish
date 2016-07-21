/*
 * functioncallreturn.h
 *
 *  Created on: Jun 6, 2016
 *      Author: chris
 */

#ifndef FUNCTIONCALLRETURN_H_
#define FUNCTIONCALLRETURN_H_

#include <bytecode.h>
#include <function.h>
#include <list.h>
#include <expression.h>
#include <value.h>

class FunctionCallReturn: Expression {
public:
	FunctionCallReturn(Function* function, List<Expression*>* arguments);
	~FunctionCallReturn();

	Function* function;
	List<Expression*>* arguments;

	Value* call();
};

#endif /* FUNCTIONCALLRETURN_H_ */
