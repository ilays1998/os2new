//
// Created by rz121 on 4/21/23.
//

#include "myThread.h"
myThread::myThread(int numID, int stackSize) {
  ID = numID;
  stack = new char [stackSize];
}
myThread::myThread ()
{
}
int myThread::get_id () const
{
  return ID;
}

state myThread::getCurState(state state) const {
    return curState;
}

void myThread::setCurState(state curState) {
    myThread::curState = curState;
}

char *myThread::getStack() {
    return stack;
}

