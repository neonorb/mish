/*
 * mish.h
 *
 *  Created on: May 3, 2016
 *      Author: chris
 */

#ifndef INCLUDE_MISH_H_
#define INCLUDE_MISH_H_

#include <bytecode.h>
#include <state.h>
#include <code.h>
#include <compile.h>
#include <execute.h>
#include <schedule.h>
#include <string.h>

extern List<Function*> mish_syscalls;
extern List<Class*> mish_classes;

#endif /* INCLUDE_MISH_H_ */
