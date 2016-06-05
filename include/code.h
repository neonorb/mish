/*
 * code.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_CODE_H_
#define INCLUDE_CODE_H_

#include <list.h>
#include <scope.h>
#include <bytecode.h>

class Scope;
class Code {
public:
	List<Bytecode*> bytecodes;
	Scope* scope;

	Code* destroy();

	void execute();
};

#endif /* INCLUDE_CODE_H_ */
