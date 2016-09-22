/*
 * value.cpp
 *
 *  Created on: Jun 7, 2016
 *      Author: chris
 */

#include <value.h>
#include <memory.h>

// value
Value::Value(ValueType type) :
		Expression(type, VALUE_EXPRESSION) {
	isConstant = false;
}

Value::~Value() {
}

// string
StringValue::StringValue(String value) :
		Value(STRING_VALUE) {
	this->value = value;
}

StringValue::StringValue(String value, bool isConstant) :
		StringValue(value) {
	this->isConstant = isConstant;
}

StringValue::~StringValue() {
	delete value;
}

// boolean
BooleanValue::BooleanValue(bool value) :
		Value(BOOLEAN_VALUE) {
	this->value = value;
}

BooleanValue::BooleanValue(bool value, bool isConstant) :
		BooleanValue(value) {
	this->isConstant = isConstant;
}

BooleanValue::~BooleanValue() {
}
