/*
 * array.h
 *
 *  Created on: Jun 5, 2016
 *      Author: chris
 */

#ifndef INCLUDE_ARRAY_H_
#define INCLUDE_ARRAY_H_

#include <feta.h>

using namespace feta;

template<typename T> bool arrayContains(T* array, size size, T element) {
	for (uint64 i = 0; i < size; i++) {
		if (array[i] == element) {
			return true;
		}
	}

	return false;
}

#endif /* INCLUDE_ARRAY_H_ */
