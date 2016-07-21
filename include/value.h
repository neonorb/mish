/*
 * value.h
 *
 *  Created on: Jun 7, 2016
 *      Author: chris
 */

#ifndef INCLUDE_VALUE_H_
#define INCLUDE_VALUE_H_

#include <expression.h>
#include <string.h>

enum ValueType {
	STRING_VALUE
};

class Value: Expression {
public:
	ValueType type;

	Value(ValueType type);
	virtual ~Value();
};

class StringValue: Value {
public:
	StringValue(String value);
	~StringValue();

	String value;
};

#endif /* INCLUDE_VALUE_H_ */
