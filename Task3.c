/*
	FabOS example File

	(c) 2008-2010 Fabian Huslik

	Please change this file to your needs.
*/

#include "OS/FabOS.h"

// *********  User Task 2
void Task3() 
{

	OS_SetAlarm(2,100);

	while(1)
	{
		OS_WaitAlarm();
		OS_SetAlarm(2,100);
		
		// TODO add your code here



		
	}

}
