/*
	FabOS for ATMEL AVR
	tested on WinAVR 20090913 and Mega32
	
	(c) 2009 Fabian Huslik

	This is the file, which implememts automated software testing of the OS modules.
	This file should only be altered if really necessary.
	It does not contain any code, which is useful for production use of FabOS.


remember:

avr-gcc -mmcu=atmega32 -E FabOS_test.c >bla.c  invoke perprocessor


*/

#include "OS/FabOS.h"

#if OS_DO_TESTSUITE == 1 // compile only if tests are activated (OS development use only)

OS_DeclareTask(TestTask0,200);
OS_DeclareTask(TestTask1,200);
OS_DeclareTask(TestTask2,200);

volatile uint16_t r,s,t,u;
volatile uint8_t testvar=0;

OS_DeclareQueue(TestQ,64,1);
OS_DeclareQueue(TestQLong,10,4);

//OS_Queue_t TestQ ={0,0,{}};

uint32_t longQtest[10] = {0x11223344, 0x22334455, 0x33445566, 0x44556677, 0x55667788, 0x66778899, 0x778899aa, 0x8899aabb, 0x99aabbcc, 0xaabbccdd};

#define MAXTESTCASES 9 // one more than the max ID.
uint8_t testcase = 0; // test case number
uint8_t TestResults[MAXTESTCASES]; // test result array (0= OK)
uint8_t TestProcessed[MAXTESTCASES]; // test passed array (number of processed assertions

#define assert(X) do{								\
						OS_ENTERCRITICAL;			\
						if(!X)						\
						{							\
							TestResults[testcase]++;\
						}							\
						TestProcessed[testcase]++;	\
						OS_LEAVECRITICAL;			\
					}while(0)

void WasteOfTime(uint32_t waittime);

void OS_TestSuite(void)
{

	uint8_t test0pass=0;


    OS_CreateTask(TestTask0, 0);
    OS_CreateTask(TestTask1, 1);
    OS_CreateTask(TestTask2, 2);

	OS_StartExecution() ;
	while(1)
	{
		// THIS IS the idle task which will be preemted by all other tasks.
		// NO OS-wait-API allowed here!!!

#if OS_USEMEMCHECKS ==1
		r = OS_GetUnusedStack(0);
		s = OS_GetUnusedStack(1);
		t = OS_GetUnusedStack(2);
		u = OS_GetUnusedStack(3); // idle
#endif

		switch(testcase)
			{
				case 0:
					// Task Priority
					if(!test0pass)
					{
						assert(testvar ==3);
						test0pass = 1;
					}
					break;
				default:
					break;
			}
	}

}

volatile uint32_t testtime=0;


void TestTask1(void)
{
	uint8_t ret,i;
	uint32_t ts,te;
	uint8_t v;
	uint16_t t=25;
	uint32_t ti,tj;

	while(1)
	{
		switch(testcase)
		{
			case 0:
				// Task Priority
				assert(testvar ==1);
				testvar++;
				break;
			case 1:
				OS_WaitTicks(1);
				// Timer / Wait
				OS_GetTicks(&ti); // get time
				OS_WaitTicks(t);
				OS_GetTicks(&tj); // get time
				assert(tj-ti == t); // waited time was correct
									
				break;
			case 2:
				// WaitEvent
				// SetEvent
				// wait for A, then A occurs
				testvar=1;
				OS_WaitEvent(1<<0);
				testvar=2;
				// A occurs, then wait for A
				WasteOfTime(50);
				assert(testvar==3);
				OS_WaitEvent(1<<0);
				testvar=4;

				// wait for more events, one occurs, then the other
				ret = OS_WaitEvent(1<<1 | 1<<2);
				testvar=5;
				assert(ret == (1<<2));
				testvar=6;
				ret = OS_WaitEvent(1<<1 | 1<<2);
				testvar=7;
				assert(ret == (1<<1));
				testvar=8;

				// wait for less events

				break;
			case 3:
				// MutexGet
				// MutexRelease			
				// two cases: 
				// 1:mutex is free
				OS_MutexGet(0);
				WasteOfTime(50);
				testvar = 1;
				OS_WaitTicks(50);
				assert(testvar == 1);
				OS_MutexRelease(0);

				OS_WaitEvent(1<<1);
				// 2:mutex is occupied by lower prio
				OS_WaitTicks(50);
				OS_MutexGet(0);
				testvar = 3;
				WasteOfTime(50);
				assert(testvar == 3);
				OS_MutexRelease(0);

				break;
			case 4:
				// QueueIn
				// QueueOut

				TestQ.read = 13; // tweak the queue to a "used" state
				TestQ.write =13;

				ret = OS_QueueOut(&TestQ,&v); // q empty at start
				assert(ret == 1);
				OS_WaitEvent(1<<6);
				for (i=0;i<20;i++) // forward
				{
					ret = OS_QueueOut(&TestQ,&v);
					assert(v==i);
					assert(ret ==0);
				}
				// Q empty
				ret = OS_QueueOut(&TestQ,&v);
				//assert(v==i);
				assert(ret ==1);
				OS_WaitEvent(1<<6);
				for (i=20;i>0;i--) // backward
				{
					ret = OS_QueueOut(&TestQ,&v);
					assert(v==i);
					assert(ret ==0);
				}
				// Q empty
				ret = OS_QueueOut(&TestQ,&v);
				//assert(v==i);
				assert(ret ==1);
				OS_WaitEvent(1<<6);

				break;
			case 5:
				// SetAlarm
				// WaitAlarm
				OS_GetTicks(&ts);
				OS_WaitAlarm();
				OS_GetTicks(&te);
				assert(te-ts == 53);
				break;
			case 6:
				// Event with timeout
				ret = OS_WaitEventTimeout(1<<3,30);
				assert(ret == 1<<3); // no timeout, it was the event.


				OS_GetTicks(&ti); // get time
				ret = OS_WaitEventTimeout(1<<3,t); // wait for the event, which never comes...
				assert(ret == 0); // timeout recoginzed
				OS_GetTicks(&tj); // get time
				assert(tj-ti == t); // waited time was correct


				break;
			case 7:
				// long queue


				break;
			default:
				break;
		}
	OS_WaitEvent(1<<7);
	}// while(1)

}

void TestTask2(void)
{
	uint32_t ts,te;
	uint32_t i,j;
	uint16_t t;
	while(1)
	{
		switch(testcase)
		{
			case 0:
				// Task Priority
				assert(testvar ==2);
				testvar++;
				break;
			case 1:
				// Timer / Wait
				OS_WaitTicks(2);
				
				t=50;
				OS_GetTicks(&i); // get time
				OS_WaitTicks(t);
				OS_GetTicks(&j); // get time
				assert(j-i == t); // waited time was correct
				break;
			case 2:
				// WaitEvent
				// SetEvent

				break;
			case 3:
				// MutexGet
				// MutexRelease			
				// two cases: 
				// 1:mutex is occupied by higher prio
				WasteOfTime(10);
				OS_MutexGet(0);
				testvar = 7;
				WasteOfTime(50);
				assert(testvar == 7);
				OS_MutexRelease(0);
				OS_SetEvent(1,1<<1);
				// 2:mutex is occupied
				OS_MutexGet(0);
				testvar = 4;
				WasteOfTime(50);
				assert(testvar == 4);
				OS_MutexRelease(0);


				break;
			case 4:
				// QueueIn
				// QueueOut

				break;
			case 5:
				// SetAlarm
				// WaitAlarm
				OS_GetTicks(&ts);
				OS_WaitAlarm();
				OS_GetTicks(&te);
				assert(te-ts == 44);

				break;
			case 6:
				// Event with timeout


				break;
			case 7:
				// long queue


				break;
			default:
				break;
		}
	OS_WaitEvent(1<<7);
	}// while(1)

}

void TestTask0(void)
{
	uint8_t ret,i;
	uint32_t ti,tj;
	uint16_t t;

	while(1)
	{
		switch(testcase)
		{
			case 0:
				// Task Priority
				assert(testvar ==0);
				testvar++;
				break;
			case 1:
				// Timer / Wait
				OS_WaitTicks(3);

				t=10;
				OS_GetTicks(&ti); // get time
				OS_WaitTicks(t);
				OS_GetTicks(&tj); // get time
				assert(tj-ti == t); // waited time was correct
				break;
			case 2:
				// WaitEvent
				// SetEvent
// wait for A, then A occurs
				testvar =0;
				OS_WaitTicks(10);
				assert(testvar==1);
				OS_SetEvent(1,1<<0);
				OS_WaitTicks(2);
				assert(testvar==2);
// A occurs, then wait for A
				testvar =3;
				OS_SetEvent(1,1<<0);
				assert(testvar==3);
				OS_WaitTicks(100); // other task wastes time...
				assert(testvar==4);
// wait for more events, one occurs, then the other
				OS_SetEvent(1,1<<2);
				assert(testvar==4);
				OS_WaitTicks(10);
				assert(testvar==6);
				OS_SetEvent(1,1<<1);
				assert(testvar==6);
				OS_WaitTicks(10);
				assert(testvar==8);

				break;
			case 3:
				// MutexGet
				// MutexRelease			

				break;
			case 4:
				// QueueIn
				// QueueOut
				OS_WaitTicks(50);
				for (i=0;i<20;i++) // forward
				{
					ret = OS_QueueIn(&TestQ,&i);
					assert(ret==0);
				}
				OS_SetEvent(1,1<<6);
				OS_WaitTicks(50);
				for (i=20;i>0;i--) // backward
				{
					ret = OS_QueueIn(&TestQ,&i);
					assert(ret==0);
				}
				OS_SetEvent(1,1<<6);
				OS_WaitTicks(50);
				for (i=0;i<63;i++) // overload the Q (only 63 of 64 usable for indication full/empty)
				{
					ret = OS_QueueIn(&TestQ,&i);
					assert(ret==0);
				}
				// now one too much:
				ret = OS_QueueIn(&TestQ,&i);
				assert(ret==1);

				break;
			case 5:
				// SetAlarm
				// WaitAlarm
				OS_SetAlarm(1,53);
				OS_SetAlarm(2,44);
				break;
			case 6:
				// Event with timeout
				OS_SetEvent(1,1<<3);
				OS_WaitTicks(50);
				// do not set the second one...
				
				break;
			case 7:
				//queue long test todo
				ret = OS_QueueOut(&TestQLong,(uint8_t*)&ti); // Q empty
				assert(ret == 1);
				for (i=0;i<9;i++)
				{
					ret = OS_QueueIn(&TestQLong,(uint8_t*)&longQtest[i]);
					assert (ret==0);
				}
				ret = OS_QueueIn(&TestQLong,(uint8_t*)&longQtest[9]);
				assert (ret==1); // Q full
				for (i=0;i<9;i++)
				{
					ret = OS_QueueOut(&TestQLong,(uint8_t*)&ti);
					assert(ret==0);
					assert(ti == longQtest[i]);
				}
				ret = OS_QueueOut(&TestQLong,(uint8_t*)&ti);
				assert(ret==1); // q empty

				break;
			case 8:
				// Nested Mutexes
				OS_MutexGet(0);
				WasteOfTime(1);
				OS_MutexGet(1);
				WasteOfTime(1);
				OS_MutexRelease(0);
				WasteOfTime(1);
				OS_MutexRelease(1);
				
				break;
			default:
				// The END
				while(1)
				{
					// sit here and wait for someone picking up the results out of "TestResults". 0 = OK. 
					// "TestProcessed" shows the number of processed assertions.
					asm("break");
				}
				break;
		}

		OS_WaitTicks(500); // wait for tests to be processed...

		testcase++;
		OS_SetEvent(1,1<<7);
		OS_SetEvent(2,1<<7);	
	}//while(1)


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



#endif // OS_DO_TESTSUITE == 1

