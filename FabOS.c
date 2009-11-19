#include "FabOS.h"

FabOS_t MyOS; // the global instance of the struct


// the idle task does not have a stack-array defined. Instead it uses the unused "ordinary" stack at the end of RAM.


// -mtiny-stack ??
// 
// register unsigned char counter asm("r3");  Typically, it should be safe to use r2 through r7 that way.

// -Os -mcall-prologues


// **************************** TIMER

// ISR(TIMER0_OVF_vect) __attribute__ ((naked,signal));
// Prototype for the INT0 ISR is in FabOS.h. The naked attribute
// tells the compiler not to add code to push the registers
// it uses onto the stack or even add a RETI instruction
// at the end. It just compiles the code inside the braces.
// No use of stack space inside this function!
// except embedding it into a function, as this creates a stack frame.
//	register uint8_t taskID asm("r3"); // have to use a register (r2..7) here. we would otherwise damage the stack
ISR  (OS_ScheduleISR) //__attribute__ ((naked,signal)) // Timer isr
{

	saveCPUContext() ; 
	MyOS.Stacks[MyOS.CurrTask] = SP ; // catch the SP before we (possibly) do anything with it.

#if OS_USECLOCK == 1
	// tick the RT-clock...
	MyOS.OSTicks++; 
#endif
	
	OS_CustomISRCode(); // Caller of Custom ISR code to be executed inside this ISR on the last tasks stack

	ProcessAlarms(); // function uses stack

	// task is to be run
	MyOS.CurrTask = OS_GetNextTaskNumber() ;
	SP = MyOS.Stacks[MyOS.CurrTask] ;
	restoreCPUContext() ;
	asm volatile("reti"); 
}

// ************************** OS_INTERNALS


void ProcessAlarms(void)
{
	uint8_t taskID;
	// handling of OS_Wait / Alarms
	for(taskID=0; taskID < NUMTASKS; taskID++ ) 
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
	cli();
	saveCPUContext() ; 
	MyOS.Stacks[MyOS.CurrTask] = SP ; // catch the SP before we (possibly) do anything with it.

	// task is to be run
	MyOS.CurrTask = OS_GetNextTaskNumber();

	SP = MyOS.Stacks[MyOS.CurrTask] ;// set Stack pointer
	restoreCPUContext() ;
	asm volatile("reti"); 
}

int8_t OS_GetNextTaskNumber() // which is the next task (ready and highest (= rightmost) prio)?
{
	uint8_t Task,ReadyMask,next= NUMTASKS; // NO task is ready, which one to execute?? the idle task !!;

	ReadyMask= MyOS.TaskReadyBits; // make working copy
	
	for (Task=0;Task<NUMTASKS;Task++)
	{
		if (ReadyMask & 0x01) // last bit set
		{	
			next =  Task; // this task is the one to be executed
			break;
		}
		else
		{
			ReadyMask= (ReadyMask>>1); // shift to right; i and the shift is synchronous.
		}
	}
	// now "next" is the next highest prio task.

	// look in mutex waiting list, if any task is blocking "next", then "next" must not run.
	if (MyOS.MutexTaskWaiting[next] != 0xff) // "next" is waiting for mutex
	{
		// which task is blocking it?
		next = MyOS.MutexOwnedByTask[MyOS.MutexTaskWaiting[next]]; 
		// the blocker gets the run.
		// this is also a priority inversion.
	}
	return next;
}

// internal task create function
void OS_TaskCreateInt( uint8_t taskNum, void (*t)(), uint8_t *stack, uint8_t stackSize ) 
{
	uint16_t z ;
#if OS_USECHECKS == 1
	// "colorize" the stacks
	for (z=0;z<stackSize;z++)
	{
		stack[z] = UNUSEDMASK;
	}
#endif

	for (z=stackSize-35;z<stackSize;z++) // clear the stack frame space (should already be, but you never know...)
	{
		stack[z] = 0;
	}
		
	MyOS.TaskReadyBits |= 1<<taskNum ;  // indicate that the task exists and is ready to run.

	MyOS.Stacks[taskNum] = (uint16_t)stack + stackSize - 1 ; // Point the task's SP to the top address of the array that represents its stack.

	*(uint8_t*)(MyOS.Stacks[taskNum]-1) = ((uint16_t)(t)) >> 8; // Put the address of the function that implements the task on its stack
	*(uint8_t*)(MyOS.Stacks[taskNum]) = ((uint16_t)(t)) & 0x00ff;

	MyOS.Stacks[taskNum] -= 35;   // Create a stack frame with placeholders for all the user registers as well as the SR.
}



// Takes the task out of the list of tasks ready to run.
// If the task is waiting on a mailbox or for a number of clock ticks
// to pass, it is taken out of the mailbox wait list and the number of
// clock ticks it has to wait for to be rescheduled is set to zero.
void OS_TaskDestroy( int8_t taskNum ) 
{
	MyOS.TaskReadyBits &= ~(1<<taskNum) ;

	MyOS.EventMask[taskNum]=0;
	MyOS.EventWaiting[taskNum]=0;

	MyOS.AlarmTicks[taskNum] = 0 ;
}


// Start the operating system
void OS_StartExecution() // __attribute__ ((naked));
{
	
	uint8_t i;
	for(i=0; i < NUMTASKS+1; i++ ) // init mutexes
	{ 
		MyOS.MutexTaskWaiting[i] = 0xff;
	}
	for(i=0; i < NUMMUTEX; i++ ) 
	{ 
		MyOS.MutexOwnedByTask[i] = 0xff;
	}

	//store THIS context for idling!!
	MyOS.CurrTask = NUMTASKS;
	OS_Reschedule();
}


// ************************** MUTEX

// Try to get a mutex; execution will block as long the mutex is occupied. If it is free, it is occupied afterwards.
void OS_mutexGet(int8_t mutexID)
{
	cli(); // critical section; prevent timer isr during the read-modify-write operation
	while( MyOS.MutexOwnedByTask[mutexID] != 0xff) // as long as anyone is the owner..
	{
		MyOS.MutexTaskWaiting[MyOS.CurrTask] = mutexID; // set waiting info for priority inversion of scheduler
		OS_Reschedule(); // also re-enables interrupts...
		cli();
	}
	MyOS.MutexOwnedByTask[mutexID] = MyOS.CurrTask; // tell others, that I am the owner.
	MyOS.MutexTaskWaiting[MyOS.CurrTask] = 0xff; // remove waiting info
	sei();
}

// release the occupied mutex
void OS_mutexRelease(int8_t mutexID)
{
	cli(); // critical section; prevent timer isr during the read-modify-write operation
	MyOS.MutexOwnedByTask[mutexID] = 0xff; // tell others, that no one is the owner.
	OS_Reschedule() ; // re-schedule; will wake up waiting task, if higher prio.
}

// ************************** EVENTS

void OS_SetEvent(uint8_t EventMask, uint8_t TaskID) // Set one or more events
{
	cli();
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
		sei();
	}
}

uint8_t OS_WaitEvent(uint8_t EventMask) //returns event(s), which lead to execution
{
	uint8_t ret;
	cli();
	
	if(!(EventMask & MyOS.EventMask[MyOS.CurrTask])) // This task is Not having one of these events active
	{
		MyOS.EventWaiting[MyOS.CurrTask] = EventMask; // remember what this task is waiting for
		// no event yet... waiting
		MyOS.TaskReadyBits &= ~(1<<MyOS.CurrTask) ;     // indicate that this task is not ready to run.

		OS_Reschedule() ; // re-schedule; will be waked up here by "SetEvent" or alarm

		MyOS.EventWaiting[MyOS.CurrTask] = 0; // no more waiting!
		cli();
	}
	ret = MyOS.EventMask[MyOS.CurrTask] & EventMask;
	// clear the events:
	MyOS.EventMask[MyOS.CurrTask] &= ~EventMask; // the actual events minus the ones, which have been waited for 
	sei();
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

void OS_WaitTicks( uint16_t numTicks ) // Wait for a certain number of OS-ticks (1 = wait to the next timer interrupt)
{
	OS_SetAlarm(MyOS.CurrTask, numTicks);
	OS_WaitAlarm();
}

void OS_SetAlarm(uint8_t TaskID, uint16_t numTicks ) // set Alarm for the future and continue // set alarm to 0 disable it.
{
	cli(); // critical section;
	MyOS.AlarmTicks[TaskID] = numTicks ;
	sei();
}

void OS_WaitAlarm(void) // Wait for an Alarm set by OS_SetAlarm 
{
	cli(); // critical section; re-enabled by reti in OS_Schedule()
	if(MyOS.AlarmTicks[MyOS.CurrTask] > 0) // fixme this "if" could be possibly omitted.
	{
		MyOS.TaskReadyBits &= ~(1<<MyOS.CurrTask) ;  // Disable this task
		OS_Reschedule();  // re-schedule; let the others run...
	}
	else
	{
		sei(); // just continue
	}
}

// ************************** QUEUES

uint8_t OS_QueueIn(OS_Queue_t* pQueue , uint8_t byte) // Put byte into queue, return 1 if q full.
{
	uint8_t next;
	cli(); // critical section;
	next = ((pQueue->write + 1) & BUFFER_MASK);
	if (pQueue->read == next)
	{
		sei();
		return 1; // buffer full
	}
	pQueue->data[pQueue->write] = byte;
	// buffer.data[buffer.write & BUFFER_MASK] = byte; // absolute secure
	pQueue->write = next;
	sei();
	return 0;
}
 
uint8_t OS_QueueOut(OS_Queue_t* pQueue, uint8_t *pByte) // Get a byte out of the queue, return 1 if q empty.
{
	cli(); // critical section;
	if (pQueue->read == pQueue->write)
	{
		sei();
		return 1; // buffer empty
	}
	*pByte = pQueue->data[pQueue->read];
	pQueue->read = (pQueue->read+1) & BUFFER_MASK;
	sei();
	return 0;
}


/* alternatively

#define BUFFER_SIZE 23
 
struct Buffer {
  uint8_t data[BUFFER_SIZE];
  uint8_t read; // zeigt auf das Feld mit dem ältesten Inhalt
  uint8_t write; // zeigt immer auf leeres Feld
} buffer = {{}, 0, 0};
 
uint8_t BufferIn(uint8_t byte)
{
  //if (write >= BUFFER_SIZE)
  //  write = 0; // erhöht sicherheit
 
  if (buffer.write + 1 == buffer.read || buffer.read == 0 && buffer.write + 1 == BUFFER_SIZE)
    return FAIL; // voll
 
  buffer.data[buffer.write] = byte;
 
  buffer.write = buffer.write + 1;
  if (buffer.write >= BUFFER_SIZE)
    buffer.write = 0;
}
 
uint8_t BufferOut(uint8_t *pByte)
{
  if (buffer.read == buffer.write)
    return FAIL;
  *pByte = buffer.data[buffer.read];
 
  buffer.read = buffer.read + 1;
  if (buffer.read >= BUFFER_SIZE)
    buffer.read = 0;
  return SUCCESS;
}


*/


// *********************** Aux functions


#if OS_USECHECKS == 1
// give the free stack space for any task in function result.
uint16_t OS_get_unused_Stack (uint8_t* pStack)
{
   uint16_t unused = 0;
   uint8_t *p = pStack;

   do
   {
      if (*p++ != UNUSEDMASK)
         break;

      unused++;
   } while (p <= (uint8_t*) RAMEND);

      return unused;
}
#endif

#if OS_USECLOCK == 1
// fills given variable with the OS ticks since start.
void OS_GetTicks(uint32_t* pTime) 
{
	cli();
		*pTime = MyOS.OSTicks;
	sei();
}
#endif
