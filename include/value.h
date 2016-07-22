/*
 * value.h
 *
 *  Created on: Jun 7, 2016
 *      Author: chris
 */

#ifndef INCLUDE_VALUE_H_
#define INCLUDE_VALUE_H_

#include <string.h>
#include <expression.h>

class Value: public Expression {
public:
	Value(ValueType type);
	virtual ~Value();

	Value* evaluate();
};

class StringValue: Value {
public:
	StringValue(String value);
	~StringValue();

	String value;
};

#endif /* INCLUDE_VALUE_H_ */
