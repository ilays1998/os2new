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
#include <iterator>
#include <set>


#ifndef _UTHREADS_H
#define _UTHREADS_H


#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define STACK_SIZE 4096 /* stack size per myThread (in bytes) */

typedef void (*thread_entry_point)(void);

/* internal interface */

std::list<int> readyThreads;
std::set<int> sleepingThreads;
std::map<int, myThread> allThreads;

bool IDs[MAX_THREAD_NUM];

int runThread;

int quantum_time;
int quantum_time_counter;

sigjmp_buf env[MAX_THREAD_NUM];
sigset_t mask;

struct sigaction sa = {0};
struct itimerval timer;


int RETURN_ERROR = -1;
#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

#define DEF "thread library error: quantum_usecs must be positive integer"

#define MASKING sigprocmask(SIG_BLOCK, &mask, nullptr)
#define UNMASKING sigprocmask(SIG_UNBLOCK, &mask, nullptr)


#define BLOCKED 0

#define SLEEP 1

#define READY 2

#define TERMINATED 3

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
            : "=g" (ret)
            : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}


#endif


//taken from https://wandbox.org/permlink/PCKVrRgLcv0xOZvd
template <class It>
 size_t find_index_of_next_false(It first, It last)
{
    size_t size = std::distance(first, last);
    return size <= 0 ? size : std::distance(first, std::find(first + 1, last, false));
}


void setup_thread(int tid, char *stack, thread_entry_point entry_point)
{
    // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
    // siglongjmp to jump into the thread.
    address_t sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t) entry_point;
    sigsetjmp(env[tid], 1);
    (env[tid]->__jmpbuf)[JB_SP] = translate_address(sp);
    (env[tid]->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env[tid]->__saved_mask);
}



/* External interface */

void timer_handler(int);
void timer_init();

void awake();

void ready_change();

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
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
  if (quantum_usecs <= 0)
  {
      std::cerr << DEF << std::endl;
      return -1;
  }
    if (RETURN_ERROR == sigemptyset(&mask))
    {
        std::cerr << "EMPTY MASK FAIL" << std::endl;
        return  RETURN_ERROR;
    }
    if (RETURN_ERROR == sigaddset(&mask, SIGVTALRM))
    {
        std::cerr << "ADD SIGNAL MASK FAIL" << std::endl;
        return  RETURN_ERROR;
    }

  allThreads[0] = myThread(0, STACK_SIZE);
  allThreads[0].setCurState(running);
  allThreads[0].updateQuantumLife();
  IDs[0] = true;
  setup_thread(0, allThreads[0].getStack(), nullptr);
  quantum_time = quantum_usecs;
  // Install timer_handler as the signal handler for SIGVTALRM.
  timer_init();

  return 0;
}



void timer_init(){
    quantum_time_counter = 1;
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        printf("sigaction error.");
    }

    // Configure the timer to expire after 1 sec... */
    timer.it_value.tv_sec = 0;        // first time interval, seconds part
    timer.it_value.tv_usec = quantum_time;        // first time interval, microseconds part

    // configure the timer to expire every 3 sec after that.
    timer.it_interval.tv_sec = 0;    // following time intervals, seconds part
    timer.it_interval.tv_usec = quantum_time;    // following time intervals, microseconds part

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &timer, nullptr))
    {
        printf("setitimer error.");
    }
}

void jump_to_thread(int tid)
{
    if (RETURN_ERROR == MASKING)
    {
        std::cerr << "FAIL MASKING" << std::endl;
        return;
    }
    runThread = tid;
    if (RETURN_ERROR == UNMASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return;
    }
    siglongjmp(env[tid], 1);


}

void timer_handler(int sig) {
    if (RETURN_ERROR == MASKING)
    {
        std::cerr << "FAIL MASKING" << std::endl;
        return;
    }
    quantum_time_counter++;

    awake();

    int ret_val = sigsetjmp(env[runThread], 1);
    //printf("yield: ret_val=%d\n", ret_val);
    bool did_just_save_bookmark = ret_val == 0;
//    bool did_jump_from_another_thread = ret_val != 0;
    if (did_just_save_bookmark)
    {
        switch (sig) {
            case BLOCKED:
                if (allThreads[runThread].getTimeToSleep() > 0)
                    allThreads[runThread].setCurState(sleep_and_blocked);
                else
                    allThreads[runThread].setCurState(blocked);
                break;
            case SLEEP:
                if (allThreads[runThread].getCurState() == blocked)
                    allThreads[runThread].setCurState(sleep_and_blocked);
                else
                    allThreads[runThread].setCurState(sleeping);
                break;
            case READY:
                allThreads[runThread].setCurState(ready);
                break;
            case TERMINATED:
                IDs[runThread] = false;
                allThreads[runThread].deleteStack();
                allThreads.erase(runThread);
                break;
            default:
                allThreads[runThread].setCurState(ready);
                readyThreads.push_back(runThread);
                break;
        }
        if(!readyThreads.empty()){
            runThread = readyThreads.front();
            readyThreads.pop_front();
            allThreads[runThread].setCurState(running);
            allThreads[runThread].updateQuantumLife();
        }
        else{
            runThread = 0;
            allThreads[runThread].setCurState(running);
            allThreads[runThread].updateQuantumLife();
        }
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return;
        }
        jump_to_thread(runThread);
    }

}

void awake() {
    if (RETURN_ERROR == MASKING)
    {
        std::cerr << "FAIL MASKING" << std::endl;
        return;
    }
    std::set<int> need_to_wake;
    for (auto it = sleepingThreads.begin(); it != sleepingThreads.end(); ++it){
        if (allThreads[*it].getCurState() == running){ // didn't pass a quantum yet
            continue;
        }
        allThreads[*it].updateTimeToSleep();
        if (allThreads[*it].getTimeToSleep() <= 0){
            need_to_wake.insert(*it);
/*            if (allThreads[*it].getCurState() != sleep_and_blocked)
                readyThreads.push_back(*it);
            else
                allThreads[*it].setCurState(blocked);
            allThreads[*it].setTimeToSleep(0);
            it = sleepingThreads.erase(it);*/
        }
    }
    for (auto it = need_to_wake.begin(); it != need_to_wake.end(); ++it){
        if (allThreads[*it].getCurState() != sleep_and_blocked)
        {
            readyThreads.push_back(*it);
            allThreads[*it].setCurState(ready);
        }
        else
            allThreads[*it].setCurState(blocked);
        allThreads[*it].setTimeToSleep(0);
        sleepingThreads.erase(*it);
    }
    if (RETURN_ERROR == UNMASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return;
    }


/*    for (auto it = sleepingThreads.begin(); it != sleepingThreads.end();it++) {
        printf("%d",*it);
        std::cout << " ";
    }
    std::cout << "\n";*/
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
    if (RETURN_ERROR == MASKING)
    {
        std::cerr << "FAIL MASKING" << std::endl;
        return -1;
    }
    if (!entry_point)
    {
        std::cerr << "thread library error: invalid given function" << std::endl;
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return -1;
    }
    const int freeID = find_index_of_next_false(std::begin(IDs), std::end(IDs));
    if (freeID >= MAX_THREAD_NUM)
    {
        std::cerr << "thread library error: too many threads" << std::endl;
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return -1;
    }
    allThreads[freeID] =  myThread(freeID, STACK_SIZE);
    setup_thread(freeID, allThreads[freeID].getStack(), entry_point);
    readyThreads.push_back(freeID);
    IDs[freeID] = true;
    if (RETURN_ERROR == UNMASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return -1;
    }
    return freeID;
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
    if (RETURN_ERROR == MASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return -1;
    }
  if (tid < 0 || tid >= MAX_THREAD_NUM || !IDs[tid]) {
      std::cerr << "thread library error: no such thread" << std::endl;
      if (RETURN_ERROR == UNMASKING)
      {
          std::cerr << "FAIL UNMASKING" << std::endl;
          return -1;
      }
      return -1;
  }
  if (tid == 0) //TODO erase the memory
    exit(0);
  if (allThreads[tid].getCurState() == running) {
      timer_handler(TERMINATED);
      if (RETURN_ERROR == UNMASKING)
      {
          std::cerr << "FAIL UNMASKING" << std::endl;
          return -1;
      }
      return 0;
  }
  else if (allThreads[tid].getCurState() == ready){
      readyThreads.remove(tid);
  }
  else if (allThreads[tid].getCurState() == sleep_and_blocked || allThreads[tid].getCurState() == sleeping){
      sleepingThreads.erase(tid);
  }
  IDs[tid] = false;
  allThreads[tid].deleteStack();
  allThreads.erase(tid);
    if (RETURN_ERROR == UNMASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return -1;
    }
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
    if (RETURN_ERROR == MASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return -1;
    }
    if (tid < 0 || tid >= MAX_THREAD_NUM || !IDs[tid]){
        std::cerr << "thread library error: no such thread" << std::endl;
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return -1;
    }
    if (tid == 0){
        std::cerr << "thread library error: can't block main thread" << std::endl;
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return -1;
    }
    if (tid == runThread){
        //block the running thread and save is state
        sigsetjmp(env[tid], 1);
        allThreads[tid].setCurState(blocked);
        // change the previous and next threads stats
/*        runThread = readyThreads.front();
        readyThreads.pop_front();
        runThread.setCurState(running);
        // run the first ready thread
        siglongjmp(env[runThread.get_id()], 1);*/
        timer_handler(BLOCKED); // TODO
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return 0;
    }
    if (allThreads[tid].getCurState() == blocked){
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return 0;
    }
    readyThreads.remove(tid);
    if (allThreads[tid].getCurState() == sleeping)
        allThreads[tid].setCurState(sleep_and_blocked);
    else
        allThreads[tid].setCurState(blocked);
    if (RETURN_ERROR == UNMASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return -1;
    }
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
    if (RETURN_ERROR == MASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return -1;
    }
    if (tid < 0 || tid >= MAX_THREAD_NUM || !IDs[tid]){
        std::cerr << "thread library error: no such thread" << std::endl;
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return -1;
    }
    if (allThreads[tid].getCurState() == running || allThreads[tid].getCurState() == ready ||
        allThreads[tid].getCurState() == sleeping){
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return 0;
    }
    if (allThreads[tid].getCurState() == sleep_and_blocked){
        allThreads[tid].setCurState(sleeping);
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return 0;
    }
    readyThreads.push_back(tid);
    allThreads[tid].setCurState(ready);
    if (RETURN_ERROR == UNMASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return -1;
    }
    return 0;
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
int uthread_sleep(int num_quantums){
    if (RETURN_ERROR == MASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return -1;
    }
    if (runThread == 0){
        std::cerr << "thread library error: main thread can't sleep" << std::endl;
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return -1;
    }
    if (num_quantums < 0){
        std::cerr << "thread library error: can't sleep negative time" << std::endl;
        if (RETURN_ERROR == UNMASKING)
        {
            std::cerr << "FAIL UNMASKING" << std::endl;
            return -1;
        }
        return -1;
    }
    allThreads[runThread].setTimeToSleep(num_quantums);
    sleepingThreads.insert(runThread);
    timer_handler(SLEEP); // TODO
    if (RETURN_ERROR == UNMASKING)
    {
        std::cerr << "FAIL UNMASKING" << std::endl;
        return -1;
    }
    return 0;
}


/**
 * @brief Returns the myThread ID of the calling myThread.
 *
 * @return The ID of the calling myThread.
*/
int uthread_get_tid(){
    return runThread;
}


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums(){
    return quantum_time_counter;
}


/**
 * @brief Returns the number of quantums the myThread with ID tid was in RUNNING state.
 *
 * On the first time a myThread runs, the function should return 1. Every additional quantum that the myThread starts should
 * increase this value by 1 (so if the myThread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no myThread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the myThread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid){
    if (tid < 0 || tid >= MAX_THREAD_NUM || !IDs[tid]){
        std::cerr << "thread library error: no such thread" << std::endl;
        return -1;
    }
    return allThreads[tid].getQuantumLife();
}


#endif
