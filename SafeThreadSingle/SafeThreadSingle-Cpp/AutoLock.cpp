#include "AutoLock.h"

AutoLock::AutoLock(CLock *Lock) :m_Clock(Lock)
{
	m_Clock->Lock();
}

AutoLock::~AutoLock()
{
	m_Clock->UnLock();
}
