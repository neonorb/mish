/*
 * expression.cpp
 *
 *  Created on: Jun 7, 2016
 *      Author: chris
 */

#include <expression.h>
#include <functioncallreturn.h>
#include <value.h>

Expression::Expression(ValueType valueType) {
	this->valueType = valueType;
}

Expression::~Expression() {
}
