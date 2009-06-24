#include "FabOS.h"


uint16_t ret;

void Task3() 
{
	uint32_t i;
	
uint32_t j,k,l,m;


	while(1)
	{	
		j = 4; 
		k = 5;
		l = 6;
			

		for(i=0;i<333;i++)
		{
			m= j*k+l;
			l= 7*j;
			if (m % 4) j ++;
		
			asm("nop"); // waste of time
		}

		ret = OS_mBoxPend(1);

		OS_mBoxPost(0, 1234);



		OS_Wait(30);
	}
}
