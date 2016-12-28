/*
 * bytecode.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_BYTECODE_H_
#define INCLUDE_BYTECODE_H_

namespace mish {

class Function;
class Code;
class Bytecode;
class FunctionCallBytecode;
class IfConditionCode;
class IfBytecode;
class WhileBytecode;

}

#include <state.h>

namespace mish {

class Function {
public:
	String name;
	List<ValueType>* parameterTypes;
	ValueType returnType;

	Callback<Value*(List<Value*>*)> native;
	Code* code;

	Function(String name, List<ValueType>* parameterTypes, ValueType returnType,
			Code* code, Callback<Value*(List<Value*>*)> native);
	Function(String name, List<ValueType>* parameterTypes, ValueType returnType,
			Code* code);
	Function(String name, List<ValueType>* parameterTypes, ValueType returnType,
			Callback<Value*(List<Value*>*)> native);
	~Function();
};

class Code {
public:
	Code();
	~Code();

	List<Bytecode*>* bytecodes;
	Scope* scope;
};

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

}

#endif /* INCLUDE_BYTECODE_H_ */
