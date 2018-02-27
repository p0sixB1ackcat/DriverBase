#include "StdAfx.h"

BOOL LoadDeviceDriver( const char * Name, const char * Path, 
					  HANDLE * lphDevice, PDWORD Error );
BOOL UnloadDeviceDriver( const char * Name );