/*
 * scheduler.h
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_SCHEDULE_H_
#define INCLUDE_SCHEDULE_H_

#include <mish.h>
#include <thread.h>

using namespace feta;

namespace mish {
namespace execute {
namespace schedule {

uinteger threadCount();
uinteger activeThreadCount();
uinteger backgroundThreadCount();

void run();
void kill(Thread* thread);
void spawn(Thread* thread);

}
}
}


#endif /* INCLUDE_SCHEDULE_H_ */
