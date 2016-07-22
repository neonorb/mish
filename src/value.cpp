/*
 * value.cpp
 *
 *  Created on: Jun 7, 2016
 *      Author: chris
 */

#include <value.h>
#include <memory.h>

Value::Value(ValueType type) :
		Expression(type) {

}

Value::~Value() {
}

Value* Value::evaluate() {
	return this;
}

StringValue::StringValue(String value) :
		Value(STRING_VALUE) {
	this->value = value;
}

StringValue::~StringValue() {
	delete value;
}
