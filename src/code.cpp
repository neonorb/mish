/*
 * code.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <code.h>
#include <functioncallvoid.h>

#include <memory.h>
#include <mish.h>
#include <string.h>
#include <instruction.h>
#include <list.h>
#include <value.h>

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
		case FUNC_CALL:
			FunctionCallVoid* functionCallVoid = (FunctionCallVoid*) bytecode;
			functionCallVoid->call();

			break;
		}
	}

	delete iterator;
}
