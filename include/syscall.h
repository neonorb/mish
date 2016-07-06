/*
 * syscall.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include <string.h>
#include <list.h>
#include <value.h>
#include <function.h>

typedef Value* (*SyscallHandler)(List<Value*>* arguments);

class Syscall: Function {
private:
	SyscallHandler handler;
public:
	String name;

	Syscall(String name, SyscallHandler handler);
	Syscall* destroy();

	Value* call(List<Value*>* arguments);
};

#endif /* INCLUDE_SYSCALL_H_ */
