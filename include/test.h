/*
 * test.h
 *
 *  Created on: Oct 21, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_TEST_H_
#define INCLUDE_TEST_H_

#ifdef ALLOW_TEST
void test();

// ---- tests ----

#define TEST1        \
"if (false) {        \
	__triggerFlag1() \
};                    \
__triggerFlag2()"

// ---- end tests ----

#endif

#endif /* INCLUDE_TEST_H_ */
