#pragma once
#include "Common.h"

class CLock
{
public:
	CLock();
	void Lock(void);
	void UnLock(void);
	~CLock();
private:
	CRITICAL_SECTION m_CriticalSection;
};

