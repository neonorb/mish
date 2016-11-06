/*
 * scheduler.cpp
 *
 *  Created on: Sep 23, 2016
 *      Author: chris13524
 */

#include <scheduler.h>

using namespace mish::execute;

List<Thread*> mish_threads;

// thread counts
uint64 mish_threadCount() {
	uint64 threadCount = 0;

	Iterator<Thread*> threadIterator = mish_threads.iterator();
	while (threadIterator.hasNext()) {
		threadCount++;
	}

	return threadCount;
}
uint64 mish_activeThreadCount() {
	uint64 activeThreadCount = 0;

	Iterator<Thread*> threadIterator = mish_threads.iterator();
	while (threadIterator.hasNext()) {
		if (threadIterator.next()->priority == ACTIVE) {
			activeThreadCount++;
		}
	}

	return activeThreadCount;
}
uint64 mish_backgroundThreadCount() {
	uint64 backgroundThreadCount = 0;

	Iterator<Thread*> threadIterator = mish_threads.iterator();
	while (threadIterator.hasNext()) {
		if (threadIterator.next()->priority == BACKGROUND) {
			backgroundThreadCount++;
		}
	}

	return backgroundThreadCount;
}

// scheduler

void mish_runScheduler() {
	List<Thread*> removedThreads;

	Iterator<Thread*> threadIterator = mish_threads.iterator();
	while (threadIterator.hasNext()) {
		Thread* thread = threadIterator.next();
		bool keepGoing = true;
		for (uint64 i = 0; i < 10 && keepGoing; i++) {
			Status status = mish_execute(thread->state);
			if (status == Status::DONE) {
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

