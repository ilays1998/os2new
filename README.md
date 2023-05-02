ilaysoffer, rz121
Ilay Soffer (207446709), Ramon Zerem(209492156)

EX: 2

FILES:

  * uthreads.cpp - static libary for user level threads
  * uthreads.h - header for the static libary for user level threads
  * myThread.cpp - class for the myThread represnting single thread in the user threads libary
  * myThread.h - header for myThread class 


ANSWERS:

Q1:

  a. The function sigsetjmp save the current state of the thread - it's pc stack and registers. The function siglngjmp
  jump back to the saved state of the thread.

  b. sigsetjmp saves the signal mask at the point of the jump. This means that if a signal is delivered after sigsetjmp
  is called but before siglongjmp is called, the signal will be blocked and will not be delivered until after the jump
  is made. and vice versa siglngjmp restores the signal mask that was saved by sigsetjmp. This means that if any signals
  were blocked when sigsetjmp() was called, they will continue to be blocked after siglongjmp is called.

Q2:

  In graphical user interfaces (GUIs), where user-level threads can be used to manage the various components of the GUI,
  such as the menus, buttons, and dialog boxes. We don't need kernel level premission.Each component can be managed
  independently in a separate user-level thread,allowing the GUI to remain responsive even when one component is busy
  or unresponsive.

Q3:

  The main advantages for the creating of a new process are:
  * Resource allocation: Each tab has its own copies of system resources, such as memory, file descriptors, and network
    connections, which makes it easier to manage and allocate resources.
  * Robustness: If a tab crashes, it does not affect other tabs or the entire browser. The other tabs and the browser 
    itself can continue to run without interruption.
  * Security: Each tab can run in a sandboxed environment with limited access to the operating system, which reduces 
    the impact of any security vulnerabilities in the browser.


  On the other hand it comes with a few disadvantages:
  * Resource overhead: Creating a new process for each tab can be resource-intensive, especially if the user has many
    tabs open. Each process requires its own memory, file descriptors, and network connections, which can lead to high
    memory usage and slow performance.
  * Communication overhead: Inter-process communication (IPC) is more expensive than inter-thread communication, as it 
    typically requires marshalling data between processes and using a slower communication mechanism, such as pipes or 
    sockets. This can lead to slow performance when tabs need to communicate with each other or with the browser.
  * Synchronization: Processes cannot share memory directly, so synchronization between processes requires using IPC 
    mechanisms, such as semaphores or message queues. This can be more complex than synchronization between threads.

Q4:

Q5:

  'real' time refers to the actual amount of time that has passed in the real world, while 'virtual' time refers to a 
  simulated or artificial representation of time.
  One example of using real time is in a program that measures the performance of a system like benchmarks. We want to know how much
  time it takes for an action to occured in real time.
  One scenario were we can use virtual time is games, I myself used 'virtual' time when building a game and. I've added 
  slowmotion affect by using virtual time. 
