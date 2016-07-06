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
void mish_addSyscall(Syscall* syscall);
List<Syscall*> mish_getSyscalls();

#endif /* INCLUDE_MISH_H_ */
