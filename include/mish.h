/*
 * mish.h
 *
 *  Created on: May 3, 2016
 *      Author: chris
 */

#ifndef INCLUDE_MISH_H_
#define INCLUDE_MISH_H_

#include <list.h>
#include <string.h>
#include <memory.h>

enum Instruction {
	PRINTHI, PRINTBOB
};

class Bytecode: Deleteable {
public:
	Instruction instruction;
	Bytecode(Instruction instruction);
};

class Function: Deleteable {
public:
	void destroy();
	List<Bytecode*> bytecodes;
};

void mish_execute(String code);
void execute(Function* function);

#endif /* INCLUDE_MISH_H_ */
