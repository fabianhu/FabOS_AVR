#include "FabOS.h"
#ifdef TESTSUITE
extern OS_Queue_t Qtest;

void Task2() 
{

	uint32_t i;
	uint32_t j,k,l,m;
	uint8_t r,e;

	while(1)
	{
		OS_WaitAlarm();
		
		j = 4; 
		k = 5;
		l = 6;
			
		OS_mutexGet(0);
		for(i=0;i<222;i++)
		{
			m= j*k+l;
			l= 7*j;
			if (m % 4) j ++;
			
			

			asm("nop"); // waste of time
		}
		OS_mutexRelease(0);		
		
		OS_Wait(20);


		e = OS_QueueOut(&Qtest, &r); // Get a byte out of the queue, return 1 if q empty.

		e++;

	}

}

#endif
