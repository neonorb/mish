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
	referenceCount = 0;
}

void Value::createReference() {
	referenceCount++;
}

void Value::deleteReference() {
	referenceCount--;

	if (referenceCount == 0 && !isConstant) {
		// TODO iterate over weak references and set them to default values
		delete this;
	}
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
