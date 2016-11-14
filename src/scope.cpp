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
	functions = new List<Function*>();
	classes = new List<Class*>();
	variables = new List<VariableDefinition*>();
}

Scope::~Scope() {
	Iterator<Function*> functionIterator = functions->iterator();
	while (functionIterator.hasNext()) {
		delete functionIterator.next();
	}
	delete functions;

	Iterator<Class*> classIterator = classes->iterator();
	while (classIterator.hasNext()) {
		delete classIterator.next();
	}
	delete classes;

	Iterator<VariableDefinition*> variableIterator = variables->iterator();
	while (variableIterator.hasNext()) {
		delete variableIterator.next();
	}
	delete variables;
}
