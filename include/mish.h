/*
 * mish.h
 *
 *  Created on: May 3, 2016
 *      Author: chris
 */

#include <bytecode.h>
#include <state.h>
#include <compile.h>
#include <execute.h>
#include <schedule.h>
#include <string.h>

#ifndef INCLUDE_MISH_H_
#define INCLUDE_MISH_H_

#include <list.h>

namespace mish {

extern feta::List<Function*> mish_syscalls;
extern feta::List<Class*> mish_classes;

}

#endif /* INCLUDE_MISH_H_ */
