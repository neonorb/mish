/*
 * expression.h
 *
 *  Created on: Jun 6, 2016
 *      Author: chris
 */

#ifndef INCLUDE_EXPRESSION_H_
#define INCLUDE_EXPRESSION_H_

enum ValueType {
	VOID_VALUE, STRING_VALUE
};
enum ExpressionType {
	VALUE_EXPRESSION, FUNCTION_EXPRESSION
};

class Value;
class Expression {
public:
	ValueType valueType;
	ExpressionType expressionType;

	Expression(ValueType valueType, ExpressionType expressionType);
	virtual ~Expression();
};

#endif /* INCLUDE_EXPRESSION_H_ */
