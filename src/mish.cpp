/*
 * mish.cpp
 *
 *  Created on: May 3, 2016
 *      Author: chris
 */

#include <mish.h>
#include <danbo.h>

namespace mish {

List<Function*> mish_syscalls;
List<Class*> mish_classes;

#include "grammar/mish-grammar"

}
