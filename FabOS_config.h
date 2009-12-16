/*
	FabOS for ATMEL AVR user configuration file
	
	(c) 2009 Fabian Huslik
*/

// *********  USER Configurable Block BEGIN

#define OS_NUMTASKS 3 // Number of (OS_Create)Tasks ; never >8 (idle task is not counted here!)
#define OS_NUMMUTEX 3 // Number of Mutexes

#define OS_ScheduleISR TIMER1_COMPA_vect // Interrupt Vector used for OS-tick generation (check out CustomOS_ISRCode if you want to add isr code)

#define OS_USECLOCK 1 		// Use "OS_GetTicks()" which returns a 32bit timer tick
#define OS_USECOMBINED 1 	// Use "OS_WaitEventTimeout()" which is easier to use, than combining alarms and events to get the functionality.
#define OS_USEEXTCHECKS 1	// prevent false use of API -> does not work, but no damage to OS stability.
#define OS_USEMEMCHECKS 1 	// Use "OS_get_unused_Stack()" which returns free stack space for each task
#define OS_UNUSEDMASK 0xEE  // unused Stack RAM will be filled with this byte.


// *********  USER Configurable Block END 

#define OS_DO_TESTSUITE 1	// compile and execute the automated software tests. set to 0 for production use of OS.
