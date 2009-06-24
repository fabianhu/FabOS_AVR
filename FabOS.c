#include "FabOS.h"

FabOS_t MyOS; // the global instance of the struct


// the idle task does not have a stack-array defined. Instead it uses the unused "ordinary" stack at the end of RAM.


/*
implement the following:
- signal (wait for) including timeout
- cyclic alarm OK (more or less)
- mutex OK
- messages / queues

*/


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
ISR  (OS_ScheduleISR) //__attribute__ ((naked,signal)) // 10 ms isr
{
	saveCPUContext() ; 
	MyOS.Stacks[MyOS.currTask] = SP ; // catch the SP before we (possibly) do anything with it.

	OS_CustomISRCode(); // Caller of Custom ISR code to be executed inside this ISR

	uint8_t i;

	for(i=0; i < NUMTASKS; i++ ) 
	{ 
		if( MyOS.TickBlock[i] > 0 ) // this task has to wait
		{
			if( --MyOS.TickBlock[i] == 0 ) // if now task is ready, it will be activated.
			{
				MyOS.TaskReadyBits |= 1<<(i) ; // now it is finished with waiting
			}
		}
	}

	// task is to be run
	MyOS.currTask = OS_GetNextTaskNumber() ;
	SP = MyOS.Stacks[MyOS.currTask] ;
	restoreCPUContext() ;
	asm volatile("reti"); 
}



void OS_TaskCreateInt( uint8_t taskNum, void (*t)(), uint8_t *stack, uint8_t stackSize ) 
{
	uint16_t z ;
	// "colorize" the stacks
	for (z=0;z<stackSize;z++)
	{
		stack[z] = UNUSEDMASK;
	}

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
	register uint8_t temp ;

	MyOS.TaskReadyBits &= ~(1<<taskNum) ;

	for( temp = 0 ; temp < NUMMBOX ; temp++ ) {
	  if( MyOS.mBoxWait[temp] == taskNum ) MyOS.mBoxWait[temp] = 0 ;
	}
	MyOS.TickBlock[taskNum] = 0 ;

}


void OS_Reschedule(void)
{
	saveCPUContext() ; 
	MyOS.Stacks[MyOS.currTask] = SP ; // catch the SP before we (possibly) do anything with it.

	// task is to be run
	MyOS.currTask = OS_GetNextTaskNumber() ;// MyOS.taskResolutionTable[MyOS.TaskReadyBits]; //
	SP = MyOS.Stacks[MyOS.currTask] ;// set Stack pointer
	restoreCPUContext() ;
	asm volatile("reti"); 
}

int8_t OS_GetNextTaskNumber() // which is the next task (ready and highest (= rightmost) prio)?
{
	uint8_t i,j;

	j= MyOS.TaskReadyBits; // make working copy
	
	for (i=0;i<NUMTASKS;i++)
	{
		if (j & 0x01) // last bit set
			return i; // this task is the one to be executed
		else
			j= (j>>1);
	}
	// NO task is ready, which one to execute??
	return NUMTASKS; // the idle task !!
}


void OS_StartExecution() // naked!!
{
	//store THIS context for idling!!
	saveCPUContext() ;
	MyOS.Stacks[NUMTASKS] = SP ; // catch the SP

	MyOS.currTask = OS_GetNextTaskNumber(); // start with next task
	SP = MyOS.Stacks[MyOS.currTask] ;
	restoreCPUContext();
	asm volatile( "reti" ) ;
}


// *************** MBOX


void OS_mBoxPost( int8_t _mBoxNum, int16_t _data )
{
	cli();
	MyOS.mBoxData[_mBoxNum] = _data ;   // Put the data in the mailbox.

	if( (MyOS.mBoxStat[_mBoxNum] & 0x01) != 0 )
	{  // If a task is already waiting on this mailbox.
		MyOS.TaskReadyBits |= 1<<MyOS.mBoxWait[_mBoxNum] ;   // Make the task ready to run again.
		MyOS.mBoxStat[_mBoxNum] &= ~0b00000011 ;   // Clear the valid data bit and the task waiting bit
		OS_Reschedule() ; // re-schedule; will wake up the sleeper directly, if higher prio.
	} 
	else 
	{ // If no task is waiting for data, put the data in the box and return.
		MyOS.mBoxStat[_mBoxNum] |= 0x02 ;  // Indicate that the mailbox contains valid data
	}
	sei();
	// return with no other action!
}


int16_t OS_mBoxPend( int8_t _mBoxNum )
{
	cli();
	if( (MyOS.mBoxStat[_mBoxNum] & 0x02) != 0 )
	{  // if valid data is present in the mailbox
		MyOS.mBoxStat[_mBoxNum] &= ~0b00000011 ;   // Clear the valid data bit and the task waiting bit
		sei(); // no re-schedule, as there was data in the MB	
	}
	else
	{
		MyOS.mBoxStat[_mBoxNum] |= 0x01 ;   // Indicate that a task is waiting for the mailbox
		MyOS.mBoxWait[_mBoxNum] = MyOS.currTask ;         // tell which task is waiting for the mailbox.
		MyOS.TaskReadyBits &= ~(1<<MyOS.currTask) ;     // indicate that this task is not ready to run.
		
		OS_Reschedule() ; // re-schedule; will be waked up by "post"
	}
		return MyOS.mBoxData[_mBoxNum] ;
} 


// ************************** MUTEX



void OS_mutexTake(int8_t mutexNum) // number of mutexes limited to 8 !!!
{
	cli(); // critical section; prevent timer isr during the read-modify-write operation
	while( MyOS.mutexStat & (1<<mutexNum))
	{
		sei();
		OS_Wait(1) ;
		cli();
	}
	MyOS.mutexStat |= 1<<mutexNum ;   // Set the stat bit in the mutex status byte.
	sei();
}



void OS_mutexRelease(int8_t mutexNum)
{
	cli(); // critical section; prevent timer isr during the read-modify-write operation
	MyOS.mutexStat &= ~(1<<mutexNum) ;
	OS_Reschedule() ; // re-schedule; will wake up waiting task, if higher prio.
}




// ************************** Waiting

void OS_Wait( uint16_t numTicks ) // Wait for a certain number of OS-ticks
{
	OS_SetAlarm(MyOS.currTask, numTicks);
	OS_WaitAlarm();
}

void OS_SetAlarm(uint8_t TaskID, uint16_t numTicks ) // set Alarm for the future and continue // set alarm to 0 disable it.
// fixme maybe do a direct activation of other task, if 0 ???
{
	cli(); // critical section; re-enabled by reti in OS_Schedule()
	MyOS.TickBlock[TaskID] = numTicks ;
	sei();
}

void OS_WaitAlarm(void) // Wait for an Alarm set by OS_SetAlarm
{
	cli(); // critical section; re-enabled by reti in OS_Schedule()
	if(MyOS.TickBlock[MyOS.currTask] > 0)
	{
		MyOS.TaskReadyBits &= ~(1<<MyOS.currTask) ;  // Disable the task
		OS_Reschedule();  // re-schedule; let the others run...
	}
	else
	sei();
}


// *********************** Buffers
// fixme not yep part of OS !!!

#define BUFFER_SIZE 64 // must be 2^n (8, 16, 32, 64 ...)
#define BUFFER_MASK (BUFFER_SIZE-1) // don't forget the braces
 
struct Buffer {
  uint8_t read; // field with oldest content
  uint8_t write; // always empty field
  uint8_t data[BUFFER_SIZE];
} buffer = {0, 0, {}};
 
uint8_t BufferIn(uint8_t byte)
{
  uint8_t next = ((buffer.write + 1) & BUFFER_MASK);
  if (buffer.read == next)
    return 1; // buffer full
  buffer.data[buffer.write] = byte;
  // buffer.data[buffer.write & BUFFER_MASK] = byte; // absolut Sicher
  buffer.write = next;
  return 0;
}
 
uint8_t BufferOut(uint8_t *pByte)
{
  if (buffer.read == buffer.write)
    return 1; // buffer empty
  *pByte = buffer.data[buffer.read];
  buffer.read = (buffer.read+1) & BUFFER_MASK;
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


