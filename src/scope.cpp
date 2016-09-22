/*
 * scope.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <scope.h>
#include <memory.h>

Scope::Scope() {
	parent = NULL;
}

Scope::~Scope() {
	Iterator<Function*> iterator = functions.iterator();
	while (iterator.hasNext()) {
		delete iterator.next();
	}
	functions.clear();
}
