/*
	FabOS example implementation

	(c) 2009 Fabian Huslik

*/

#include "OS/FabOS.h"

// *********  Prototypes
void Task1(void);
void Task2(void);
void Task3(void);
void CPU_init(void);


// *********  Global variables
uint8_t Task1Stack[200] ;
uint8_t Task2Stack[200] ;
uint8_t Task3Stack[200] ;

// also the queues should go here...


// *********  THE main()
int main(void)
{
	CPU_init();

#if OS_DO_TESTSUITE == 1
	OS_TestSuite(); // call automated tests of OS.
#endif

    OS_TaskCreate(0, Task1, Task1Stack);
    OS_TaskCreate(1, Task2, Task2Stack);
    OS_TaskCreate(2, Task3, Task3Stack);

	OS_StartExecution() ;
	while(1)
	{
		// THIS IS the idle task which will be preemted by all other tasks.
		// NO OS_Wait.. functions are allowed here!!!
		
		// TODO add your code here
	}

}


// *********  Code to be executed inside Timer ISR used for the OS, defined in FabOS_config.h
void OS_CustomISRCode(void)
{
	// TODO add your Timer ISR here
	TCNT1 =0;  // reset the timer on ISR
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


