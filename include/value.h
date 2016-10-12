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

using namespace feta;

// value
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

#endif /* INCLUDE_VALUE_H_ */
