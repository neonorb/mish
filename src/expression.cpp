/*
 * expression.cpp
 *
 *  Created on: Jun 7, 2016
 *      Author: chris
 */

#include <mish.h>

// ==== expression ====

Expression::Expression(ValueType valueType, ExpressionType expressionType) {
	this->valueType = valueType;
	this->expressionType = expressionType;
}

Expression::~Expression() {
}

FunctionCallExpression::FunctionCallExpression(Function* function,
		List<Expression*>* arguments) :
		Expression(function->returnType, ExpressionType::FUNCTION) {
	this->function = function;
	this->arguments = arguments;
}

FunctionCallExpression::~FunctionCallExpression() {
	Iterator<Expression*> iterator = arguments->iterator();
	while (iterator.hasNext()) {
		delete iterator.next();
	}

	delete arguments;
}

// ==== value ====

// value
Value::Value(ValueType type) :
		Expression(type, ExpressionType::VALUE) {
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
		Value(ValueType::STRING) {
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
		Value(ValueType::BOOLEAN) {
	this->value = value;
}

BooleanValue::BooleanValue(bool value, bool isConstant) :
		BooleanValue(value) {
	this->isConstant = isConstant;
}

BooleanValue::~BooleanValue() {
}
