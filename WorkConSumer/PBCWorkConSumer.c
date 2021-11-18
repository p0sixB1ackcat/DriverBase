#include <ntddk.h>

KSEMAPHORE g_Ksemaphore;
ULONG g_uTotalNums = 0;

VOID StartThread(VOID);

VOID WorkerFunc(PVOID);

VOID ConSumerFunc(PVOID);



VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("PBCWorkConSumer:goodbye!\n");
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING pDriverPath)
{
	DbgPrint("PBCWorkConSumer:hello\n");

	pDriverObject->DriverUnload = DriverUnload;

	KeInitializeSemaphore(&g_Ksemaphore,0,10);

	StartThread();

	return STATUS_SUCCESS;
	
}

VOID StartThread(VOID)
{
	HANDLE hWorkerThreadHandle = NULL;
	HANDLE hConSumerThreadHandle = NULL;
	PVOID pThreadFileObjects[2] = {NULL};
	NTSTATUS ntStatus;

	ntStatus = PsCreateSystemThread(&hWorkerThreadHandle,
		0,
		NULL,
		(HANDLE)0,
		NULL,
		WorkerFunc,
		NULL);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PsCreateSystemThread Worker faild:%x\n",ntStatus);
		return;
	}

	ntStatus = PsCreateSystemThread(&hConSumerThreadHandle,
		0,
		NULL,
		(HANDLE)0,
		NULL,
		ConSumerFunc,
		NULL);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PsCreateSystemThread ConSumer faild:%x\n",ntStatus);
		return;
	}
	
	if(KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		ntStatus = KfRaiseIrql(PASSIVE_LEVEL);
	}
	if(KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		DbgPrint("KfRaiseIrql faild:%x\n",ntStatus);
		return;
	}

	ntStatus = ObReferenceObjectByHandle(hWorkerThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&pThreadFileObjects[0],
		NULL);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ObReferenceObjectByHandle Work faild:%x\n",ntStatus);
		return;
	}

	ntStatus = ObReferenceObjectByHandle(hConSumerThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&pThreadFileObjects[1],
		NULL);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ObReferenceObjectByHandle Consumer faild:%x\n",ntStatus);
		ObDereferenceObject(pThreadFileObjects[0]);
		return;
	}

	ntStatus = KeWaitForMultipleObjects(2,
		pThreadFileObjects,
		WaitAll,
		Executive,
		KernelMode,
		FALSE,
		NULL,
		NULL);
	
	ObDereferenceObject(pThreadFileObjects[0]);
	ObDereferenceObject(pThreadFileObjects[1]);

	return;


}

VOID WorkerFunc(PVOID pContext)
{
	ULONG i = 0;
	//用来表示大数的结构
	LARGE_INTEGER timeOut = {0x00};
	timeOut.QuadPart = -3 * 10000000i64;

	while(i < 10)
	{
		//将当前线程从等待访问资源列表中移除，并将信号量g_Ksemaphore+1，相当于生产一个元素
		KeReleaseSemaphore(&g_Ksemaphore,IO_NO_INCREMENT,1,FALSE);

		g_uTotalNums++;
		DbgPrint("%s:g_uTotalNums is %d\n",__FUNCDNAME__,g_uTotalNums);
		i++;

		//延迟3秒
		KeDelayExecutionThread(KernelMode,FALSE,&timeOut);
	}
}

VOID ConSumerFunc(PVOID pContext)
{
	ULONG i = 0;
	LARGE_INTEGER timeOut = {0x00};
	timeOut.QuadPart = -3 * 10000000i64;

	while(i < 10)
	{
		//将当前线程加入到等待资源列表中，并将信号量g_Ksemaphore-1，相当于消耗一个元素
		KeWaitForSingleObject(&g_Ksemaphore,
			Executive,
			KernelMode,
			FALSE,
			&timeOut);

		g_uTotalNums--;
		DbgPrint("%s:g_uTotalNums is %d\n",__FUNCDNAME__,g_uTotalNums);
		i++;

		KeDelayExecutionThread(KernelMode,FALSE,&timeOut);

	}
}