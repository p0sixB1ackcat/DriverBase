#pragma once
#include "Lock.h"

class AutoLock
{
public:
	AutoLock(CLock *Lock);
	~AutoLock();
private:
	CLock *m_Clock;
};

