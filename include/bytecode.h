/*
 * bytecode.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_BYTECODE_H_
#define INCLUDE_BYTECODE_H_

#include <instruction.h>

class Bytecode {
public:
	Instruction instruction;

	Bytecode(Instruction instruction);
	virtual ~Bytecode();
};

#endif /* INCLUDE_BYTECODE_H_ */
