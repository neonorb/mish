/*
 * functioncallreturn.h
 *
 *  Created on: Jun 6, 2016
 *      Author: chris
 */

#ifndef FUNCTIONCALLRETURN_H_
#define FUNCTIONCALLRETURN_H_

class FunctionCallReturn;

#include <bytecode.h>
#include <list.h>
#include <expression.h>
#include <value.h>

class FunctionCallReturn: public Expression {
public:
	FunctionCallReturn(Function* function, List<Expression*>* arguments);
	~FunctionCallReturn();

	Function* function;
	List<Expression*>* arguments;
};

#endif /* FUNCTIONCALLRETURN_H_ */
