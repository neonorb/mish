/*
 * test.cpp
 *
 *  Created on: Oct 21, 2016
 *      Author: chris13524
 */

#ifdef ALLOW_TEST

#include <mish.h>
#include <feta.h>
#include <mishtest.h>
#include <fetatest.h>

using namespace feta;

namespace mishtest {

	int flag1 = 0;
	static Value* triggerFlag1Function(List<Value*>* arguments) {
		UNUSED(arguments);

		flag1++;
		return VALUE_NOT_USED;
	}

	int flag2 = 0;
	static Value* triggerFlag2Function(List<Value*>* arguments) {
		UNUSED(arguments);

		flag2++;
		return VALUE_NOT_USED;
	}

	int trueFalseCounter = 0;
	static Value* trueFalseFunction(List<Value*>* arguments) {
		UNUSED(arguments);

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
		UNUSED(arguments);

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
		flag2 = 0;
		trueFalseCounter = 0;
		trueTrueFalseCounter = 0;
	}

	static void testMishCode(String sourceCode) {
		Code* code = mish::compile::compile(sourceCode);

		if(code!=NULL) {
			mish::execute::execute(code);
			delete code;
		}
	}

	List<Function*> testSyscalls;
	static void mish() {
		log("  - mish");

		// get allocated count
		uint64 origionalAllocatedCount = getAllocatedCount();

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

		// ---- tests ----

		resetFlags();

		testMishCode(TEST1);
		assert(flag1 == 1, "1");
		resetFlags();

		// if
		testMishCode(TEST2);
		assert(flag1 == 0, "2");
		assert(flag2 == 1, "2-1");
		resetFlags();

		testMishCode(TEST3);
		assert(flag1 == 1, "3");
		resetFlags();

		testMishCode(TEST4);
		assert(flag1 == 1, "4");
		assert(flag2 == 0, "4-1");
		resetFlags();

		testMishCode(TEST5);
		assert(flag1 == 0, "5");
		assert(flag2 == 1, "5-1");
		resetFlags();

		testMishCode(TEST6);
		assert(flag1 == 0, "6");
		assert(flag2 == 1, "6-1");
		resetFlags();

		testMishCode(TEST7);
		assert(flag1 == 1, "7");
		assert(flag2 == 0, "7-1");
		resetFlags();

		testMishCode(TEST8);
		assert(flag1 == 1, "8");
		assert(flag2 == 0, "8-1");
		resetFlags();

		testMishCode(TEST9);
		assert(flag1 == 0, "9");
		assert(flag2 == 1, "9-1");
		resetFlags();

		// while
		testMishCode(TEST10);
		assert(flag1 == 1, "10");
		resetFlags();

		testMishCode(TEST11);
		assert(flag1 == 2, "11");
		resetFlags();

		testMishCode(TEST12);
		assert(flag1 == 0, "12");
		resetFlags();

		testMishCode(TEST13);
		assert(flag1 == 1, "13");
		resetFlags();

		testMishCode(TEST14);
		assert(flag1 == 2, "14");
		resetFlags();

		// ---- done tests ----

		// unregister syscalls
		Iterator<Function*> iterator = testSyscalls.iterator();
		while (iterator.hasNext()) {
			Function* function = iterator.next();
			mish_syscalls.remove(function);
			delete function;
		}
		testSyscalls.clear();

		// get allocated count
		uint64 laterAllocatedCount = getAllocatedCount();

		// confirm no memory leaks
		if(origionalAllocatedCount != laterAllocatedCount) {
			//dumpAllocated();
		}
		assert(origionalAllocatedCount == laterAllocatedCount, "memory leak");
	}

	void test() {
		mish();
	}

}
#endif
