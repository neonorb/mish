/*
 * scheduler.cpp
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#include <scheduler.h>

List<Thread*> mish_threads;

void mish_runScheduler() {
	List<Thread*> removedThreads;

	Iterator<Thread*> threadIterator = mish_threads.iterator();
	while (threadIterator.hasNext()) {
		Thread* thread = threadIterator.next();
		bool keepGoing = true;
		for (uint64 i = 0; i < 10 && keepGoing; i++) {
			ExecuteStatus status = mish_execute(thread->state);
			if (status == DONE) {
				removedThreads.add(thread);
				keepGoing = false;
			}
		}
	}

	Iterator<Thread*> removedThreadsIterator = removedThreads.iterator();
	while (removedThreadsIterator.hasNext()) {
		mish_killThread(removedThreadsIterator.next());
	}
}

void mish_killThread(Thread* thread) {
	if (thread->onThreadExit != NULL) {
		thread->onThreadExit(thread);
	}

	mish_threads.remove(thread);
	delete thread;
}

void mish_spawnThread(Thread* thread) {
	mish_threads.add(thread);
}

