/*
 * state.h
 *
 *  Created on: Jun 6, 2016
 *      Author: chris
 */

#ifndef INCLUDE_STATE_H_
#define INCLUDE_STATE_H_

#include <list.h>
#include <string.h>

using namespace feta;

enum class ExpressionType {
	VALUE, FUNCTION
};

#undef VOID
class Class;
class ValueType {
	enum class Type {
		UNKNOWN, VOID, BOOLEAN, STRING, CLASS
	};
	ValueType(Type type);
	ValueType(Class* clazz);
public:
	ValueType();
	static const ValueType UNKNOWN;
	static const ValueType VOID;
	static const ValueType BOOLEAN;
	static const ValueType STRING;

	static ValueType CLASS(Class* clazz) {
		return ValueType(clazz);
	}

	Type type;
	Class* clazz;

	inline bool operator==(ValueType other) {
		if (type == other.type) {
			if (type == Type::CLASS) {
				return clazz == other.clazz;
			} else {
				return true;
			}
		} else {
			return false;
		}
	}

	inline bool operator!=(ValueType other) {
		return !operator==(other);
	}
};

class VariableDefinition {
public:
	VariableDefinition(ValueType type, String name);
	~VariableDefinition();

	ValueType type;
	String name;
};

class Class {
public:
	Class(String name, List<VariableDefinition>* variableDefinitions);
	~Class();

	String name;
	List<VariableDefinition>* variableDefinitions;
};

class Value;
class Variable {
public:
	Variable(VariableDefinition* definition, Value* value);
	~Variable();

	void setValue(Value* value);

	VariableDefinition* definition;
	Value* value;
};

// ==== expression ====

class Expression {
public:
	ValueType valueType;
	ExpressionType expressionType;

	Expression(ValueType valueType, ExpressionType expressionType);
	virtual ~Expression();
};

#include <function.h>
class FunctionCallExpression: public Expression {
public:
	FunctionCallExpression(Function* function, List<Expression*>* arguments);
	~FunctionCallExpression();

	Function* function;
	List<Expression*>* arguments;
};

// ==== value ====

class Value: public Expression {
public:
	Value(ValueType type);
	virtual ~Value();

	bool isConstant;

	uint64 referenceCount;
	void createReference();
	void deleteReference();
};

// string
class StringValue: public Value {
public:
	StringValue(String value);
	StringValue(String value, bool isConstant);
	~StringValue();

	String value;
};

// boolean
class BooleanValue: public Value {
public:
	BooleanValue(bool value);
	BooleanValue(bool value, bool isConstant);
	~BooleanValue();

	bool value;
};

// class
class ClassValue: public Value {
public:
	ClassValue(Class* clazz, Scope* scope);
	~ClassValue();

	Scope* scope;
};

#endif /* INCLUDE_STATE_H_ */
