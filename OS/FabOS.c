/*
	FabOS for ATMEL AVR
	
	(c) 2008-2010 Fabian Huslik

	This software is free for use in private, educational or evaluation applications.
	For commercial use a license is necessary.

	contact FabOS@huslik-elektronik.de for support and license.

	In this file there should be no need to change anything.
	If you have to change something, please let the author know via FabOS@huslik-elektronik.de.

*/

#include "FabOS.h"

// variable types for more tasks
#if OS_NUMTASKS <= 8
#define OS_TypeTaskBits_t  uint8_t
#elif OS_NUMTASKS <= 16
#define OS_TypeTaskBits_t  uint16_t
#elif OS_NUMTASKS <= 32
#define OS_TypeTaskBits_t  uint32_t
#elif OS_NUMTASKS <= 64
#define OS_TypeTaskBits_t  uint64_t
#else
	#error reduce OS_NUMTASKS
#endif
FabOS_t MyOS; // the global instance of the OS struct

#if OS_TRACE_ON == 1
	uint8_t OS_Tracebuffer[OS_TRACESIZE];
	#if OS_TRACESIZE <= 0xff
		uint8_t OS_TraceIdx;
	#else
		uint16_t OS_TraceIdx;
	#endif
#endif

// From linker script
extern unsigned char __heap_start;

// *********  Timer Interrupt
// The naked attribute tells the compiler not to add code to push the registers it uses onto the stack or even add a RETI instruction at the end. 
// It just compiles the code inside the braces.
// *** No direct use of stack space inside a naked function, except embedding it into a function, as this creates a valid stack frame.
// or use "register unsigned char counter asm("r3")";  Typically, it should be safe to use r2 through r7 that way.
ISR  (OS_ScheduleISR) //__attribute__ ((naked,signal)) // Timer isr
{
	OS_Int_saveCPUContext() ; 
	MyOS.Stacks[MyOS.CurrTask] = SP ; // catch the SP before we (possibly) do anything with it.

	OS_TRACE(1);
	OS_CustomISRCode(); // Caller of Custom ISR code to be executed inside this ISR on the last active tasks stack
						// do early in ISR, as here a timer could be reset.
	OS_TRACE(2);

#if OS_USECLOCK == 1
	MyOS.OSTicks++; 	// tick the RT-clock...
#endif
	
	OS_Int_ProcessAlarms(); // Calculate alarms; function uses stack
	OS_TRACE(3);

	// task is to be run
	MyOS.CurrTask = OS_GetNextTaskNumber() ;
	SP = MyOS.Stacks[MyOS.CurrTask] ;
	OS_Int_restoreCPUContext() ;
	asm volatile("reti");  // at the XMEGA the I in SREG is statically ON before and after RETI.
}

// *********  Internal scheduling and priority stuff

void OS_Int_ProcessAlarms(void)
{
	uint8_t alarmID;
	OS_TRACE(4);

	// handling of OS_Wait / Alarms
	for(alarmID=0; alarmID < OS_NUMALARMS; alarmID++ )
	{ 
		if( MyOS.Alarms[alarmID].AlarmTicks > 0 ) // this task has to wait
		{
			OS_TRACE(5);
			MyOS.Alarms[alarmID].AlarmTicks--;
			if( MyOS.Alarms[alarmID].AlarmTicks == 0 ) // if now task is ready, it will be activated.
			{
				OS_TRACE(6);
				MyOS.TaskReadyBits |= 1<<(MyOS.Alarms[alarmID].TaskID) ; // now it is finished with waiting
			}
		}
	}
}

void OS_Reschedule(void) //with "__attribute__ ((naked))"
{
	OS_ENTERCRITICAL;
	OS_Int_saveCPUContext() ; 
	MyOS.Stacks[MyOS.CurrTask] = SP ; // catch the SP before we (possibly) do anything with it.

	OS_TRACE(7);

	// task is to be run
	MyOS.CurrTask = OS_GetNextTaskNumber();

	SP = MyOS.Stacks[MyOS.CurrTask] ;// set Stack pointer

	OS_TRACE(8);

	OS_Int_restoreCPUContext() ;
	OS_LEAVECRITICAL;
	asm volatile("ret"); 
}

int8_t OS_GetNextTaskNumber() // which is the next task (ready and highest (= rightmost) prio)?
{
	uint8_t Task;
	uint8_t	next= OS_NUMTASKS; // NO task is ready, which one to execute?? the idle task !!;

	OS_TypeTaskBits_t ReadyMask = MyOS.TaskReadyBits; // make working copy
	
	OS_TRACE(9);

	for (Task=0;Task<OS_NUMTASKS;Task++)
	{
		if (ReadyMask & 0x01) // last bit set
		{	
			OS_TRACE(10);
			next =  Task; // this task is the one to be executed
			break;
		}
		else
		{
			OS_TRACE(11);
			ReadyMask= (ReadyMask>>1); // shift to right; "Task" and the shift count is synchronous.
		}
	}
	// now "next" is the next highest prio task.

	// look in mutex waiting list, if any task is blocking "next", then "next" must not run.
	// if next is waiting for a task and the mutex is owned by a task then give the waiting the run.
	if (MyOS.MutexTaskWaiting[next] != 0xff) // "next" is waiting for a mutex
	{
		OS_TRACE(12);
		if(MyOS.MutexOwnedByTask[MyOS.MutexTaskWaiting[next]]!=0xff) // is the mutex still owned?
		{
			OS_TRACE(13);
			// which task is blocking it?
			next = MyOS.MutexOwnedByTask[MyOS.MutexTaskWaiting[next]]; 
			// the blocker gets the run.
			// this is also a priority inversion.
			if(((1<<next)&MyOS.TaskReadyBits) == 0)  // special case, where the blocker is not ready to run (waiting inside mutex)
			{
				OS_TRACE(14);
				next = OS_NUMTASKS; // the idle task gets the run...
			}
		}
	}
	return next;
}

// internal task create function
void OS_TaskCreateInt( void (*t)(), uint8_t TaskID, uint8_t *stack, uint16_t stackSize )
{
	uint16_t z ;
	OS_TRACE(15);

#if OS_USEMEMCHECKS == 1
	// "colorize" the stacks
	for (z=0;z<stackSize;z++)
	{
		stack[z] = OS_UNUSEDMASK+TaskID;
	}

	MyOS.StackStart[TaskID]=stack;
#endif

	for (z=stackSize-35;z<stackSize;z++) // clear the stack frame space (should already be, but you never know...)
	{
		stack[z] = 0;
	}
		
	MyOS.TaskReadyBits |= 1<<TaskID ;  // indicate that the task exists and is ready to run.

	MyOS.Stacks[TaskID] = (uint16_t)stack + stackSize - 1 ; // Point the task's SP to the top address of the array that represents its stack.

	*(uint8_t*)(MyOS.Stacks[TaskID]-1) = ((uint16_t)(t)) >> 8; // Put the address of the function that implements the task on its stack
	*(uint8_t*)(MyOS.Stacks[TaskID]) = ((uint16_t)(t)) & 0x00ff;

	MyOS.Stacks[TaskID] -= 35;   // Create a stack frame with placeholders for all the user registers as well as the SR.
}


// Start the operating system
void OS_StartExecution()
{
	uint8_t i;
	OS_TRACE(16);
	for(i=0; i < OS_NUMTASKS+1; i++ ) // init mutexes
	{ 
		MyOS.MutexTaskWaiting[i] = 0xff;
	}
	for(i=0; i < OS_NUMMUTEX; i++ ) 
	{ 
		MyOS.MutexOwnedByTask[i] = 0xff;
	}

#if OS_USEMEMCHECKS == 1
	uint8_t* stack = (uint8_t*)SP;
	MyOS.StackStart[OS_NUMTASKS] = &__heap_start;
	// "colorize" the stacks
	while (stack > MyOS.StackStart[OS_NUMTASKS])
	{
		*--stack = OS_UNUSEDMASK+OS_NUMTASKS;
	}
#endif

	//store THIS context for idling!!
	MyOS.CurrTask = OS_NUMTASKS;
	OS_Reschedule();
	OS_LEAVECRITICAL; // the stored context has the interrupts OFF!
}


// ************************** MUTEX

// Try to get a mutex; execution will block as long the mutex is occupied. If it is free, it is occupied afterwards.
void OS_MutexGet(int8_t mutexID)
{
#if OS_USEEXTCHECKS == 1
	if(mutexID >= OS_NUMMUTEX)
	{
		OS_ErrorHook(5);// OS_MutexGet: invalid Mutex number
		return;
	}
#endif
	OS_ENTERCRITICAL;
	OS_TRACE(17);
	while( MyOS.MutexOwnedByTask[mutexID] != 0xff) // as long as anyone is the owner..
	{
		OS_TRACE(18);
		MyOS.MutexTaskWaiting[MyOS.CurrTask] = mutexID; // set waiting info for priority inversion of scheduler
		OS_Reschedule(); // also re-enables interrupts...
		OS_ENTERCRITICAL;
		OS_TRACE(19);
		// we only get here, if the other has released the mutex.
		MyOS.MutexTaskWaiting[MyOS.CurrTask] = 0xff; // remove waiting info
	}
	OS_TRACE(20);
	MyOS.MutexOwnedByTask[mutexID] = MyOS.CurrTask; // tell others, that I am the owner.
	OS_LEAVECRITICAL;
}

// release the occupied mutex
void OS_MutexRelease(int8_t mutexID)
{
#if OS_USEEXTCHECKS == 1
	if(mutexID >= OS_NUMMUTEX)
	{
		OS_ErrorHook(6);// OS_MutexRelease: invalid Mutex number
		return;
	}
#endif
	OS_ENTERCRITICAL;
	OS_TRACE(21);
	MyOS.MutexOwnedByTask[mutexID] = 0xff; // tell others, that no one is the owner.
	OS_Reschedule() ; // re-schedule; will wake up waiting task, if higher prio.
}

// ************************** EVENTS

void OS_SetEvent(uint8_t TaskID, uint8_t EventMask) // Set one or more events
{
	OS_ENTERCRITICAL;
	OS_TRACE(22);
	MyOS.EventMask[TaskID] |= EventMask; // set the event mask, as there may be more events than waited for.

	if(EventMask & MyOS.EventWaiting[TaskID]) // Targeted task is waiting for one of this events
	{
		OS_TRACE(23);
		// wake up this task directly
		MyOS.TaskReadyBits |= 1<<TaskID ;   // Make the task ready to run again.

		// the waked up task will then clean up all entries to the event

		OS_Reschedule() ; // re-schedule; will wake up the sleeper directly, if higher prio.
	}
	else
	{
		OS_TRACE(24);
		// remember the event and task continues on its call of WaitEvent directly. 
		OS_LEAVECRITICAL;
	}
}

uint8_t OS_WaitEvent(uint8_t EventMask) //returns event(s), which lead to execution
{
#if OS_USEEXTCHECKS == 1
	if(MyOS.CurrTask == OS_NUMTASKS) 
	{
		OS_ErrorHook(2);// OS_WaitEvent: waiting in idle is not allowed
		return 0; 
	}
#endif

	uint8_t ret;
	OS_ENTERCRITICAL;
	OS_TRACE(25);
	
	if(!(EventMask & MyOS.EventMask[MyOS.CurrTask])) // This task is Not having one of these events active
	{
		OS_TRACE(26);
		MyOS.EventWaiting[MyOS.CurrTask] = EventMask; // remember what this task is waiting for
		// no event yet... waiting
		MyOS.TaskReadyBits &= ~(1<<MyOS.CurrTask) ;     // indicate that this task is not ready to run.

		OS_Reschedule() ; // re-schedule; will be waked up here by "SetEvent" or alarm
		OS_ENTERCRITICAL;
		OS_TRACE(27);

		MyOS.EventWaiting[MyOS.CurrTask] = 0; // no more waiting!
	}
	ret = MyOS.EventMask[MyOS.CurrTask] & EventMask;
	// clear the events:
	MyOS.EventMask[MyOS.CurrTask] &= ~EventMask; // the actual events minus the ones, which have been waited for 
	OS_TRACE(28);
	OS_LEAVECRITICAL;
	return ret;
}


// ************************** ALARMS


void OS_SetAlarm(uint8_t AlarmID, OS_TypeAlarmTick_t numTicks ) // set Alarm for the future and continue // set alarm to 0 disable an alarm.
{
	OS_ENTERCRITICAL;
	OS_TRACE(29);
#if OS_USEEXTCHECKS == 1
	if(AlarmID >= OS_NUMALARMS)// check for ID out of range
	{
		OS_ErrorHook(7); // ID bigger than array size
		return;
	}
#endif	
	MyOS.Alarms[AlarmID].AlarmTicks = numTicks ;
	OS_LEAVECRITICAL;
}

void OS_WaitAlarm(uint8_t AlarmID) // Wait for any Alarm set by OS_SetAlarm
{
#if OS_USEEXTCHECKS == 1
	if(AlarmID >= OS_NUMALARMS)// check for ID out of range
	{
		OS_ErrorHook(7); // ID bigger than array size
		return;
	}
	if(MyOS.CurrTask == OS_NUMTASKS) 
	{
		OS_ErrorHook(4);// OS_WaitAlarm: waiting in idle is not allowed
		return;  
	}
	if(MyOS.Alarms[AlarmID].TaskID != MyOS.CurrTask) // Alarm is not assigned!
	{
		OS_TRACE(32);
		OS_ErrorHook(9); // OS_WaitAlarm: Alarm is not assigned to the task
		return;  
	}
#endif

	OS_ENTERCRITICAL; // re-enabled by OS_Schedule()
	OS_TRACE(30);
	if(MyOS.Alarms[AlarmID].AlarmTicks == 0 ) // notice: this "if" could be possibly omitted.
	{
#if OS_USEEXTCHECKS == 1
		OS_ErrorHook(8); // OS_WaitAlarm: Alarm was not active
#endif
		OS_LEAVECRITICAL; // just continue
		return;  
	}
	else
	{
		OS_TRACE(33);
		MyOS.TaskReadyBits &= ~(1<<MyOS.CurrTask) ;  // Disable this task
		OS_Reschedule();  // re-schedule; let the others run...	
	}
}

// ************************** QUEUES
/* in FabOS.h we have:
typedef struct OS_Queue_tag {
	  uint8_t read; // field with oldest content
	  uint8_t write; // always empty field
	  uint8_t chunk;
	  uint8_t size;
	  uint8_t* data;
	} OS_Queue_t;
*/

uint8_t OS_QueueIn(OS_Queue_t* pQueue , uint8_t* pByte)
{
	uint8_t i;
	OS_ENTERCRITICAL;
	OS_TRACE(33);
	if (pQueue->write + pQueue->chunk == pQueue->read || (pQueue->read == 0 && pQueue->write + pQueue->chunk == pQueue->size))
	{
		OS_TRACE(34);
		OS_LEAVECRITICAL;
		return 1;  // queue full
	}

	for(i=0;i<pQueue->chunk;i++)
	{
		pQueue->data[pQueue->write] = *pByte++;
		pQueue->write = pQueue->write + 1;
		if (pQueue->write >= pQueue->size)
			pQueue->write = 0;
	}
	OS_TRACE(35);
	OS_LEAVECRITICAL;
	return 0;
}
 
uint8_t OS_QueueOut(OS_Queue_t* pQueue , uint8_t* pByte)
{
	uint8_t i;
	OS_ENTERCRITICAL;
	OS_TRACE(36);
	if (pQueue->read == pQueue->write)
	{
		OS_TRACE(37);
		OS_LEAVECRITICAL;
		return 1; // queue empty
	}

	for(i=0;i<pQueue->chunk;i++)
	{
		*pByte++ = pQueue->data[pQueue->read];
		pQueue->read = pQueue->read + 1;
		if (pQueue->read >= pQueue->size)
			pQueue->read = 0;
	}
	OS_TRACE(38);
	OS_LEAVECRITICAL;
	return 0;
}

#if OS_USEMEMCHECKS == 1
uint8_t OS_GetQueueSpace(OS_Queue_t* pQueue)
{
	if (pQueue->read < pQueue->write)
		return pQueue->size - pQueue->write + pQueue->read;
	else if(pQueue->read > pQueue->write)
		return  (pQueue->read - pQueue->write)-1;
	return pQueue->size-1;
}
#endif

// *********************** Aux functions

#if OS_USEMEMCHECKS == 1
// give the free stack space for any task in function result.
uint16_t OS_GetUnusedStack (uint8_t TaskID)
{
   uint16_t unused = 0;
   uint8_t *p = MyOS.StackStart[TaskID]; 

   do
   {
      if (*p++ != OS_UNUSEDMASK+TaskID)
         break;

      unused++;
   } while (p <= (uint8_t*) MyOS.Stacks[TaskID]);

      return unused;
}
#endif

#if OS_USECLOCK == 1
// fills given variable with the OS ticks since start.
void OS_GetTicks(uint32_t* pTime) 
{
	OS_ENTERCRITICAL;
		*pTime = MyOS.OSTicks;
	OS_LEAVECRITICAL;
}
#endif


#if OS_USECOMBINED
uint8_t OS_WaitEventTimeout(uint8_t EventMask, uint8_t AlarmID, OS_TypeAlarmTick_t numTicks ) //returns event on event, 0 on timeout.
{
	uint8_t ret;
	OS_SetAlarm(AlarmID,numTicks); // set timeout
	ret = OS_WaitEvent(EventMask);
	if(ret & EventMask)
	{
		// event occured
		OS_SetAlarm(AlarmID,0); // disable timeout
		return ret;
	}
	else
	{
		// timeout occured
		return 0;
	}
}
#endif

