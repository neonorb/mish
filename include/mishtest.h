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

// ---- end tests ----

}
#endif

#endif /* INCLUDE_MISHTEST_H_ */
