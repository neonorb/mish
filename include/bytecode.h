/*
 * bytecode.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_BYTECODE_H_
#define INCLUDE_BYTECODE_H_

class ConditionalBytecode;

#include <instruction.h>

class Bytecode {
public:
	Instruction instruction;

	Bytecode(Instruction instruction);
	virtual ~Bytecode();
};

enum ConditionalBytecodeType {
	WHILE_CONDITIONALTYPE, IF_CONDITIONALTYPE, DOWHILE_CONDITIONALTYPE
};

#include <mish.h>

class ConditionalBytecode: public Bytecode {
public:
	List<Expression*>* condition;
	Code* code;
	ConditionalBytecodeType type;

	ConditionalBytecode(List<Expression*>* condition, Code* code,
			ConditionalBytecodeType type);
	virtual ~ConditionalBytecode();
};

#endif /* INCLUDE_BYTECODE_H_ */
