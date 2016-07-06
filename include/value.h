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
	Value(ValueType type);
	ValueType type;
};

class StringValue: Value {
public:
	StringValue(String value);
	String value;
};

#endif /* INCLUDE_VALUE_H_ */
