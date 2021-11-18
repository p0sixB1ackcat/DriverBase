#pragma once
#include "AutoLock.h"

class ClassA
{
public:
	static ClassA *ShareInstance(void);
	void func(void);
private:
	ClassA();
	~ClassA();
	static ClassA *_SelfClass;
	CLock m_Clock;
};

