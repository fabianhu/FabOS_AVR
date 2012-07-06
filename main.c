/*
	FabOS example implementation

	(c) 2008-2012 Fabian Huslik

	Please change this file to your needs.
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

    OS_CreateTask(Task1, 0); // ID 0 with higest priority
    OS_CreateTask(Task2, 1);
    OS_CreateTask(Task3, 2); // ID 2 with lowest prio, but higher than Idle

	OS_CreateAlarm(0, OSALM1);
	OS_CreateAlarm(1, OSALM2);
	OS_CreateAlarm(2, OSALM3);


	OS_StartExecution() ;
	while(1)
	{
		// THIS IS the idle task (ID 3 in this case) which will be preemted by all other tasks.
		// NO OS_Wait.. functions are allowed here!!!
		
		// TODO add your code here
		asm("nop"); //at least one instruction is required for debugging!
	}

}


// *********  Code to be executed inside Timer ISR used for the OS, defined in FabOS_config.h
void OS_CustomISRCode(void)
{
	// TODO add your Timer ISR here
#if defined (__AVR_ATmega32__)
	TCNT1 =0;  // reset the timer on ISR to have correct timing

#elif defined (__AVR_ATxmega32A4__)
	TCC1.CNT=0; // reset the timer on ISR to have correct timing

#elif defined (__AVR_ATmega644P__)
	TCNT1 = 0;
#else
	#error MCU not yet supported, you must configure a timer yourself.
#endif
}


// *********  Controller initialisation
void CPU_init(void)
{
	// init OS timer and interrupt
	//Timer0 Initializations for ATMEGA16
	//TCCR0 |= 5;  // Enable TMR0, set clock source to CLKIO/1024. Interrupts @ 32.768ms intervals @ 8 MHz. This means tasks can execute at least 130,000 instructions before being preempted.
	//TIMSK |= 1 ; // Interrupt on TMR0 Overflow.
#if defined (__AVR_ATmega32__) 

	TCCR1A = 0b00000000;
	TCCR1B = 0b00000011; //250kHZ timer ck
	OCR1A  = 250; //interrupt every 1ms
	TIMSK |= 1<<OCIE1A; // Output Compare Interrupt ON

#elif defined (__AVR_ATmega644P__)

	// init cyclic ISR
	TCCR1A = 0b00000000; 
	TCCR1B = 0b00000010; //1250 kHZ timer ck
	OCR1A  = 12500; //interrupt every 10ms at 10MHz

	TIMSK1 |= 1<<OCIE1A;

	
#elif defined (__AVR_ATxmega32A4__)
	// set ck = 32MHz,
	// PLL (128 MHz) -> peripheral x4
	// Presc. B (64MHz) -> peripheral x2
	// Presc. C (32MHz) -> CPU

#if USEEXTERNALOSC == 1
	OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
	OSC.CTRL |= OSC_XOSCEN_bm; // enable XTAL
	// ! PLL MUST run at least 10MHz
	OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 8; // configure pll x 8; // fixme check if correct!!
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
#else
	CCP = CCP_IOREG_gc; // unlock
	OSC_XOSCFAIL = OSC_XOSCFDEN_bm; // enable NMI for oscillator failure.

	// Desired Clock : 16MHz,
	// PLL (16 MHz) -> peripheral x1
	// Presc. B (16MHz) -> peripheral x1
	// Presc. C (16MHz) -> CPU
	//OSC_XOSCCTRL = 0;
	OSC_CTRL = OSC_RC2MEN_bm; // enable XTAL
	OSC_PLLCTRL = OSC_PLLSRC_RC2M_gc | 8; // configure pll x 8; (min 10MHz!)
	while (!(OSC_STATUS & OSC_RC2MRDY_bm))
	{
		asm("nop"); // wait for the bit to become set
	}
	OSC_DFLLCTRL = OSC_RC2MCREF_bm; // enable auto calib.
	DFLLRC2M_CTRL = DFLL_ENABLE_bm;

	OSC_CTRL |= OSC_PLLEN_bm; // enable PLL

	CCP = CCP_IOREG_gc; // unlock
	CLK_PSCTRL = CLK_PSADIV_1_gc|CLK_PSBCDIV_1_1_gc;
	while (!(OSC_STATUS & OSC_PLLRDY_bm))
	{
		asm("nop"); // wait for the bit to become set
	}

#endif
	 
	CCP = CCP_IOREG_gc; // unlock
	CLK.CTRL = CLK_SCLKSEL_PLL_gc; // select PLL to run with
 
	// setup Timer for OS
	TCC1.CTRLA = TC_CLKSEL_DIV1_gc; // select clk/1 for clock source
	TCC1.CTRLB = TC0_CCAEN_bm;
	TCC1.CTRLC = 0;
	TCC1.CTRLD = 0;
	TCC1.CTRLE = 0;
	TCC1.INTCTRLA = 0;
	TCC1.INTCTRLB = TC_CCAINTLVL_HI_gc; // enable compare match A as HIGH level interrupt. 
	TCC1.CCA = 32000;// compare at 32000 gives 1ms clock

	//Enable Interrupts in INT CTRL
	PMIC.CTRL = PMIC_HILVLEN_bm|PMIC_MEDLVLEN_bm|PMIC_LOLVLEN_bm;

#else
	#error MCU not yet supported, you must do the CPU init yourself.
#endif

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
		case 4:
			// OS_WaitAlarm: waiting in idle is not allowed
			break;
		case 5:
			// OS_MutexGet: invalid Mutex number
			break;
		case 6:
			// OS_MutexRelease: invalid Mutex number
			break;
		case 7:
			// OS_Alarm misconfiguration / ID bigger than array size
			break;
		case 8:
			// OS_WaitAlarm: Alarm was not active
			break;
		case 9:
			// OS_WaitAlarm: Alarm is not assigned to the task (critical!)
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

