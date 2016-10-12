/*
 * functioncallvoid.h
 *
 *  Created on: Jun 6, 2016
 *      Author: chris
 */

#ifndef FUNCTIONCALLVOID_H_
#define FUNCTIONCALLVOID_H_

#include <bytecode.h>
#include <function.h>
#include <list.h>
#include <expression.h>

using namespace feta;

class FunctionCallVoid: Bytecode {
public:
	FunctionCallVoid(Function* function, List<Expression*>* arguments);
	virtual ~FunctionCallVoid();

	Function* function;
	List<Expression*>* arguments;
};

#endif /* FUNCTIONCALLVOID_H_ */
