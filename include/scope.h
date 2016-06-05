/*
 * scope.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_SCOPE_H_
#define INCLUDE_SCOPE_H_

#include <list.h>
#include <function.h>

class Function;
class Scope {
public:
	Scope* destroy();

	Scope* parent = NULL;

	List<Function*> functions;
	// TODO classes
};

#endif /* INCLUDE_SCOPE_H_ */
