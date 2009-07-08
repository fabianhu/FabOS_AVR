#include "FabOS.h"
uint16_t ret;

void Task1() 
{
	uint32_t i;
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
		
		//OS_Wait(10);
	}

}
