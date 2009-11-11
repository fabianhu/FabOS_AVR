#include "FabOS.h"
uint16_t ret;

OS_Queue_t Qtest = {0, 0, {}};

void Task1() 
{
	uint32_t i;
	uint8_t z = 0;

		OS_SetAlarm(MyOS.currTask,1);
	
	while(1)
	{
		OS_WaitAlarm();
		OS_SetAlarm(MyOS.currTask,10);
	
		OS_mutexGet(0);
		
		
		for(i=0;i<111;i++)
		{
			asm("nop"); // waste of time
		}
		
		OS_mutexRelease(0);

		
	#ifdef MBOXTEST
		OS_mBoxPost(1, 54321);
		
		ret = OS_mBoxPend(0);

	#endif
		
		
		ret = OS_QueueIn(&Qtest , z++); // Put byte into queue, return 1 if q full.

 
		

		OS_Wait(10);
	}

}
