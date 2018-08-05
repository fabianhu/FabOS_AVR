/*
	FabOS example File

	(c) 2008-2013 Fabian Huslik
		This is free software according GNU GENERAL PUBLIC LICENSE Version 3.

	Please change this file to your needs.
*/

#include "OS/FabOS.h"

// *********  User Task 1
void Task2() 
{

	OS_SetAlarm(OSALM2,50);

	while(1)
	{
		OS_WaitAlarm(OSALM2);
		OS_SetAlarm(OSALM2,50);
		
		// TODO add your code here



		
	}

}
