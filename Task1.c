/*
	FabOS example File

	(c) 2008-2010 Fabian Huslik

	Please change this file to your needs.
*/

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

