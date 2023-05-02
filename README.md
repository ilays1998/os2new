ilaysoffer, rz121
Ilay Soffer (207446709), Ramon Zerem(209492156)

EX: 2

FILES:

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
