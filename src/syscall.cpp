/*
 * syscall.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#include <syscall.h>

Syscall::Syscall(String name, SyscallHandler handler) {
	this->name = name;
	this->handler = handler;
}

Syscall* Syscall::destroy() {
	return this;
}

Value* Syscall::call(List<Value*>* arguments) {
	return handler(arguments);
}
