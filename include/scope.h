/*
 * scope.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_SCOPE_H_
#define INCLUDE_SCOPE_H_

class Scope;

#include <mish.h>

using namespace feta;

class Scope {
public:
	Scope();
	~Scope();

	Scope* parent;

	List<Function*> functions;
	List<Class*> classes;
	List<VariableDefinition*> variables;
};

#endif /* INCLUDE_SCOPE_H_ */
