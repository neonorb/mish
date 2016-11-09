/*
 * expression.h
 *
 *  Created on: Jun 6, 2016
 *      Author: chris
 */

#ifndef INCLUDE_EXPRESSION_H_
#define INCLUDE_EXPRESSION_H_

class Expression;
enum class ExpressionType {
	VALUE, FUNCTION
};
class FunctionCallExpression;
class Value;

#undef VOID
enum class ValueType {
	VOID, STRING, BOOLEAN
};

#include <list.h>

using namespace feta;

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
class StringValue;
class BooleanValue;

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

#endif /* INCLUDE_EXPRESSION_H_ */
