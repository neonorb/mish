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
#include <log.h>

class Scope;
class Code {
public:
	~Code();

	List<Bytecode*> bytecodes;
	Scope* scope;
};

#endif /* INCLUDE_CODE_H_ */
