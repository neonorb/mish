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

class Value;
class Expression {
public:
	ValueType valueType;

	Expression(ValueType valueType);
	virtual ~Expression();

	virtual Value* evaluate();
};

#endif /* INCLUDE_EXPRESSION_H_ */
