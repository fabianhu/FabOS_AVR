#include "FabOS.h"

#ifdef TESTSUITE

void TestTask1(void);
void TestTask2(void);
void TestTask3(void);

uint8_t TestTask1Stack[200] ;
uint8_t TestTask2Stack[200] ;
uint8_t TestTask3Stack[200] ;

uint16_t r,s,t;

#define MAXTESTCASES 12
uint8_t testcase; // test case number
uint8_t TestResults[MAXTESTCASES]; // result array
#define assert(X) if(!X)TestResults[testcase]++

void WasteOfTime(uint32_t waittime);

void testsuite(void)
{

    OS_TaskCreate(0, TestTask1, TestTask1Stack);
    OS_TaskCreate(1, TestTask2, TestTask2Stack);
    OS_TaskCreate(2, TestTask3, TestTask3Stack);

	OS_StartExecution() ;
	while(1)
	{
		// THIS IS the idle task which will be preemted by all other tasks.
		// NO OS-wait-API allowed here!!!

#if USECHECKS ==1
		r = OS_get_unused_Stack (TestTask1Stack);
		s = OS_get_unused_Stack (TestTask2Stack);
		t = OS_get_unused_Stack (TestTask3Stack);
#endif

	}

}

volatile uint8_t testvar=0;


void TestTask1(void)
{
	switch(testcase)
	{
		case 1:
			OS_WaitTicks(1);
			// Timer / Wait
			uint16_t t=100;
			uint32_t i,j;

			OS_GetTicks(&i); // get time
			OS_WaitTicks(t);
			OS_GetTicks(&j); // get time
			assert(j-i == t);
									
			break;
		case 2:
			// WaitEvent
			OS_WaitEvent(1<<2);
			assert(testvar == 1);
			OS_WaitTicks(100);

			break;
		case 3:
			// SetEvent

			break;
		case 4:
			// MutexGet
			// two cases: 
			// 1:mutex is free
			OS_mutexGet(0);
			WasteOfTime(50);
			testvar = 1;
			WasteOfTime(50);
			assert(testvar == 1);
			OS_mutexRelease(0);

			// 2:mutex is occupied

			break;
		case 5:
			// MutexRelease			

			break;
		case 6:
			// SetEvent

			break;
		case 7:
			// QueueIn

			break;
		case 8:
			// QueueOut

			break;
		case 9:
			// SetAlarm

			break;
		case 10:
			// WaitAlarm

			break;
		case 11:
			// GetUnusedStack

			break;
		default:
			break;
	}

}

void TestTask2(void)
{
	switch(testcase)
	{
		case 1:
			// Timer / Wait
			OS_WaitTicks(2);
			uint32_t i,j;
			uint16_t t=250;

			OS_GetTicks(&i); // get time
			OS_WaitTicks(t);
			OS_GetTicks(&j); // get time
			assert(j-i == t);
		case 2:
			// WaitEvent
			OS_WaitEvent(1<<2);
			assert(testvar == 1);
			OS_WaitTicks(100);

			break;
		case 3:
			// SetEvent

			break;
		case 4:
			// MutexGet

			break;
		case 5:
			// MutexRelease			

			break;
		case 6:
			// SetEvent

			break;
		case 7:
			// QueueIn

			break;
		case 8:
			// QueueOut

			break;
		case 9:
			// SetAlarm

			break;
		case 10:
			// WaitAlarm

			break;
		case 11:
			// GetUnusedStack

			break;
		default:
			break;
	}


}

void TestTask3(void)
{
	switch(testcase)
	{
		case 1:
			// Timer / Wait
			OS_WaitTicks(3);
			uint32_t i,j;
			uint16_t t=500;

			OS_GetTicks(&i); // get time
			OS_WaitTicks(t);
			OS_GetTicks(&j); // get time
			assert(j-i == t);
			testcase++;
		case 2:
			// WaitEvent
			testvar =2;
			OS_SetEvent(1<<2,1);
			OS_WaitTicks(10);
			testvar =1;
			OS_SetEvent(1<<2,0);
			OS_WaitTicks(100);
			break;
		case 3:
			// SetEvent

			break;
		case 4:
			// MutexGet

			break;
		case 5:
			// MutexRelease			

			break;
		case 6:
			// SetEvent

			break;

		case 7:
			// QueueIn

			break;
		case 8:
			// QueueOut

			break;
		case 9:
			// SetAlarm

			break;
		case 10:
			// WaitAlarm

			break;
		case 11:
			// GetUnusedStack

			break;
		default:
			break;
	}



}

void WasteOfTime(uint32_t waittime)
{
	uint32_t oldTime,newTime;
	OS_GetTicks(&oldTime); // get time
	do
	{
		OS_GetTicks(&newTime); // get time
	}
	while (newTime - oldTime < waittime);
}



#endif // Testsuite
