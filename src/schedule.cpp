/*
 * scheduler.cpp
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#include <schedule.h>

namespace mish {
namespace execute {
namespace schedule {

List<Thread*> threads;

// thread counts
uint64 threadCount() {
	return threads.size();
}
uint64 activeThreadCount() {
	uint64 activeThreadCount = 0;

	Iterator<Thread*> threadIterator = threads.iterator();
	while (threadIterator.hasNext()) {
		if (threadIterator.next()->priority == ACTIVE) {
			activeThreadCount++;
		}
	}

	return activeThreadCount;
}
uint64 backgroundThreadCount() {
	uint64 backgroundThreadCount = 0;

	Iterator<Thread*> threadIterator = threads.iterator();
	while (threadIterator.hasNext()) {
		if (threadIterator.next()->priority == BACKGROUND) {
			backgroundThreadCount++;
		}
	}

	return backgroundThreadCount;
}

// scheduler

void run() {
	List<Thread*> removedThreads;

	Iterator<Thread*> threadIterator = threads.iterator();
	while (threadIterator.hasNext()) {
		Thread* thread = threadIterator.next();
		bool keepGoing = true;
		for (uint64 i = 0; i < 10 && keepGoing; i++) {
			Status status = execute(thread->state);
			if (status == Status::DONE) {
				removedThreads.add(thread);
				keepGoing = false;
			}
		}
	}

	Iterator<Thread*> removedThreadsIterator = removedThreads.iterator();
	while (removedThreadsIterator.hasNext()) {
		kill(removedThreadsIterator.next());
	}
}

void kill(Thread* thread) {
	if (thread->onThreadExit != NULL) {
		thread->onThreadExit(thread);
	}

	threads.remove(thread);
	delete thread;
}

void spawn(Thread* thread) {
	threads.add(thread);
}

}
}
}

