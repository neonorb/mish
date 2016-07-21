/*
 * expression.h
 *
 *  Created on: Jun 6, 2016
 *      Author: chris
 */

#ifndef INCLUDE_EXPRESSION_H_
#define INCLUDE_EXPRESSION_H_

enum ExpressionType {
	FUNCTION_EXPRESSION, VALUE_EXPRESSION
};

class Expression {
public:
	ExpressionType type;

	Expression(ExpressionType type);
	virtual ~Expression();
};

#endif /* INCLUDE_EXPRESSION_H_ */
