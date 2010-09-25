/*
	FabOS for ATMEL AVR user configuration file
	
	(c) 2008-2010 Fabian Huslik

	Please change this file to your needs.
*/

// *********  USER Configurable Block BEGIN

#define OS_NUMTASKS  3 // Number of (OS_Create)Tasks ; never >64 (idle task is not counted here!)
#define OS_NUMMUTEX  3 // Number of Mutexes
#define OS_NUMALARMS 5 // Number of Alarms

#if defined (__AVR_ATmega32__)
	#define OS_ScheduleISR 			TIMER1_COMPA_vect // Interrupt Vector used for OS-tick generation (check out CustomOS_ISRCode if you want to add isr code)
	#define OS_ALLOWSCHEDULING 		TIMSK |= (1<<OCIE1A);	// turn Timer Interrupt ON
	#define OS_PREVENTSCHEDULING 	TIMSK &= ~(1<<OCIE1A); // turn Timer Interrupt OFF
#elif defined (__AVR_ATmega644P__)
	#define OS_ScheduleISR 			TIMER1_COMPA_vect
	#define OS_ALLOWSCHEDULING 		TIMSK1 |= (1<<OCIE1A);	// turn Timer Interrupt ON
	#define OS_PREVENTSCHEDULING 	TIMSK1 &= ~(1<<OCIE1A); // turn Timer Interrupt OFF
#elif defined (__AVR_ATxmega32A4__)
	#define OS_ScheduleISR 			TCC1_CCA_vect
	#define OS_ALLOWSCHEDULING 		TCC1.INTCTRLB |= 3 ;//;	// turn Timer Interrupt ON
	#define OS_PREVENTSCHEDULING 	TCC1.INTCTRLB &= ~3 ; // turn Timer Interrupt OFF
#else
	#error "MCU Timer ISR not defined. Set correct ISR vector in FabOS_config.h"
#endif

#define OS_USECLOCK 1 		// Use "OS_GetTicks()" which returns a 32bit timer tick
#define OS_USECOMBINED 1 	// Use "OS_WaitEventTimeout()" which is easier to use, than combining alarms and events to get the functionality.
#define OS_USEEXTCHECKS 1	// check wrong usage of OS API -> does not work, but no damage to OS stability.
#define OS_USEMEMCHECKS 1 	// Enable "OS_get_unused_Stack()" and "OS_GetQueueSpace()"
#define OS_UNUSEDMASK 0xAA  // unused Stack RAM will be filled with this byte, if OS_USEMEMCHECKS == 1.
#define OS_TRACE_ON  0 		// enable trace to OS_Tracebuffer[]
#define OS_TRACESIZE 1000	// size of OS_Tracebuffer[] (depending on memory left ;-)

#define OS_TypeAlarmTick_t uint16_t // change this type to uint32_t, if you need longer wait time than 65535 OS ticks.

// Task definitions

#define OSTSK0 0
#define OSTSK1 1
#define OSTSK2 2
#define OSTSK3 3

// Event Names

#define OSEVT1 1

// Mutex Names

#define OSMTX0 0
#define OSMTX1 1

// Alarm Names

#define OSALM1  0
#define OSALM2  1
#define OSALM3  2

// *********  USER Configurable Block END 

#define OS_DO_TESTSUITE 1	// compile and execute the automated software tests. Set to 0 for production use of OS.
