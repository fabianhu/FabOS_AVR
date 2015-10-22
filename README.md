# FabOS for Atmel AVR / XMEGA

FabOS is a lightweight embedded real time operating system, which is completely written in C.

The Programming is -as far as possible- independent of the compiler and processor.

A stable implementation for Atmel AVR is available.

ATMEGA32 and ATXMEGAxxA4 are directly supported, porting to another derivate is quite simple.

The "FabOS Programmers Guide" including a programming example is available for download.

 
Key features:

    Preemptive real time task scheduling
    Mutexes with priority inversion
    Up to 8 Events per task
    User defined Queues
    Multiple timed Alarms per Task
    Flexible OS-tick timer interrupt - any interrupt source can be used for scheduling
    The scheduling interrupt remains usable for the application
    Idle task, which runs on "native" stack
    Optional run-time checks
    Easy to use and to understand

Footprint:

For the AVR (MEGA32) implementation with 3 Tasks (4 including idle), 3 alarms and 3 mutexes, it takes 38 to 51 bytes RAM and 1774 to 2196 bytes code, depending on features used.
The values are net values. The stack space for every task and possibly used queue space is to be added.
