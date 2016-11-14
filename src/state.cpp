/*
 * expression.cpp
 *
 *  Created on: Jun 7, 2016
 *      Author: chris
 */

#include <mish.h>

ValueType::ValueType(Type type) {
	this->type = type;
	this->clazz = NULL;
}

ValueType::ValueType(Class* clazz) :
		ValueType(Type::CLASS) {
	this->clazz = clazz;
}

ValueType::ValueType() {
	type = Type::UNKNOWN;
	clazz = NULL;
}

const ValueType ValueType::UNKNOWN = ValueType(Type::UNKNOWN);
const ValueType ValueType::VOID = ValueType(Type::VOID);
const ValueType ValueType::BOOLEAN = ValueType(Type::BOOLEAN);
const ValueType ValueType::STRING = ValueType(Type::STRING);

// ==== variable definition ====

VariableDefinition::VariableDefinition(ValueType type, String name) {
	this->type = type;
	this->name = name;
}

VariableDefinition::~VariableDefinition() {
	delete name;
}

// ==== class ====

Class::Class(String name, List<VariableDefinition>* variableDefinitions) {
	this->name = name;
	this->variableDefinitions = variableDefinitions;
}

Class::~Class() {
	delete name;
	delete variableDefinitions;
}

// ==== variable ====

Variable::Variable(VariableDefinition* definition, Value* value) {
	this->definition = definition;
	this->value = value;
	value->createReference();
}

Variable::~Variable() {
	value->deleteReference();
}

void Variable::setValue(Value* value) {
	value->createReference();
	this->value->deleteReference();
	this->value = value;

	// TODO trigger events
}

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

// class
ClassValue::ClassValue(Class* clazz, Scope* scope) :
		Value(ValueType::CLASS(clazz)) {
	this->scope = scope;
}

ClassValue::~ClassValue() {
	delete scope;
}
