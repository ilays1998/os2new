//
// Created by rz121 on 4/21/23.
//

#include "myThread.h"
myThread::myThread (int numID) {
  ID = numID;
}
myThread::myThread ()
{
}
int myThread::get_id () const
{
  return ID;
}

myThread::state myThread::getCurState() const {
    return curState;
}

void myThread::setCurState(myThread::state curState) {
    myThread::curState = curState;
}

