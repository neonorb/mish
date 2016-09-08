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
#include <log.h>

Code::~Code() {
	Iterator<Bytecode*> iterator = bytecodes.iterator();
	while (iterator.hasNext()) {
		delete iterator.next();
	}
}
