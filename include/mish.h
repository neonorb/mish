/*
 * mish.h
 *
 *  Created on: May 3, 2016
 *      Author: chris
 */

#ifndef INCLUDE_MISH_H_
#define INCLUDE_MISH_H_

#include <code.h>
#include <string.h>
#include <syscall.h>

Code* mish_compile(String code);
Code* mish_compile(String start, void* end);

void mish_addSyscall(Function* syscall);
List<Function*> mish_getSyscalls();

#endif /* INCLUDE_MISH_H_ */
