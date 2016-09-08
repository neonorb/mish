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

extern List<Function*> mish_syscalls;

Code* mish_compile(String code);
Code* mish_compile(String start, size_t size);

void mish_execute(Code* code);

#endif /* INCLUDE_MISH_H_ */
