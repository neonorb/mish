/*
 * bytecode.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <mish.h>

// bytecode
Bytecode::Bytecode(Type instruction) {
	this->type = instruction;
}

Bytecode::~Bytecode() {
}

// function call
FunctionCallBytecode::FunctionCallBytecode(Function* function,
		List<Expression*>* arguments) :
		Bytecode(Type::FUNC_CALL) {
	this->function = function;
	this->arguments = arguments;
}

FunctionCallBytecode::~FunctionCallBytecode() {
	Iterator<Expression*> iterator = arguments->iterator();
	while (iterator.hasNext()) {
		delete iterator.next();
	}

	delete arguments;
}

// if condition
IfConditionCode::IfConditionCode(Expression* condition, Code* code) {
	this->condition = condition;
	this->code = code;
}

IfConditionCode::~IfConditionCode() {
	delete condition;
	delete code;
}

// if
IfBytecode::IfBytecode() :
		Bytecode(Type::IF) {
	ifs = new List<IfConditionCode*>();
}

IfBytecode::IfBytecode(IfConditionCode* conditionCode) :
		IfBytecode() {
	ifs->add(conditionCode);
}

IfBytecode::~IfBytecode() {
	Iterator<IfConditionCode*> ifsIterator = ifs->iterator();
	while (ifsIterator.hasNext()) {
		delete ifsIterator.next();
	}
	delete ifs;
}

// while
WhileBytecode::WhileBytecode(Expression* condition, Code* code, bool isDoWhile) :
		Bytecode(Type::WHILE) {
	this->condition = condition;
	this->code = code;
	this->isDoWhile = isDoWhile;
}

WhileBytecode::~WhileBytecode() {
	delete condition;
	delete code;
}

// set variable
SetVariableBytecode::SetVariableBytecode(VariableDefinition* variable, Expression* value) :
		Bytecode(Type::SET_VARIABLE) {
	this->variable = variable;
	this->value = value;
}

SetVariableBytecode::~SetVariableBytecode() {
	delete value;
}
