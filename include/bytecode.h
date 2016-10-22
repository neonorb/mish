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
	WHILE_CONDITIONALTYPE, IF_CONDITIONALTYPE, ELSEIF_CONDITIONALTYPE, DOWHILE_CONDITIONALTYPE
};

#include <mish.h>

class ConditionalBytecode: public Bytecode {
public:
	List<Expression*>* condition;
	Code* code;
	ConditionalBytecodeType type;
	List<ConditionalBytecode*>* elseifs;

	ConditionalBytecode(List<Expression*>* condition, Code* code,
			ConditionalBytecodeType type);
	virtual ~ConditionalBytecode();
};

class WhileBytecode: public Bytecode {
public:
	Expression* condition;
	Code* code;
	bool isDoWhile;

	WhileBytecode(Expression* condition, Code* code, bool isDoWhile);
};

#endif /* INCLUDE_BYTECODE_H_ */
