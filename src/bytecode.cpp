/*
 * bytecode.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <bytecode.h>
#include <functioncallvoid.h>

// bytecode
Bytecode::Bytecode(Instruction instruction) {
	this->instruction = instruction;
}

Bytecode::~Bytecode() {
}

// while bytecode
WhileBytecode::WhileBytecode(List<Expression*>* condition, Code* code) :
		Bytecode(WHILE_INSTRUCTION) {
	this->condition = condition;
	this->code = code;
}
