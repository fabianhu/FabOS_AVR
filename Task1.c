/*
	FabOS example File

	(c) 2008-2013 Fabian Huslik
		This is free software according GNU GENERAL PUBLIC LICENSE Version 3.

	Please change this file to your needs.
*/

#include "OS/FabOS.h"

// *********  User Task 0
void Task1() 
{

	OS_SetAlarm(OSALM1,10);

	while(1)
	{
		OS_WaitAlarm(OSALM1);
		OS_SetAlarm(OSALM1,10);
		
		// TODO add your code here



		
	}

}

