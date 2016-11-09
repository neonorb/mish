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

// IfConditionCode
IfConditionCode::IfConditionCode(Expression* condition, Code* code) {
	this->condition = condition;
	this->code = code;
}

IfConditionCode::~IfConditionCode() {
	delete condition;
	delete code;
}

// IfBytecode
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

// WhileBytecode
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
