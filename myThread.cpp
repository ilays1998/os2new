//
// Created by rz121 on 4/21/23.
//

#include "myThread.h"
#include "iostream"
myThread::myThread(int numID, int stackSize) {
  ID = numID;
  this->stack = (char*) malloc(stackSize);
  quantum_life = 0;
  timeToSleep = 0;
  curState = ready;
}
myThread::myThread ()
{
}
int myThread::get_id () const
{
  return ID;
}

int myThread::getTimeToSleep() const {
    return timeToSleep;
}
void myThread::setTimeToSleep(int time) {
    timeToSleep = time;
}
state myThread::getCurState() const {
    return curState;
}

void myThread::setCurState(state curState) {
    myThread::curState = curState;
}

char *myThread::getStack() {
    return stack;
}

void myThread::updateTimeToSleep() {
    myThread::timeToSleep--;
}
int myThread::getQuantumLife() const {
    return quantum_life;
}

void myThread::updateQuantumLife() {
    quantum_life++;
}
bool myThread::operator == (const myThread& t) const {return ID == t.ID;}

void myThread::deleteStack() {
    delete stack;
    stack = nullptr;
}

