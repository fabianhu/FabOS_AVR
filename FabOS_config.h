/*
	FabOS user configuration file

	(c) 2008-2010 Fabian Huslik

	Please change this file to your needs.
*/

// *********  USER Configurable Block BEGIN

#define OS_NUMTASKS 3 // Number of (OS_Create)Tasks ; never >8 (idle task is not counted here!)
#define OS_NUMMUTEX 3 // Number of Mutexes

#if defined (__AVR_ATmega32__)
	#define OS_ScheduleISR TIMER1_COMPA_vect // Interrupt Vector used for OS-tick generation (check out CustomOS_ISRCode if you want to add isr code)
#elif defined (__AVR_ATxmega32A4__)
	#define OS_ScheduleISR TCC1_CCA_vect
#endif

#define OS_USECLOCK 1 		// Use "OS_GetTicks()" which returns a 32bit timer tick
#define OS_USECOMBINED 1 	// Use "OS_WaitEventTimeout()" which is easier to use, than combining alarms and events to get the functionality.
#define OS_USEEXTCHECKS 1	// prevent false use of API -> does not work, but no damage to OS stability.
#define OS_USEMEMCHECKS 1 	// Enable "OS_get_unused_Stack()" and "OS_GetQueueSpace()"
#define OS_UNUSEDMASK 0xEE  // unused Stack RAM will be filled with this byte, if OS_USEMEMCHECKS == 1.



// *********  USER Configurable Block END 

#define OS_DO_TESTSUITE 0	// compile and execute the automated software tests. Set to 0 for production use of OS.
