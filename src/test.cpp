/*
 * test.cpp
 *
 *  Created on: Oct 21, 2016
 *      Author: chris13524
 */

#include <iostream>
#include <test.h>

#ifdef ALLOW_TESTING

#include <mish.h>

static void assert(bool b, String message) {
	if(!b) {
		crash(message);
	}
}

int flag1 = 0;
static Value* triggerFlag1Function(List<Value*>* arguments) {
	flag1++;
	return NULL;
}

int flag2 = 0;
static Value* triggerFlag2Function(List<Value*>* arguments) {
	flag2++;
	return NULL;
}

int trueFalseCounter = 0;
static Value* trueFalseFunction(List<Value*>* arguments) {
	Value* ret;

	if(trueFalseCounter == 1) {
		ret = new BooleanValue(false);
		trueFalseCounter = 0;
	} else {
		ret = new BooleanValue(true);
		trueFalseCounter++;
	}

	return ret;
}

int trueTrueFalseCounter = 0;
static Value* trueTrueFalseFunction(List<Value*>* arguments) {
	Value* ret;

	if(trueTrueFalseCounter == 2) {
		ret = new BooleanValue(false);
		trueTrueFalseCounter = 0;
	} else {
		ret = new BooleanValue(true);
		trueTrueFalseCounter++;
	}

	return ret;
}

static void resetFlags() {
	flag1 = 0;
	trueFalseCounter = 0;
	trueTrueFalseCounter = 0;
}

static void testMishCode(String sourceCode) {
	Code* code = mish_compile(sourceCode);
	mish_execute(code);
	delete code;
}

List<Function*> testSyscalls;
static void mish() {
	std::cout << "  - mish" << std::endl;

	// ---- register syscalls ----
	List<ValueType>* triggerFlag1ParameterTypes = new List<ValueType>();
	Function* triggerFlag1 = new Function("__triggerFlag1"_H, triggerFlag1ParameterTypes, VOID_VALUE, triggerFlag1Function);
	mish_syscalls.add(triggerFlag1);
	testSyscalls.add(triggerFlag1);

	List<ValueType>* triggerFlag2ParameterTypes = new List<ValueType>();
	Function* triggerFlag2 = new Function("__triggerFlag2"_H, triggerFlag2ParameterTypes, VOID_VALUE, triggerFlag2Function);
	mish_syscalls.add(triggerFlag2);
	testSyscalls.add(triggerFlag2);

	List<ValueType>* trueFalseParameterTypes = new List<ValueType>();
	Function* trueFalse = new Function("__trueFalse"_H, trueFalseParameterTypes, VOID_VALUE, trueFalseFunction);
	mish_syscalls.add(trueFalse);
	testSyscalls.add(trueFalse);

	List<ValueType>* trueTrueFalseParameterTypes = new List<ValueType>();
	Function* trueTrueFalse = new Function("__trueTrueFalse"_H, trueTrueFalseParameterTypes, VOID_VALUE, trueTrueFalseFunction);
	mish_syscalls.add(trueTrueFalse);
	testSyscalls.add(trueTrueFalse);

	// get allocated count
	uint64 origionalAllocatedCount = getAllocatedCount();

	// ---- tests ----

	resetFlags();

	testMishCode(TEST1);
	assert(flag1 == 0, "1");
	assert(flag2 == 1, "1-1");
	resetFlags();

	testMishCode("__triggerFlag1()");
	assert(flag1 == 1, "1");
	resetFlags();

	testMishCode("if(false){ __triggerFlag1() }");
	assert(flag1 == 0, "2");
	resetFlags();

	testMishCode("if(true){ __triggerFlag1() }");
	assert(flag1 == 1, "3");
	resetFlags();

	testMishCode("if(true){ while(false){ __triggerFlag1() } }");
	assert(flag1 == 0, "4");
	resetFlags();

	testMishCode("if(false){ while(true){ __triggerFlag1() } }");
	assert(flag1 == 0, "5");
	resetFlags();

	testMishCode("while(__trueFalse()){ __triggerFlag1() }");
	assert(flag1 == 1, "6");
	resetFlags();

	testMishCode("while(__trueTrueFalse()){ __triggerFlag1() }");
	assert(flag1 == 2, "7");
	resetFlags();

	// ---- done tests ----

	// get allocated count
	uint64 laterAllocatedCount = getAllocatedCount();

	// confirm no memory leaks
	//assert(origionalAllocatedCount == laterAllocatedCount, "memory leak");

	// unregister syscalls
	Iterator<Function*> iterator = testSyscalls.iterator();
	while (iterator.hasNext()) {
		Function* function = iterator.next();
		mish_syscalls.remove(function);
		delete function;
	}
	testSyscalls.clear();

	std::cout << "mish tests passed" << std::endl;
}

void test() {
	mish();
}

#endif
