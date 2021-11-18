#include <ntddk.h>

ULONG g_uTotalNums = 0;
ULONG g_uAnotherNums = 10;
FAST_MUTEX g_OneMutex;
FAST_MUTEX g_TwoMutex;

VOID startThread(VOID);

VOID Thread1Func(PVOID);

VOID Thread2Func(PVOID);

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("PBCLockThread:goodbye!\n");
	return;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING pDriverPath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	DbgPrint("PBCLockThread:hello\n");

	pDriverObject->DriverUnload = DriverUnload;

	ExInitializeFastMutex(&g_OneMutex);

	ExInitializeFastMutex(&g_TwoMutex);

	startThread();
	
	return ntStatus;
}

VOID startThread(VOID)
{
	HANDLE hThread1Handle = NULL;
	HANDLE hThread2Handle = NULL;
	NTSTATUS ntStatus;
	PVOID pThreadFileObjects[2] = {NULL};

	ntStatus = PsCreateSystemThread(&hThread1Handle,
		0,
		NULL,
		(HANDLE)0,
		NULL,
		Thread1Func,
		NULL);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PsCreateSystemThread1 faild:%x\n",ntStatus);
		return;
	}

	ntStatus = PsCreateSystemThread(&hThread2Handle,
		0,
		NULL,
		(HANDLE)0,
		NULL,
		Thread2Func,
		NULL);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PsCreateSystemThread2 faild:%x\n",ntStatus);
		return;
	}

	//判断当前的中断请求级别
	if(KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		ntStatus = KfRaiseIrql(PASSIVE_LEVEL);
	}
	if(KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		DbgPrint("KfRaiseIrql faild:%x\n",ntStatus);
		return;
	}

	//创建完成之后，获取两个线程的句柄
	ntStatus = ObReferenceObjectByHandle(hThread1Handle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&pThreadFileObjects[0],
		NULL);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ObReferenceObjectByHandle hThread1Handle faild:%x\n",ntStatus);
		return;
	}

	ntStatus = ObReferenceObjectByHandle(hThread2Handle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&pThreadFileObjects[1],
		NULL);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ObReferenceObjectByHandle faild:%x\n",ntStatus);
		ObDereferenceObject(pThreadFileObjects[0]);
		return;
	}
	
	KeWaitForMultipleObjects(2,
		pThreadFileObjects,
		WaitAll,
		Executive,
		KernelMode,
		FALSE,
		NULL,
		NULL);

	//等待结束后，就直接退出了
	ObDereferenceObject(pThreadFileObjects[0]);
	ObDereferenceObject(pThreadFileObjects[1]);

	return;


}

VOID Thread1Func(PVOID pContext)
{
	ULONG i = 30;
	while(i > 0)
	{
		//上锁
		ExAcquireFastMutex(&g_OneMutex);
		ExAcquireFastMutex(&g_TwoMutex);

		g_uAnotherNums--;
		g_uTotalNums++;

		DbgPrint("%s:g_uAnotherNums is %d,g_uTotalNums is %d\n",__FUNCDNAME__,g_uAnotherNums,g_uTotalNums);

		//解锁
		ExReleaseFastMutex(&g_OneMutex);
		ExReleaseFastMutex(&g_TwoMutex);

		i--;
	}
	
	
}

VOID Thread2Func(PVOID pContext)
{
	ULONG i = 30;
	while(i > 0)
	{
		//上锁
		ExAcquireFastMutex(&g_OneMutex);
		ExAcquireFastMutex(&g_TwoMutex);

		g_uAnotherNums--;
		g_uTotalNums++;

		DbgPrint("%s:g_uAnotherNums is %d,g_uTotalNums is %d\n",__FUNCDNAME__,g_uAnotherNums,g_uTotalNums);

		ExReleaseFastMutex(&g_OneMutex);
		ExReleaseFastMutex(&g_TwoMutex);
		i--;
	}
	
}