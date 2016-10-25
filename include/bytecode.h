/*
 * bytecode.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_BYTECODE_H_
#define INCLUDE_BYTECODE_H_

class Bytecode;
class IfConditionCode;

#include <mish.h>

enum class BytecodeType {
	FUNC_CALL, IF, WHILE
};

class Bytecode {
public:
	BytecodeType type;

	Bytecode(BytecodeType instruction);
	virtual ~Bytecode();
};

// if
class IfConditionCode {
public:
	IfConditionCode(Expression* condition, Code* code);
	~IfConditionCode();

	Expression* condition;
	Code* code;
};
class IfBytecode: public Bytecode {
public:
	IfBytecode();
	~IfBytecode();
	List<IfConditionCode*>* ifs;
};

// while
class WhileBytecode: public Bytecode {
public:
	Expression* condition;
	Code* code;
	bool isDoWhile;

	WhileBytecode(Expression* condition, Code* code, bool isDoWhile);
	~WhileBytecode();
};

#endif /* INCLUDE_BYTECODE_H_ */
