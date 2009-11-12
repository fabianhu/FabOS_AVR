/*
	FabOS for ATMEL AVR

*/


#include <avr/io.h>
#include <avr/interrupt.h>


// *********  USER Configurable Block BEGIN

#define NUMTASKS 3 // Number of (Create)Tasks ; never >8
#define NUMMUTEX 3 // Number of Mutexes

#define UNUSEDMASK 0xEE // unused Stack RAM will be filled with this byte.

#define OS_USECLOCK 1
#define OS_USECHECKS 1

#define OS_ScheduleISR TIMER1_COMPA_vect // Interrupt Vector used for OS-tick generation (check out CustomOS_ISRCode if you want to add isr code)

//#define TESTSUITE

#define BUFFER_SIZE 64 // must be 2^n (8, 16, 32, 64 ...)

// *********  USER Configurable Block END 

typedef struct FabOS_tag
{
#if OS_USECLOCK == 1
	uint32_t    OSTicks;				// the OS time, to prevent clutteded results, alway use the Function GetTime() to read it.	
#endif
	uint8_t		EventMask[NUMTASKS] ;	// The event masks for all the tasks; Index = Task ID
	uint8_t		EventWaiting[NUMTASKS]; // The mask indicates the events, the tasks are waiting for. Index = Task ID

	uint8_t 	MutexOwnedByTask[NUMMUTEX] ;	// Mutex-owner (contains task ID of owner)
	uint8_t 	MutexTaskWaiting[NUMTASKS+1] ;	// Mutex-waiters (contains mutex ID) ; Index = Task ID ; The last one is the idle task.

	uint8_t 	CurrTask; 				// here the NUMBER of the actual active task is set.
	uint8_t 	TaskReadyBits ; 		// here te task activation BITS are set. Task 0 (LSB) has the highest priority.
	uint16_t 	Stacks[NUMTASKS+1];		// actual SP position addresses for the tasks AND the IDLE-task, which uses the ordinary stack! Index = Task ID
	uint16_t 	AlarmTicks[NUMTASKS];  	// Holds the number of system clock ticks to wait before the task becomes ready to run. Index = Task ID

} FabOS_t;

#define BUFFER_MASK (BUFFER_SIZE-1) // don't forget the braces
 
typedef struct OS_Queue_tag {
  uint8_t read; // field with oldest content
  uint8_t write; // always empty field
  uint8_t data[BUFFER_SIZE];
} OS_Queue_t; 


extern FabOS_t MyOS;


// OS function prototypes


// user API
#define OS_TaskCreate( X, Y, Z )  OS_TaskCreateInt(X,Y, Z , sizeof(Z))// Macro to simplify the API
void OS_TaskDestroy( int8_t taskNum ); // destroy task

void OS_StartExecution() __attribute__ ((naked)) ; // Start the OS

void OS_SetEvent(uint8_t EventMask, uint8_t TaskID); // Set one or more events
uint8_t OS_WaitEvent(uint8_t EventMask); //returns event(s) in a mask, which lead to execution

void OS_mutexGet(int8_t mutexID); // number of mutexes limited to NUMMUTEX !!!
void OS_mutexRelease(int8_t mutexID);

void OS_Wait( uint16_t ) ; // OS API: Wait for a certain number of OS ticks
void OS_SetAlarm(uint8_t TaskID, uint16_t numTicks ); // set Alarm and continue
void OS_WaitAlarm(void); // set Alarm and continue

void OS_CustomISRCode(); // do not call; just fill in your code.

#if OS_USECHECKS == 1
uint16_t OS_Get_Unused_Stack (uint8_t*);
#endif
#if OS_USECLOCK == 1
void OS_GetTicks(uint32_t* pTime); // fills given variable with the OS ticks since start.
#endif

uint8_t OS_QueueIn(OS_Queue_t* pQueue , uint8_t byte); // Put byte into queue, return 1 if q full.
uint8_t OS_QueueOut(OS_Queue_t* pQueue, uint8_t *pByte); // Get a byte out of the queue, return 1 if q empty.


// internal OS functions, not to be called by the user.
ISR (OS_ScheduleISR)__attribute__ ((naked,signal)); // OS tick interrupt function (vector defined above)
void OS_TaskCreateInt( uint8_t taskNum, void (*t)(), uint8_t *stack, uint8_t stackSize ) ; // Create the task internal
void OS_Reschedule(void)__attribute__ ((naked)); // internal: Trigger re-scheduling
int8_t OS_GetNextTaskNumber(); // internal: get the next task to be run



// Save all CPU registers on the AVR chip.
// The last two lines save the Status Reg.
#define saveCPUContext() \
asm volatile( \
"	push r0\n\t\
	push r1\n\t\
	push r2\n\t\
	push r3\n\t\
	push r4\n\t\
	push r5\n\t\
	push r6\n\t\
	push r7\n\t\
	push r8\n\t\
	push r9\n\t\
	push r10\n\t\
	push r11\n\t\
	push r12\n\t\
	push r13\n\t\
	push r14\n\t\
	push r15\n\t\
	push r16\n\t\
	push r17\n\t\
	push r18\n\t\
	push r19\n\t\
	push r20\n\t\
	push r21\n\t\
	push r22\n\t\
	push r23\n\t\
	push r24\n\t\
	push r25\n\t\
	push r26\n\t\
	push r27\n\t\
	push r28\n\t\
	push r29\n\t\
	push r30\n\t\
	push r31\n\t\
	in r0,0x3f\n\t\
	push r0");

// Restore all AVR CPU Registers. The first two lines
// restore the stack pointer.
#define restoreCPUContext() \
asm volatile( \
"	pop r0\n\t\
	out 0x3f,r0\n\t\
	pop r31\n\t\
	pop r30\n\t\
	pop r29\n\t\
	pop r28\n\t\
	pop r27\n\t\
	pop r26\n\t\
	pop r25\n\t\
	pop r24\n\t\
	pop r23\n\t\
	pop r22\n\t\
	pop r21\n\t\
	pop r20\n\t\
	pop r19\n\t\
	pop r18\n\t\
	pop r17\n\t\
	pop r16\n\t\
	pop r15\n\t\
	pop r14\n\t\
	pop r13\n\t\
	pop r12\n\t\
	pop r11\n\t\
	pop r10\n\t\
	pop r9\n\t\
	pop r8\n\t\
	pop r7\n\t\
	pop r6\n\t\
	pop r5\n\t\
	pop r4\n\t\
	pop r3\n\t\
	pop r2\n\t\
	pop r1\n\t\
	pop r0");

#if NUMMUTEX > NUMTASKS 
	#warning something is wrong fritz..
#endif

#if NUMTASKS >8 
	#error only 8 tasks are allowed.
#endif

#if OS_USECHECKS == 0
	#undef UNUSEDMASK
#endif
