/*
 * mish.cpp
 *
 *  Created on: May 3, 2016
 *      Author: chris
 */

#include <mish.h>
#include <log.h>
#include <string.h>

Code* mish_compile(String code) {
	Code* compiledCode = new Code();

	for (uint64 i = 0; i < strlen(code); i++) {
		char c = code[i];
		// TODO
	}

	return compiledCode;
}
