/*
 * bytecode.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_BYTECODE_H_
#define INCLUDE_BYTECODE_H_

class Bytecode;
class FunctionCallVoid;
class IfConditionCode;
class IfBytecode;
class WhileBytecode;

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

class FunctionCallVoid: public Bytecode {
public:
	FunctionCallVoid(Function* function, List<Expression*>* arguments);
	virtual ~FunctionCallVoid();

	Function* function;
	List<Expression*>* arguments;
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
	IfBytecode(IfConditionCode* conditionCode);
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
