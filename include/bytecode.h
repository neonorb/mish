/*
 * bytecode.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_BYTECODE_H_
#define INCLUDE_BYTECODE_H_

class Bytecode;
class FunctionCallBytecode;
class IfConditionCode;
class IfBytecode;
class WhileBytecode;

#include <state.h>

class Bytecode {
public:
	enum class Type {
		FUNC_CALL, IF, WHILE, SET_VARIABLE
	};
	Bytecode(Type instruction);
	virtual ~Bytecode();

	Type type;
};

class FunctionCallBytecode: public Bytecode {
public:
	FunctionCallBytecode(Function* function, List<Expression*>* arguments);
	~FunctionCallBytecode();

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

// set variable
class SetVariableBytecode: public Bytecode {
public:
	SetVariableBytecode(VariableDefinition* variable, Expression* value);
	~SetVariableBytecode();

	VariableDefinition* variable;
	Expression* value;
};

#endif /* INCLUDE_BYTECODE_H_ */
