#include "ClassA.h"
#include "Common.h"
#include "AutoLock.h"

ClassA * ClassA::_SelfClass = NULL;

ClassA *ClassA::ShareInstance(void)
{
	if (_SelfClass == NULL)
	{
		CLock(m_Clock);
		if(_SelfClass == NULL)
			_SelfClass = new ClassA;
	}
	return _SelfClass;
}

void ClassA::func(void)
{
	cout << "%s" << __FUNCTION__ << endl;
}

ClassA::ClassA()
{

}


ClassA::~ClassA()
{
}
