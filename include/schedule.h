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

namespace mish {
namespace execute {
namespace schedule {

uint64 threadCount();
uint64 activeThreadCount();
uint64 backgroundThreadCount();

void run();
void kill(Thread* thread);
void spawn(Thread* thread);

}
}
}


#endif /* INCLUDE_SCHEDULE_H_ */
