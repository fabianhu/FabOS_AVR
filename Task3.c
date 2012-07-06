/*
	FabOS example File

	(c) 2008-2012 Fabian Huslik

	Please change this file to your needs.
*/

#include "OS/FabOS.h"

// *********  User Task 2
void Task3() 
{

	OS_SetAlarm(OSALM3,100);

	while(1)
	{
		OS_WaitAlarm(OSALM3);
		OS_SetAlarm(OSALM3,100);
		
		// TODO add your code here



		
	}

}
