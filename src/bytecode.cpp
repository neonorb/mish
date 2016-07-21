/*
 * bytecode.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <bytecode.h>
#include <functioncallvoid.h>

Bytecode::Bytecode(Instruction instruction) {
	this->instruction = instruction;
}

Bytecode::~Bytecode() {
}
