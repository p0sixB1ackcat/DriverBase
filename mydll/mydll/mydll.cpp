// mydll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "mydll.h"


// This is an example of an exported variable
MYDLL_API int nmydll=0;

// This is an example of an exported function.
MYDLL_API int fnmydll(void)
{
	return 42;
}

MYDLL_API int fnMyFunc(void)
{

	return 0;
}


// This is the constructor of a class that has been exported.
// see mydll.h for the class definition
Cmydll::Cmydll()
{
	return;
}
