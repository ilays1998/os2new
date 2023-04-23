/*
 * User-Level Threads Library (uthreads)
 * Hebrew University OS course.
 * Author: OS, os@cs.huji.ac.il
 */
#include <queue>
#include "myThread.h"
#include <map>
#include <algorithm>
#include <iostream>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>


#ifndef _UTHREADS_H
#define _UTHREADS_H


#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define STACK_SIZE 4096 /* stack size per myThread (in bytes) */

typedef void (*thread_entry_point)(void);

/* internal interface */

std::list<myThread> readyThreads;
std::map<int, myThread> allThreads;

bool IDs[MAX_THREAD_NUM];

myThread runThread;

int quantum_time;

sigjmp_buf env[MAX_THREAD_NUM];

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


//taken from https://wandbox.org/permlink/PCKVrRgLcv0xOZvd
template <class It>
constexpr size_t find_index_of_next_false(It first, It last)
{
    size_t size = std::distance(first, last);
    return size <= 0 ? size : std::distance(first, std::find(first + 1, last, false));
}

address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
            : "=g" (ret)
            : "0" (addr));
    return ret;
}

void setup_thread(int tid, char *stack, thread_entry_point entry_point)
{
    // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
    // siglongjmp to jump into the thread.
    address_t sp = (address_t) reinterpret_cast<std::uintptr_t>(stack) + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t) reinterpret_cast<std::uintptr_t>(entry_point);
    sigsetjmp(env[tid], 1);
    (env[tid]->__jmpbuf)[JB_SP] = translate_address(sp);
    (env[tid]->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env[tid]->__saved_mask);
}



/* External interface */


/**
 * @brief initializes the myThread library.
 *
 * Once this function returns, the main myThread (tid == 0) will be set as RUNNING. There is no need to
 * provide an entry_point or to create a stack for the main myThread - it will be using the "regular" stack and PC.
 * You may assume that this function is called before any other myThread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
  if (quantum_usecs < 1)
    return -1;
  quantum_time = quantum_usecs;
  myThread *mainThread = new myThread(0, STACK_SIZE);
  allThreads[0] = *mainThread;
  return 0;
}

/**
 * @brief Creates a new myThread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The myThread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each myThread should be allocated with a stack of size STACK_SIZE bytes.
 * It is an error to call this function with a null entry_point.
 *
 * @return On success, return the ID of the created myThread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point){
    if (!entry_point)
        return -1;
    const int freeID = find_index_of_next_false(std::cbegin(IDs), std::cend(IDs));
    if (freeID >= MAX_THREAD_NUM)
        return -1;
    myThread *newThread = new myThread(freeID, STACK_SIZE);
    allThreads[freeID] = *newThread;
    setup_thread(freeID, newThread->getStack(), entry_point);
}


/**
 * @brief Terminates the myThread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this myThread should be released. If no myThread with ID tid exists it
 * is considered an error. Terminating the main myThread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the myThread was successfully terminated and -1 otherwise. If a myThread terminates
 * itself or the main myThread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
  if (tid < 0 || tid >= MAX_THREAD_NUM || !IDs[tid]) {
    return -1;
  }
  if (tid == 0)
    exit(0);
  if (tid == runThread.get_id()) {
      IDs[tid] = false;
      delete &allThreads[tid];

      //ready not empty
      runThread = readyThreads.front();
      readyThreads.pop_front();
      runThread.setCurState(running);
      siglongjmp(env[runThread.get_id()], 1);
  }
  if (allThreads[tid].getCurState(ready)){
      readyThreads.remove(allThreads[tid]);
  }
  IDs[tid] = false;
  delete &allThreads[tid];
  allThreads[tid];
  return 0;
}


/**
 * @brief Blocks the myThread with ID tid. The myThread may be resumed later using uthread_resume.
 *
 * If no myThread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main myThread (tid == 0). If a myThread blocks itself, a scheduling decision should be made. Blocking a myThread in
 * BLOCKED state has no effect and is not considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid){
    if (tid < 0 || tid >= MAX_THREAD_NUM || !IDs[tid]){
        //error msg
        return -1;
    }
    if (tid == 0){
        //error msg
        return -1;
    }
    if (tid == runThread.get_id()){
        //block the running thread and save is state
        sigsetjmp(env[tid], 1);
        allThreads[tid].setCurState(blocked);
        // change the previous and next threads stats
        runThread = readyThreads.front();
        readyThreads.pop_front();
        runThread.setCurState(running);
        // run the first ready thread
        siglongjmp(env[runThread.get_id()], 1);
        return 0;
    }
    if (allThreads[tid].getCurState(blocked))
        return 0;
    readyThreads.remove(allThreads[tid]);
    allThreads[tid].setCurState(blocked);
    return 0;
}


/**
 * @brief Resumes a blocked myThread with ID tid and moves it to the READY state.
 *
 * Resuming a myThread in a RUNNING or READY state has no effect and is not considered as an error. If no myThread with
 * ID tid exists it is considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid){
    if (tid < 0 || tid >= MAX_THREAD_NUM || !IDs[tid]){
        //error msg
        return -1;
    }
    if (allThreads[tid].getCurState(running) || allThreads[tid].getCurState(ready)){
        //error msg
        return -1;
    }
    readyThreads.push_back(allThreads[tid]);
    allThreads[tid].setCurState(ready);
}


/**
 * @brief Blocks the RUNNING myThread for num_quantums quantums.
 *
 * Immediately after the RUNNING myThread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the myThread should go back to the end of the READY queue.
 * If the myThread which was just RUNNING should also be added to the READY queue, or if multiple threads wake up
 * at the same time, the order in which they're added to the end of the READY queue doesn't matter.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the myThread which has made the call to uthread_sleep isn’t counted.
 * It is considered an error if the main myThread (tid == 0) calls this function.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums);


/**
 * @brief Returns the myThread ID of the calling myThread.
 *
 * @return The ID of the calling myThread.
*/
int uthread_get_tid(){
    return runThread.get_id();
}


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums();


/**
 * @brief Returns the number of quantums the myThread with ID tid was in RUNNING state.
 *
 * On the first time a myThread runs, the function should return 1. Every additional quantum that the myThread starts should
 * increase this value by 1 (so if the myThread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no myThread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the myThread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid);


#endif
