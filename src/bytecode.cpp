/*
 * bytecode.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <bytecode.h>
#include <functioncallvoid.h>

// bytecode
Bytecode::Bytecode(BytecodeType instruction) {
	this->type = instruction;
}

Bytecode::~Bytecode() {
}

// IfConditionCode
IfConditionCode::IfConditionCode(Expression* condition, Code* code) {
	this->condition = condition;
	this->code = code;
}

IfConditionCode::~IfConditionCode() {

}

// IfBytecode
IfBytecode::IfBytecode() :
		Bytecode(BytecodeType::IF) {
	ifs = new List<IfConditionCode*>();
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
		Bytecode(BytecodeType::WHILE) {
	this->condition = condition;
	this->code = code;
	this->isDoWhile = isDoWhile;
}

WhileBytecode::~WhileBytecode() {
	delete condition;
	delete code;
}
