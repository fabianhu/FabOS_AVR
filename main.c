/*
	FabOS example implementation

	(c) 2009 Fabian Huslik

*/

#include "OS/FabOS.h"

// *********  Task definitions
OS_DeclareTask(Task1,200);
OS_DeclareTask(Task2,200);
OS_DeclareTask(Task3,200);

OS_DeclareQueue(DemoQ,10,4);

// *********  Prototypes
void CPU_init(void);


// *********  THE main()
int main(void)
{
	CPU_init();

#if OS_DO_TESTSUITE == 1
	OS_TestSuite(); // call automated tests of OS. may be removed in production code.
#endif

    OS_CreateTask(Task1, 0);
    OS_CreateTask(Task2, 1);
    OS_CreateTask(Task3, 2);

	OS_StartExecution() ;
	while(1)
	{
		// THIS IS the idle task which will be preemted by all other tasks.
		// NO OS_Wait.. functions are allowed here!!!
		
		// TODO add your code here
		asm("nop"); //at least one instruction is required!!!
	}

}


// *********  Code to be executed inside Timer ISR used for the OS, defined in FabOS_config.h
void OS_CustomISRCode(void)
{
	// TODO add your Timer ISR here
	TCNT1 =0;  // reset the timer on ISR to have correct timing
}


// *********  Controller initialisation
void CPU_init(void)
{
	// init OS timer and interrupt
	TCCR1A = 0b00000000;
	TCCR1B = 0b00000011; //250kHZ timer ck
	OCR1A  = 250; //interrupt every 1ms
	TIMSK |= 1<<OCIE1A; // Output Compare Interrupt ON

	
	//Timer0 Initializations for ATMEGA16
	//TCCR0 |= 5;  // Enable TMR0, set clock source to CLKIO/1024. Interrupts @ 32.768ms intervals @ 8 MHz. This means tasks can execute at least 130,000 instructions before being preempted.
	//TIMSK |= 1 ; // Interrupt on TMR0 Overflow.

	// *** NO global interrupts enabled at this point!!!
}


#if OS_USEEXTCHECKS == 1
void OS_ErrorHook(uint8_t ErrNo)
{
	static uint8_t dummy =0;
	
	switch(ErrNo)
	{
		case 2:
			// OS_WaitEvent: waiting in idle is not allowed
			break;	
		case 3:
			// OS_SetAlarm: Multiple alarm per task
			break;	
		case 4:
			// OS_WaitAlarm: waiting in idle is not allowed
			break;	
		default:
			break;	
	}
	
	dummy = ErrNo; // dummy code
	#if OS_DO_TESTSUITE == 1
	asm("break"); // for automated tests of OS. may be removed in production code.
	#endif
}
#endif

