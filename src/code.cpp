/*
 * code.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <code.h>

#include <memory.h>

Code* Code::destroy() {
	bytecodes.destroy();

	scope->destroy();
	return this;
}

void Code::execute() {
	Iterator<Bytecode*>* iterator = bytecodes.iterator();

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
