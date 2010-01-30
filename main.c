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
#if defined (__AVR_ATmega32__)
	TCNT1 =0;  // reset the timer on ISR to have correct timing

#elif defined (__AVR_ATxmega32A4__)
	TCC1.CNT=0;
#else
	#error MCU not yet supported, you must configure a timer yourself.
#endif
}


// *********  Controller initialisation
void CPU_init(void)
{
	// init OS timer and interrupt
#if defined (__AVR_ATmega32__)

	TCCR1A = 0b00000000;
	TCCR1B = 0b00000011; //250kHZ timer ck
	OCR1A  = 250; //interrupt every 1ms
	TIMSK |= 1<<OCIE1A; // Output Compare Interrupt ON

#elif defined (__AVR_ATxmega32A4__)
	// set ck = 32MHz,
	// PLL (128 MHz) -> peripheral x4
	// Presc. B (64MHz) -> peripheral x2
	// Presc. C (32MHz) -> CPU
	OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
	OSC.CTRL |= OSC_XOSCEN_bm; // enable XTAL
	OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 4; // configure pll x 4;
	while (!(OSC.STATUS & OSC_XOSCRDY_bm))
	{
		asm("nop"); // wait for the bit to become set
	}
	OSC.CTRL |= OSC_PLLEN_bm; // enable PLL

	CCP = CCP_IOREG_gc; // unlock
	CLK.PSCTRL = CLK_PSADIV_1_gc|CLK_PSBCDIV_2_2_gc; 
	while (!(OSC.STATUS & OSC_PLLRDY_bm))
	{
		asm("nop"); // wait for the bit to become set
	}
	 
	CCP = CCP_IOREG_gc; // unlock
	CLK.CTRL = CLK_SCLKSEL_PLL_gc; // select PLL to run with
 
	// setup Timer for OS
	TCC1.CTRLA = TC_CLKSEL_DIV1_gc; // select clk/1 for clock source
	TCC1.CTRLB = TC0_CCAEN_bm;
	TCC1.CTRLC = 0;
	TCC1.CTRLD = 0;
	TCC1.CTRLE = 0;
	TCC1.INTCTRLA = 0;
	TCC1.INTCTRLB = TC_CCAINTLVL_HI_gc; // enable compare match A
	TCC1.CCA = 32000;// compare at 32000 gives 1ms clock

	//Enable Interrupts in INT CTRL
	PMIC.CTRL = PMIC_HILVLEN_bm|PMIC_MEDLVLEN_bm|PMIC_LOLVLEN_bm;

#else
	#error MCU not yet supported, you must configure a timer yourself.
#endif


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

