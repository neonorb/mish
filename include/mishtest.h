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
"if (false) {\n\
	__triggerFlag1()\n\
}\n\
__triggerFlag2()"

#define TEST2 \
"if (true) {\n\
	__triggerFlag1()\n\
}"

#define TEST3 \
"if (true) {\n\
	__triggerFlag1()\n\
} else {\n\
	__triggerFlag2()\n\
}"

#define TEST4 \
"if (false) {\n\
	__triggerFlag1()\n\
} else {\n\
	__triggerFlag2()\n\
}"

#define TEST5 \
"if (false) {\n\
	__triggerFlag1()\n\
} elseif (true) {\n\
	__triggerFlag2()\n\
} else {\n\
	__triggerFlag1()\n\
}"

#define TEST6 \
"if (true) {\n\
	__triggerFlag1()\n\
} elseif (true) {\n\
	__triggerFlag2()\n\
} else {\n\
	__triggerFlag2()\n\
}"

#define TEST7 \
"if (true) {\n\
	__triggerFlag1()\n\
} elseif (false) {\n\
	__triggerFlag2()\n\
} else {\n\
	__triggerFlag2()\n\
}"

#define TEST8 \
"if (false) {\n\
	__triggerFlag1()\n\
} elseif (false) {\n\
	__triggerFlag1()\n\
} else {\n\
	__triggerFlag2()\n\
}"

// ---- end tests ----

}
#endif

#endif /* INCLUDE_MISHTEST_H_ */
