/*
 * function.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <function.h>

Function* Function::destroy() {
	code->destroy();
	return this;
}

Value* Function::call(List<Value*>* arguments){
	// TODO
}
