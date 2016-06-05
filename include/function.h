/*
 * function.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_FUNCTION_H_
#define INCLUDE_FUNCTION_H_

#include <code.h>

class Code;
class Function {
public:
	Code* code;

	Function* destroy();
};

#endif /* INCLUDE_FUNCTION_H_ */
