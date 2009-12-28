#include "OS/FabOS.h"

// *********  User Task 0
void Task1() 
{

	OS_SetAlarm(0,10);

	while(1)
	{
		OS_WaitAlarm();
		OS_SetAlarm(0,10);
		
		// TODO add your code here



		
	}

}

