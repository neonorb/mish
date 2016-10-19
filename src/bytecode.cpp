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

// conditional bytecode
ConditionalBytecode::ConditionalBytecode(List<Expression*>* condition,
		Code* code, ConditionalBytecodeType type) :
		Bytecode(CONDITIONAL_INSTRUCTION) {
	this->condition = condition;
	this->code = code;
	this->type = type;
	if (type == IF_CONDITIONALTYPE) {
		elseifs = new List<ConditionalBytecode*>();
	}
}

ConditionalBytecode::~ConditionalBytecode() {
	// condition
	if (condition->size() != 1) {
		crash(
				"condition has inappropriate size while destructing WhileBytecode");
	}
	delete condition->remove((uint64) 0);
	delete condition;

	// code
	delete code;

	if (elseifs != NULL) {
		// elseifs
		Iterator<ConditionalBytecode*> elseifsIterator = elseifs->iterator();
		while (elseifsIterator.hasNext()) {
			delete elseifsIterator.next();
		}
		delete elseifs;
	}
}
