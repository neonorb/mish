/*
 * bytecode.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_BYTECODE_H_
#define INCLUDE_BYTECODE_H_

class WhileBytecode;

#include <instruction.h>

class Bytecode {
public:
	Instruction instruction;

	Bytecode(Instruction instruction);
	virtual ~Bytecode();
};

#include <mish.h>

class WhileBytecode: public Bytecode {
public:
	List<Expression*>* condition;
	Code* code;

	WhileBytecode(List<Expression*>* condition, Code* code);
	virtual ~WhileBytecode();
};

#endif /* INCLUDE_BYTECODE_H_ */
