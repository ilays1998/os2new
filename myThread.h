//
// Created by rz121 on 4/21/23.
//

#ifndef OS2NEW_MYTHREAD_H
#define OS2NEW_MYTHREAD_H
#include <list>
#include <stack>

/**
 * Probably used more for function then save data maybe change to static
 */
class myThread {
  std::stack<long> myStack; //Probabley no need
  std::list<long> registers; //Probabley no need
  enum state { running, ready, blocked, sleeping };

public: myThread(int numID);

  myThread ();
 public:static int ID;
};


#endif //OS2NEW_MYTHREAD_H
