/*
	FabOS for ATMEL AVR
	tested on WinAVR 20090913 and Mega32
	
	(c) 2009 Fabian Huslik

*/

#include "FabOS.h"

FabOS_t MyOS; // the global instance of the OS struct

// From linker script
extern unsigned char __heap_start;

// *********  Timer Interrupt

// ISR(TIMER0_OVF_vect) __attribute__ ((naked,signal));
// Prototype for the INT0 ISR is in FabOS.h. The naked attribute
// tells the compiler not to add code to push the registers
// it uses onto the stack or even add a RETI instruction
// at the end. It just compiles the code inside the braces.
// *** No direct use of stack space inside a naked function, except embedding it into a function, as this creates a valid stack frame.
// or use "register unsigned char counter asm("r3")";  Typically, it should be safe to use r2 through r7 that way.
ISR  (OS_ScheduleISR) //__attribute__ ((naked,signal)) // Timer isr
{
	OS_Int_saveCPUContext() ; 
	MyOS.Stacks[MyOS.CurrTask] = SP ; // catch the SP before we (possibly) do anything with it.

#if OS_USECLOCK == 1
	MyOS.OSTicks++; 	// tick the RT-clock...
#endif
	
	OS_CustomISRCode(); // Caller of Custom ISR code to be executed inside this ISR on the last active tasks stack

	OS_Int_ProcessAlarms(); // Calculate alarms; function uses stack

	// task is to be run
	MyOS.CurrTask = OS_GetNextTaskNumber() ;
	SP = MyOS.Stacks[MyOS.CurrTask] ;
	OS_Int_restoreCPUContext() ;
	asm volatile("reti"); 
}

// *********  Internal scheduling and priority stuff

void OS_Int_ProcessAlarms(void)
{
	uint8_t taskID;
	// handling of OS_Wait / Alarms
	for(taskID=0; taskID < OS_NUMTASKS; taskID++ ) 
	{ 
		if( MyOS.AlarmTicks[taskID] > 0 ) // this task has to wait
		{
			if( --MyOS.AlarmTicks[taskID] == 0 ) // if now task is ready, it will be activated.
			{
				MyOS.TaskReadyBits |= 1<<(taskID) ; // now it is finished with waiting
			}
		}
	}
}

void OS_Reschedule(void) //with "__attribute__ ((naked))"
{
	OS_ENTERCRITICAL;
	OS_Int_saveCPUContext() ; 
	MyOS.Stacks[MyOS.CurrTask] = SP ; // catch the SP before we (possibly) do anything with it.

	// task is to be run
	MyOS.CurrTask = OS_GetNextTaskNumber();

	SP = MyOS.Stacks[MyOS.CurrTask] ;// set Stack pointer
	OS_Int_restoreCPUContext() ;
	asm volatile("reti"); 
}

int8_t OS_GetNextTaskNumber() // which is the next task (ready and highest (= rightmost) prio)?
{
	uint8_t Task;
	uint8_t	next= OS_NUMTASKS; // NO task is ready, which one to execute?? the idle task !!;

	uint8_t ReadyMask= MyOS.TaskReadyBits; // make working copy
	
	for (Task=0;Task<OS_NUMTASKS;Task++)
	{
		if (ReadyMask & 0x01) // last bit set
		{	
			next =  Task; // this task is the one to be executed
			break;
		}
		else
		{
			ReadyMask= (ReadyMask>>1); // shift to right; i and the shift count is synchronous.
		}
	}
	// now "next" is the next highest prio task.

	// look in mutex waiting list, if any task is blocking "next", then "next" must not run.
	// if next is waiting for a task and the mutex is owned by a task then give the waiter the run.
	if (MyOS.MutexTaskWaiting[next] != 0xff) // "next" is waiting for a mutex
	{
		if(MyOS.MutexOwnedByTask[MyOS.MutexTaskWaiting[next]]!=0xff) // is the mutex still owned?
		{
			// which task is blocking it?
			next = MyOS.MutexOwnedByTask[MyOS.MutexTaskWaiting[next]]; 
			// the blocker gets the run.
			// this is also a priority inversion.
			if(((1<<next)&MyOS.TaskReadyBits) == 0)  // special case, where the blocker is not ready to run (waiting inside mutex)
			{
				next = OS_NUMTASKS; // the idle task gets the run...
			}
		}
	}
	return next;
}

// internal task create function
void OS_TaskCreateInt( uint8_t TaskID, void (*t)(), uint8_t *stack, uint8_t stackSize ) 
{
	uint16_t z ;
#if OS_USEMEMCHECKS == 1
	// "colorize" the stacks
	for (z=0;z<stackSize;z++)
	{
		stack[z] = OS_UNUSEDMASK;
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
		*--stack = OS_UNUSEDMASK;
	}
#endif

	//store THIS context for idling!!
	MyOS.CurrTask = OS_NUMTASKS;
	OS_Reschedule();
}


// ************************** MUTEX

// Try to get a mutex; execution will block as long the mutex is occupied. If it is free, it is occupied afterwards.
void OS_MutexGet(int8_t mutexID)
{
	OS_ENTERCRITICAL;
	while( MyOS.MutexOwnedByTask[mutexID] != 0xff) // as long as anyone is the owner..
	{
		MyOS.MutexTaskWaiting[MyOS.CurrTask] = mutexID; // set waiting info for priority inversion of scheduler
		OS_Reschedule(); // also re-enables interrupts...
		OS_ENTERCRITICAL;
		MyOS.MutexTaskWaiting[MyOS.CurrTask] = 0xff; // remove waiting info
	}
	MyOS.MutexOwnedByTask[mutexID] = MyOS.CurrTask; // tell others, that I am the owner.
	OS_LEAVECRITICAL;
}

// release the occupied mutex
void OS_MutexRelease(int8_t mutexID)
{
	OS_ENTERCRITICAL;
	MyOS.MutexOwnedByTask[mutexID] = 0xff; // tell others, that no one is the owner.
	OS_Reschedule() ; // re-schedule; will wake up waiting task, if higher prio.
}

// ************************** EVENTS

void OS_SetEvent(uint8_t EventMask, uint8_t TaskID) // Set one or more events
{
	OS_ENTERCRITICAL;
	MyOS.EventMask[TaskID] |= EventMask; // set the event mask, as there may be more events than waited for.

	if(EventMask & MyOS.EventWaiting[TaskID]) // Targeted task is waiting for one of this events
	{
		// wake up this task directly
		MyOS.TaskReadyBits |= 1<<TaskID ;   // Make the task ready to run again.

		MyOS.EventWaiting[TaskID] = 0; // no more waiting!
		// clear the events:
		MyOS.EventMask[TaskID] &= ~EventMask; // the actual events minus the ones, which have been waited for 

		OS_Reschedule() ; // re-schedule; will wake up the sleeper directly, if higher prio.
	}
	else
	{
		// remember the event and task continues on its call of WaitEvent directly. 
		OS_LEAVECRITICAL;
	}
}

uint8_t OS_WaitEvent(uint8_t EventMask) //returns event(s), which lead to execution
{
#if OS_USEEXTCHECKS == 1
	if(MyOS.CurrTask == OS_NUMTASKS) return 0;
#endif

	uint8_t ret;
	OS_ENTERCRITICAL;
	
	if(!(EventMask & MyOS.EventMask[MyOS.CurrTask])) // This task is Not having one of these events active
	{
		MyOS.EventWaiting[MyOS.CurrTask] = EventMask; // remember what this task is waiting for
		// no event yet... waiting
		MyOS.TaskReadyBits &= ~(1<<MyOS.CurrTask) ;     // indicate that this task is not ready to run.

		OS_Reschedule() ; // re-schedule; will be waked up here by "SetEvent" or alarm
		OS_ENTERCRITICAL;

		MyOS.EventWaiting[MyOS.CurrTask] = 0; // no more waiting!
	}
	ret = MyOS.EventMask[MyOS.CurrTask] & EventMask;
	// clear the events:
	MyOS.EventMask[MyOS.CurrTask] &= ~EventMask; // the actual events minus the ones, which have been waited for 
	OS_LEAVECRITICAL;
	return ret;
}

/*
Mutex Prinzip:

Ein Task holt sich den Mutex
Taskwechsel
Ein anderer B hätte Ihn gerne
Muss warten, Task A kommt dran
Task A ist fertig und gibt an Task B ab, der ja immer noch eigentlich dran wäre.

Der Task, der den Mutex haben will, muss schauen, ob ihn nicht ein anderer hat.
Wenn nein, holt er sich den Mutex.
Wenn ja, bekommt der Task, der den Mutex hat, den Prozessor.
Wenn der fertig ist, ruft er den scheduler auf, der den nächsten Blockierer aufruft.

Mutex zustände:
Task schaut, ob der Mutex frei ist (0xff)
Inhaber schreibt seine ID in den Mutex (Array), wenn er ihn hat.

Reschedule ISR stößt auf eine Mutex-Waiting-Situation:
Task prio rausfinden; 
Wenn der Task, der dran wäre, nicht auf einen Mutex wartet, 
oder einen hat, weiter wie normal.

Wenn der Task, der dran wäre, auf einen Mutex wartet, 
muss der Inhaber des Mutex drankommen. Nach Abschluss scheduled der nochmal.

alles klar?

*/


// ************************** ALARMS

void OS_SetAlarm(uint8_t TaskID, uint16_t numTicks ) // set Alarm for the future and continue // set alarm to 0 disable an alarm.
{
	OS_ENTERCRITICAL;
	MyOS.AlarmTicks[TaskID] = numTicks ;
	OS_LEAVECRITICAL;
}

void OS_WaitAlarm(void) // Wait for an Alarm set by OS_SetAlarm 
{
#if OS_USEEXTCHECKS == 1
	if(MyOS.CurrTask == OS_NUMTASKS) return;
#endif
	OS_ENTERCRITICAL; // re-enabled by reti in OS_Schedule()
	if(MyOS.AlarmTicks[MyOS.CurrTask] > 0) // notice: this "if" could be possibly omitted.
	{
		MyOS.TaskReadyBits &= ~(1<<MyOS.CurrTask) ;  // Disable this task
		OS_Reschedule();  // re-schedule; let the others run...
	}
	else
	{
		OS_LEAVECRITICAL; // just continue
	}
}

// ************************** QUEUES

uint8_t OS_QueueIn(OS_Queue_t* pQueue , uint8_t byte) // Put byte into queue, return 1 if q full.
{
	uint8_t next;
	OS_ENTERCRITICAL;
	next = ((pQueue->write + 1) & OS_QUEUE_MASK);
	if (pQueue->read == next)
	{
		OS_LEAVECRITICAL;
		return 1; // queue full
	}
	pQueue->data[pQueue->write] = byte;
	// pQueue->data[pQueue->write & QUEUE_MASK] = byte; // absolute secure
	pQueue->write = next;
	OS_LEAVECRITICAL;
	return 0;
}
 
uint8_t OS_QueueOut(OS_Queue_t* pQueue, uint8_t *pByte) // Get a byte out of the queue, return 1 if q empty.
{
	OS_ENTERCRITICAL;
	if (pQueue->read == pQueue->write)
	{
		OS_LEAVECRITICAL;
		return 1; // queue empty
	}
	*pByte = pQueue->data[pQueue->read];
	pQueue->read = (pQueue->read+1) & OS_QUEUE_MASK;
	OS_LEAVECRITICAL;
	return 0;
}


/* alternatively

#define QUEUE_SIZE 23
 
struct Queue {
  uint8_t data[QUEUE_SIZE];
  uint8_t read; // zeigt auf das Feld mit dem ältesten Inhalt
  uint8_t write; // zeigt immer auf leeres Feld
} Queue = {{}, 0, 0};
 
uint8_t QueueIn(uint8_t byte)
{
  //if (write >= QUEUE_SIZE)
  //  write = 0; // erhöht sicherheit
 
  if (Queue.write + 1 == Queue.read || Queue.read == 0 && Queue.write + 1 == Queue_SIZE)
    return FAIL; // voll
 
  Queue.data[Queue.write] = byte;
 
  Queue.write = Queue.write + 1;
  if (Queue.write >= QUEUE_SIZE)
    Queue.write = 0;
}
 
uint8_t QueueOut(uint8_t *pByte)
{
  if (Queue.read == Queue.write)
    return FAIL;
  *pByte = Queue.data[Queue.read];
 
  Queue.read = Queue.read + 1;
  if (Queue.read >= QUEUE_SIZE)
    Queue.read = 0;
  return SUCCESS;
}


*/


// *********************** Aux functions


#if OS_USEMEMCHECKS == 1
// give the free stack space for any task in function result.
uint16_t OS_GetUnusedStack (uint8_t TaskID)
{
   uint16_t unused = 0;
   uint8_t *p = MyOS.StackStart[TaskID]; 

   do
   {
      if (*p++ != OS_UNUSEDMASK)
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

void OS_WaitTicks( uint16_t numTicks ) // Wait for a certain number of OS-ticks (1 = wait to the next timer interrupt)
{
#if OS_USEEXTCHECKS == 1
	if(MyOS.CurrTask == OS_NUMTASKS) return;
#endif
	OS_SetAlarm(MyOS.CurrTask, numTicks);
	OS_WaitAlarm();
}

uint8_t OS_WaitEventTimeout(uint8_t EventMask, uint16_t numTicks ) //returns event on event, 0 on timeout.
{
#if OS_USEEXTCHECKS == 1
	if(MyOS.CurrTask == OS_NUMTASKS) return 0;
#endif
	uint8_t ret;
	OS_SetAlarm(MyOS.CurrTask,numTicks); // set timeout
	ret = OS_WaitEvent(EventMask);
	if(ret | EventMask)
	{
		// event occured
		OS_SetAlarm(1,0); // disable timeout
		return ret;
	}
	else
	{
		// timeout occured
		return 0;
	}
}
#endif
