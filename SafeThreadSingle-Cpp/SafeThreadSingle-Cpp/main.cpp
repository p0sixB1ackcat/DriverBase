#include "Common.h"
#include "Lock.h"
#include "ClassA.h"
#include <Process.h>

void __cdecl Thread1Func(void *lpContext)
{
	ClassA *C = ClassA::ShareInstance();
	cout<< __FUNCTION__ << " GetInstance Address is 0x%x" << C << endl;
}

void __cdecl Thread2Func(void *lpContext)
{
	ClassA *C = ClassA::ShareInstance();
	cout << __FUNCTION__ << " GetInstance Address is 0x%x" << C << endl;
}

int main(int argc, char *argv[])
{
	HANDLE handles[2] = { 0x00 };
	ClassA *C1 = ClassA::ShareInstance();
	C1->func();

	ClassA *C2 = ClassA::ShareInstance();
	C2->func();

	cout << "C1 Address is 0x%x" << C1 << endl;
	cout << "C2 Address is 0x%x" << C2 << endl;

	handles[0] = (HANDLE)_beginthread(Thread1Func,0,NULL);
	handles[1] = (HANDLE)_beginthread(Thread2Func,0,NULL);

	WaitForMultipleObjects(2,handles,TRUE,INFINITE);


	return 0;
}