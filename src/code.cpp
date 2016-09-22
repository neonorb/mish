/*
 * code.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <code.h>

#include <mish.h>
#include <list.h>

Code::Code() {
	bytecodes = new List<Bytecode*>();
	scope = new Scope();
}

Code::~Code() {
	Iterator<Bytecode*> iterator = bytecodes->iterator();
	while (iterator.hasNext()) {
		Bytecode* bytecode = iterator.next();
		delete bytecode;
	}
	delete bytecodes;

	delete scope;
}
