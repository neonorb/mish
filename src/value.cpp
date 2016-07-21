/*
 * value.cpp
 *
 *  Created on: Jun 7, 2016
 *      Author: chris
 */

#include <value.h>
#include <memory.h>

Value::Value(ValueType type) :
		Expression(VALUE_EXPRESSION) {
	this->type = type;
}

Value::~Value() {
}

StringValue::StringValue(String value) :
		Value(STRING_VALUE) {
	this->value = value;
}

StringValue::~StringValue() {
	delete value;
}
