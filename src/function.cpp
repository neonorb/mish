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
