/*
 * scope.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_SCOPE_H_
#define INCLUDE_SCOPE_H_

class Scope;

#include <list.h>
#include <function.h>

using namespace feta;

class Scope {
public:
	Scope();
	~Scope();

	Scope* parent;

	List<Function*> functions;
	// TODO classes
};

#endif /* INCLUDE_SCOPE_H_ */
