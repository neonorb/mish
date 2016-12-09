/*
 * scope.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_SCOPE_H_
#define INCLUDE_SCOPE_H_

namespace mish {

class Scope;

}

#include <mish.h>

namespace mish {

class Scope {
public:
	Scope();
	~Scope();

	Scope* parent;

	feta::List<Function*>* functions;
	feta::List<Class*>* classes;
	feta::List<VariableDefinition*>* variables;
};

}

#endif /* INCLUDE_SCOPE_H_ */
