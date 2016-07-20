/*
 * scope.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <scope.h>
#include <memory.h>

Scope* Scope::destroy() {
	Iterator<Function*>* iterator = functions.iterator();
	Function* function;
	while ((function = iterator->next()) != NULL) {
		function->destroy();
		delete function;
	}
	functions.destroy();

	return this;
}
