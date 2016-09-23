/*
 * scheduler.h
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include <mish.h>
#include <thread.h>

void mish_runScheduler();
void mish_killThread(Thread* thread);
void mish_spawnThread(Thread* thread);

#endif /* INCLUDE_SCHEDULER_H_ */
