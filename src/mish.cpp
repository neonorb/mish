/*
 * mish.cpp
 *
 *  Created on: May 3, 2016
 *      Author: chris
 */

#include <mish.h>
#include <log.h>

Bytecode::Bytecode(Instruction instruction) {
	this->instruction = instruction;
}

void Function::destroy() {
	bytecodes.destroy();
}

static List<Function*> functions;

void mish_execute(String code) {
	//compile(code);
}

void execute(Function* function) {
	Iterator<Bytecode*>* iterator = function->bytecodes.iterator();

	Bytecode* bytecode;
	while ((bytecode = iterator->next()) != NULL) {
		switch (bytecode->instruction) {
		case PRINTHI:
			log("HI");
			break;
		case PRINTBOB:
			log("BOB");
			break;
		}
	}

	delete iterator;
}
