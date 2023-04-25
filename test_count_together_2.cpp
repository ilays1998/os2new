#include <iostream>
#include "uthreads.h"

/**
 * Goals:
 *  - Check if we know to stop threads while running.
 *  - Check if we manage time correctly (execution time should be 1 second).
 *
 * Required functions:
 *  - uthread_init
 *  - uthread_spawn
 *  - uthread_terminate
 *
 * Expected result:
 *   This test should print the numbers up to 1000, and it should take 1 second.
 *   thread0 counted 1
 *   thread1 counted 2
 *   thread0 counted 3
 *   thread1 counted 4
 *   ...
 *   thread0 counted 999
 *   thread1 counted 1000
 *   test finished
 */

int count = 0;

void mainThread() {
    while (count < 1000) {
        if (count % 2 == 0) {
            count++;
            std::cout << "thread0 counted " << count << std::endl;
        }
    }
    std::cout << "test finished" << std::endl;
    uthread_terminate(0);
}

void thread1() {
    while (count < 1000) {
        if (count % 2 == 1) {
            count++;
            std::cout << "thread1 counted " << count << std::endl;
        }
    }
    uthread_terminate(1);
}

int main() {
    std::cout << "This test should print the numbers up to 1000, and it should take 1 second." << std::endl;
    uthread_init(1000);
    uthread_spawn(thread1);
    mainThread();
}

