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
enum state { running, ready, blocked , sleep_and_blocked, sleeping};

class myThread {
  //std::list<long> registers; //Probabley no need
  state curState;
  char *stack;
  int ID;
  int quantum_life;
  int timeToSleep;

public:
    myThread(int numID, int stackSize);
    myThread ();
    int getTimeToSleep() const;



    bool operator == (const myThread& t) const;

    void setCurState(state curState);



    char *getStack();

    int get_id () const;

    void setTimeToSleep(int time);

    void updateTimeToSleep();

    int getQuantumLife() const;

    void updateQuantumLife();

    state getCurState() const;

    void deleteStack();
};


#endif //OS2NEW_MYTHREAD_H
