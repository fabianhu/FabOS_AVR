#include "FabOS.h"
uint16_t ret;

void Task1() 
{
	uint32_t i;
	
	while(1)
	{
		for(i=0;i<111;i++)
		{
			asm("nop"); // waste of time
		}
		
		OS_mBoxPost(1, 54321);
		
		ret = OS_mBoxPend(0);

		
		
		OS_Wait(10);
	}

}
