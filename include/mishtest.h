/*
 * test.h
 *
 *  Created on: Oct 21, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_MISHTEST_H_
#define INCLUDE_MISHTEST_H_

#ifdef ALLOW_TEST
namespace mishtest {

	void test();

// ---- tests ----

#define TEST1 \
"__triggerFlag1()"

// -- if --
#define TEST2 \
"if (false) {\n\
	__triggerFlag1()\n\
}\n\
__triggerFlag2()"

#define TEST3 \
"if (true) {\n\
	__triggerFlag1()\n\
}"

#define TEST4 \
"if (true) {\n\
	__triggerFlag1()\n\
} else {\n\
	__triggerFlag2()\n\
}"

#define TEST5 \
"if (false) {\n\
	__triggerFlag1()\n\
} else {\n\
	__triggerFlag2()\n\
}"

#define TEST6 \
"if (false) {\n\
	__triggerFlag1()\n\
} elseif (true) {\n\
	__triggerFlag2()\n\
} else {\n\
	__triggerFlag1()\n\
}"

#define TEST7 \
"if (true) {\n\
	__triggerFlag1()\n\
} elseif (true) {\n\
	__triggerFlag2()\n\
} else {\n\
	__triggerFlag2()\n\
}"

#define TEST8 \
"if (true) {\n\
	__triggerFlag1()\n\
} elseif (false) {\n\
	__triggerFlag2()\n\
} else {\n\
	__triggerFlag2()\n\
}"

#define TEST9 \
"if (false) {\n\
	__triggerFlag1()\n\
} elseif (false) {\n\
	__triggerFlag1()\n\
} else {\n\
	__triggerFlag2()\n\
}"

// -- while --
#define TEST10 \
"while (__trueFalse()) {\n\
	__triggerFlag1()\n\
}"

#define TEST11 \
"while (__trueTrueFalse()) {\n\
	__triggerFlag1()\n\
}"

#define TEST12 \
"while (false) {\n\
	__triggerFlag1()\n\
}"

#define TEST13 \
"dowhile (false) {\n\
	__triggerFlag1()\n\
}"

#define TEST14 \
"dowhile (__trueFalse()) {\n\
	__triggerFlag1()\n\
}"

#define TEST15 \
"__Boolean x"

#define TEST16 \
"__Boolean x = true"

#define TEST17 \
"__Boolean x = false\n\
if (x) {\n\
	__triggerFlag1()\n\
}"

#define TEST18 \
"__Boolean x = true\n\
if (x) {\n\
	__triggerFlag1()\n\
}"

#define TEST19 \
"__Boolean x = false\n\
x = true\n\
if (x) {\n\
	__triggerFlag1()\n\
}"

#define TEST20 \
"__String x = '1'\n\
x = '2'"

#define TEST21 \
"__String x = '1'\n\
__String y = x\n\
x = '2'\n\
y = '2'"

// ---- end tests ----

}
#endif

#endif /* INCLUDE_MISHTEST_H_ */
