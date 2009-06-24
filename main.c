#include "FabOS.h"


void Task1(void);
void Task2(void);
void Task3(void);
void MCU_init(void);

uint8_t Task1Stack[200] ;
uint8_t Task2Stack[200] ;
uint8_t Task3Stack[200] ;

uint16_t r,s,t;

int main(void)
{
    OS_TaskCreate(0, Task1, Task1Stack);
    OS_TaskCreate(1, Task2, Task2Stack);
    OS_TaskCreate(2, Task3, Task3Stack);

	MCU_init();

	OS_StartExecution() ;
	while(1)
	{
		// THIS is the idle task which will be preemted
		// NO OS-wait-API allowed here!!!
		r = OS_get_unused_Stack (Task1Stack);
		s = OS_get_unused_Stack (Task2Stack);
		t = OS_get_unused_Stack (Task3Stack);


	}

}

void OS_CustomISRCode(void)
{
	// tick the RT-clock...
	MyOS.OSTicks++;
}

void MCU_init(void)
{
	// init timer
	TCCR1A = 0b00000000;
	TCCR1B = 0b00000011; //250kHZ timer ck
	OCR1A  = 2500; //interrupt every 10ms
	TIMSK |= 1<<OCIE1A;

	
	//Timer0 Initializations for ATMEGA16
	//TCCR0 |= 5;  // Enable TMR0, set clock source to CLKIO/1024. Interrupts @ 32.768ms intervals @ 8 MHz. This means tasks can execute at least 130,000 instructions before being preempted.
	//TIMSK |= 1 ; // Interrupt on TMR0 Overflow.

}


