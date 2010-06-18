/*
	FabOS for ATMEL AVR
	
	(c) 2008-2010 Fabian Huslik

	This software is free for use in private, educational or evaluation applications.
	For commercial use a license is necessary.

	contact FabOS@huslik-elektronik.de for support and license.

	In this file there should be no need to change anything.
	If you have to change something, please let the author know via FabOS@huslik-elektronik.de.

*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "../FabOS_config.h"

typedef struct OS_Queue_tag {
	  uint8_t read; 	// field with oldest content
	  uint8_t write;	// always empty field
	  uint8_t chunk;	// size of element
	  uint8_t size;		// number of elements
	  uint8_t* data;	// pointer to data
	} OS_Queue_t;

typedef struct OS_Alarm_tag {
	uint8_t 			TaskID; // Task ID to wake up
	OS_TypeAlarmTick_t 	AlarmTicks; // ticks to count down before reactivation
} OS_Alarm_t;

// *********  Macros to simplify the API

#define OS_DeclareQueue(NAME,COUNT,CHUNK) uint8_t OSQD##NAME[(COUNT+1)*CHUNK]; OS_Queue_t NAME = {0,0,CHUNK,(COUNT+1)*CHUNK,OSQD##NAME}

#define OS_DeclareTask(NAME,STACKSIZE) void NAME(void); uint8_t OSStack##NAME[STACKSIZE];

#define OS_CreateTask(NAME, PRIO)  OS_TaskCreateInt(NAME, PRIO, OSStack##NAME , sizeof(OSStack##NAME))


// *********  OS function prototypes

void 	OS_CustomISRCode(); // do not call; just fill in your code.

void 	OS_StartExecution(); // Start the operating system

void 	OS_SetEvent(uint8_t TaskID, uint8_t EventMask); // Set one or more events

uint8_t OS_WaitEvent(uint8_t EventMask); //returns event(s) in a mask, which lead to execution

void 	OS_MutexGet(int8_t mutexID); // number of mutexes limited to OS_NUMMUTEX !!!
				// Try to get a mutex; execution will block as long the mutex is occupied. If it is free, it is occupied afterwards.

void 	OS_MutexRelease(int8_t mutexID); // release the occupied mutex

void 	OS_CreateAlarm(uint8_t ALARMID, uint8_t TASKID);  // number of alarms limited to OS_NUMALARMS !!!

void 	OS_SetAlarm(uint8_t AlarmID, OS_TypeAlarmTick_t numTicks ); // set Alarm for the future and continue // set alarm to 0 disable it.

void 	OS_WaitAlarm(uint8_t AlarmID); // Wait for an Alarm set by OS_SetAlarm

uint8_t OS_QueueIn(OS_Queue_t* pQueue , uint8_t *pData); // Put byte into queue, return 1 if q full.

uint8_t OS_QueueOut(OS_Queue_t* pQueue, uint8_t *pData); // Get a byte out of the queue, return 1 if q empty.

#if OS_USEEXTCHECKS == 1
	void OS_ErrorHook(uint8_t);
#endif

#if OS_USEMEMCHECKS == 1
uint16_t OS_GetUnusedStack (uint8_t TaskID); // give the free stack space for any task as result.
uint8_t OS_GetQueueSpace(OS_Queue_t* pQueue); // give the free space in a queue
#endif

#if OS_USECLOCK == 1
void 	OS_GetTicks(uint32_t* pTime); // fills given variable with the OS ticks since start.
#endif

#if OS_DO_TESTSUITE == 1
void 	OS_TestSuite(void); // execute regression test of FabOS (OS development only)
#endif


// Wait for a certain number of OS-ticks (1 = wait to the next timer interrupt)

#if OS_USECOMBINED == 1
uint8_t OS_WaitEventTimeout(uint8_t EventMask, uint8_t AlarmID, OS_TypeAlarmTick_t numTicks ); //returns event on event, 0 on timeout.
#endif

#define OS_WaitTicks(A,X) do{\
		OS_SetAlarm(A,X);\
		OS_WaitAlarm(A);\
		}while(0)



// *********  internal OS functions, not to be called by the user.
ISR (OS_ScheduleISR)__attribute__ ((naked,signal)); // OS tick interrupt ISR (vector #defined in FabOS_config.h)

void OS_TaskCreateInt( void (*t)(), uint8_t taskNum, uint8_t *stack, uint8_t stackSize ) ; // Create the task internal


// *********  CPU related assembler stuff

#define OS_ENTERCRITICAL cli();
#define OS_LEAVECRITICAL sei();




// *********  some warning calculations


#if (!defined(OS_NUMTASKS		))||\
	(!defined(OS_NUMMUTEX 		))||\
	(!defined(OS_NUMALARMS		))||\
	(!defined(OS_ScheduleISR 	))||\
	(!defined(OS_USECLOCK 		))||\
	(!defined(OS_USECOMBINED 	))||\
	(!defined(OS_USEEXTCHECKS   ))||\
	(!defined(OS_USEMEMCHECKS 	))||\
	(!defined(OS_UNUSEDMASK 	))||\
	(!defined(OS_TRACE_ON       ))||\
	(!defined(OS_TRACESIZE      ))
	#error not all defines in FabOS_config.h are done as described here (FabOS.h) below!
#endif


/* Example defines for FabOS_config.h     

#define OS_NUMTASKS  4 // Number of (OS_Create)Tasks ; never >8 (idle task is not counted here!)
#define OS_NUMMUTEX  3 // Number of Mutexes
#define OS_NUMALARMS 5 // Number of Alarms

#define OS_ScheduleISR TIMER1_COMPA_vect // Interrupt Vector used for OS-tick generation (check out CustomOS_ISRCode if you want to add isr code)

#define OS_USECLOCK 1 		// Use "OS_GetTicks()" which returns a 32bit timer tick
#define OS_USECOMBINED 1 	// Use "OS_WaitEventTimeout()" which is easier to use, than combining alarms and events to get the functionality.
#define OS_USEEXTCHECKS 1	// check wrong usage of OS API -> does not work, but no damage to OS stability.
#define OS_USEMEMCHECKS 1 	// Enable "OS_get_unused_Stack()" and "OS_GetQueueSpace()"
#define OS_UNUSEDMASK 0xEE  // unused Stack RAM will be filled with this byte, if OS_USEMEMCHECKS == 1.
#define OS_TRACE_ON  1 		// enable trace to OS_Tracebuffer[]
#define OS_TRACESIZE 1000	// size of OS_Tracebuffer[] (depending on memory left ;-)

*/


#if NUMMUTEX > NUMTASKS 
	#warning more mutexes than tasks? are you serious with that concept?
#endif

#if NUMTASKS >8 
	#error only 8 tasks are possible, if you want more, you have to change some datatypes inside the FabOS_t struct typedef and where these are used..
#endif


#if (OS_DO_TESTSUITE == 1) && (\
		(	OS_NUMTASKS 	!=3	) ||\
		(	OS_NUMMUTEX 	!=3	) ||\
		(	OS_USEEXTCHECKS !=1 ) ||\
		(	OS_USECLOCK 	!=1	) ||\
		(	OS_USEMEMCHECKS !=1	) ||\
		(	OS_USECOMBINED 	!=1	) \
		) 
		#error please configure the defines for the testsuite as stated above!
#endif

#if OS_USEMEMCHECKS == 0
	#undef UNUSEDMASK
#endif

#if OS_UNUSEDMASK+OS_NUMTASKS > 0xff
	#error please redefine OS_UNUSEDMASK to a smaller number!
#endif
