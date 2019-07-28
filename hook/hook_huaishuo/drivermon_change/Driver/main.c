#include "precomp.h"

#define		DEVICE_NAME		L"\\device\\PopupDrv"
#define		LINK_NAME		L"\\dosDevices\\PopupDrv"


LIST_ENTRY g_OperList;
ERESOURCE  g_OperListLock;

LIST_ENTRY g_WaitList;
ERESOURCE  g_WaitListLock;

LIST_ENTRY g_PendingIrpList;
ERESOURCE  g_PendingIrpListLock;

ULONG g_ulWaitID = 0;

VOID __stdcall LockWrite(ERESOURCE *lpLock)
{
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(lpLock, TRUE);
}


VOID __stdcall UnLockWrite(ERESOURCE *lpLock)
{
    ExReleaseResourceLite(lpLock);
    KeLeaveCriticalRegion();
}


VOID __stdcall LockRead(ERESOURCE *lpLock)
{
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(lpLock, TRUE);
}


VOID __stdcall LockReadStarveWriter(ERESOURCE *lpLock)
{
    KeEnterCriticalRegion();
    ExAcquireSharedStarveExclusive(lpLock, TRUE);
}


VOID __stdcall UnLockRead(ERESOURCE *lpLock)
{
    ExReleaseResourceLite(lpLock);
    KeLeaveCriticalRegion();
}


VOID __stdcall InitLock(ERESOURCE *lpLock)
{
    ExInitializeResourceLite(lpLock);
}

VOID __stdcall DeleteLock(ERESOURCE *lpLock)
{
    ExDeleteResourceLite(lpLock);
}

VOID __stdcall InitList(LIST_ENTRY *list)
{
    InitializeListHead(list);
}

VOID
IrpCancel(
				IN PDEVICE_OBJECT DeviceObject,
				IN PIRP Irp
				)
{
	KIRQL				CancelOldIrql	= Irp->CancelIrql;
	
	IoReleaseCancelSpinLock(DISPATCH_LEVEL);
	KeLowerIrql(CancelOldIrql);

	LockWrite(&g_PendingIrpListLock);
	RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
	UnLockWrite(&g_PendingIrpListLock);
	
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
PendingIrpToList(PIRP lpIrp, PLIST_ENTRY lpIrpList, PDRIVER_CANCEL lpfnCancelRoutine)
{
	InsertTailList(lpIrpList, &lpIrp->Tail.Overlay.ListEntry);
	IoMarkIrpPending(lpIrp);
	IoSetCancelRoutine(lpIrp, lpfnCancelRoutine);
}

//处理应用层的read()函数
NTSTATUS DispatchRead (
    IN PDEVICE_OBJECT	pDevObj,
    IN PIRP	lpIrp) 
{
	NTSTATUS			ntStatus		= STATUS_SUCCESS;
	ULONG				ulLength		= 0;
	PIO_STACK_LOCATION	lpIrpStack		= IoGetCurrentIrpStackLocation(lpIrp);
	OP_INFO				*lpOpInfoEntry	= NULL;
	LIST_ENTRY			*lpOpInfoList	= NULL;
	
	if (lpIrpStack->Parameters.Read.Length < sizeof(RING3_OP_INFO))
	{
		ntStatus = STATUS_INVALID_PARAMETER;
		ulLength = 0;
		goto Completed;
	}
	
	LockWrite(&g_OperListLock);
	
	if (IsListEmpty(&g_OperList) == TRUE)
	{
		UnLockWrite(&g_OperListLock);
		
		LockWrite(&g_PendingIrpListLock);
		PendingIrpToList(lpIrp, &g_PendingIrpList, IrpCancel);
		UnLockWrite(&g_PendingIrpListLock);
	
		goto Pended;
	}
	
	lpOpInfoList = g_OperList.Flink;
	lpOpInfoEntry = CONTAINING_RECORD(lpOpInfoList, OP_INFO, m_List);
	RemoveEntryList(lpOpInfoList);
	UnLockWrite(&g_OperListLock);
	
	RtlCopyMemory(lpIrp->AssociatedIrp.SystemBuffer, lpOpInfoEntry, sizeof(RING3_OP_INFO));
	ntStatus = STATUS_SUCCESS;
	ulLength = sizeof(RING3_OP_INFO);
	
	ExFreePool(lpOpInfoEntry);
	
Completed:
	
	lpIrp->IoStatus.Status = ntStatus;
	lpIrp->IoStatus.Information = ulLength;
	IoCompleteRequest(lpIrp, IO_NO_INCREMENT);
	return ntStatus;
	
Pended:
	return STATUS_PENDING;
}

WAIT_LIST_ENTRY*
FindWaitEntryByID(PLIST_ENTRY ListHead, ULONG ulWaitID)
{
    PLIST_ENTRY			pList		= NULL;
    WAIT_LIST_ENTRY		*pEntry	= NULL;
	
    for (pList = ListHead->Flink; pList != ListHead; pList = pList->Flink)
    {
        pEntry = CONTAINING_RECORD(pList, WAIT_LIST_ENTRY, m_List);
        if (pEntry->m_ulWaitID == ulWaitID)
        {
            return pEntry;
        }
    }
    return NULL;
}

ULONG MakeWaitID()
{
    InterlockedIncrement(&g_ulWaitID);
    return g_ulWaitID;
}

BOOLEAN
CompletePendingIrp(LIST_ENTRY* pIrpListHead, OP_INFO* lpOpInfo)
{
	LIST_ENTRY			*pIrpList	= NULL;
	PIRP				pIrp		= NULL;
	BOOLEAN				bFound		= FALSE;
	BOOLEAN				bRet		= FALSE;
	
	if (IsListEmpty(pIrpListHead) == TRUE)
	{
		return bRet;
	}
	
	for (pIrpList = pIrpListHead->Flink; pIrpList != pIrpListHead; pIrpList = pIrpList->Flink)
	{
		pIrp = CONTAINING_RECORD(pIrpList, IRP, Tail.Overlay.ListEntry);
		if (IoSetCancelRoutine(pIrp, NULL))
		{
			RemoveEntryList(pIrpList);
			bFound = TRUE;
			break;
		}
	}
	
	if (bFound == FALSE)
	{
		return bRet;
	}
	
	RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, lpOpInfo, sizeof(RING3_OP_INFO));
	
	pIrp->IoStatus.Information = sizeof(RING3_OP_INFO);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	bRet = TRUE;
	
	return bRet;
}

R3_RESULT __stdcall GetResultFromUser(HANDLE pID, PUNICODE_STRING strDest)
{
    R3_RESULT			NotifyResult	= R3Result_Pass;
    BOOLEAN				bSuccess		=  FALSE;
    NTSTATUS			Status			= STATUS_SUCCESS;
    LARGE_INTEGER		WaitTimeOut		= {0};
    OP_INFO				*lpNewOpInfo	= NULL;
    WAIT_LIST_ENTRY		*lpNewWaitEntry = NULL;
    ULONG_PTR ulPtr = 0;
    DECLARE_UNICODE_STRING_SIZE(StrProcessName, 260);

    lpNewOpInfo = (OP_INFO*)ExAllocatePool(PagedPool, sizeof(OP_INFO));

    if (lpNewOpInfo == NULL)
    {
        return NotifyResult;
    }
	RtlZeroMemory((PVOID)lpNewOpInfo, sizeof(OP_INFO));
    //设置事件相关的数据，发送给R3，比如进程ID，名字，路径，以及具体操作（创建，修改，删除）等等
	//当然，这里，我们只是简单的捕捉了进程的ID或者名字等
    lpNewOpInfo->m_ulProcessID = (ULONG_PTR)pID;

    lpNewOpInfo->m_ulWaitID = MakeWaitID();//区别不同事件的ID

	//获得进程全路径
	GetProcessFullNameByPid((HANDLE)lpNewOpInfo->m_ulProcessID, &StrProcessName);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"ImageFileName:%wZ\n", &StrProcessName));
	RtlCopyMemory(lpNewOpInfo->m_ProcessName, StrProcessName.Buffer, StrProcessName.Length);

    lpNewWaitEntry = (WAIT_LIST_ENTRY*)ExAllocatePool(NonPagedPool, sizeof(WAIT_LIST_ENTRY));
    if (lpNewWaitEntry == NULL)
    {
        goto End;
    }

	//拷贝目标文件路径
	if (NULL != strDest)
	{
		RtlCopyMemory(lpNewOpInfo->m_DestName, strDest->Buffer, strDest->Length);
	}

    lpNewWaitEntry->m_ulWaitID = lpNewOpInfo->m_ulWaitID;
    KeInitializeEvent(&lpNewWaitEntry->m_ulWaitEvent, SynchronizationEvent, FALSE);
	
    // 插入等待队列，等待R3下发结果
    LockWrite(&g_WaitListLock);
	InsertTailList(&g_WaitList, &lpNewWaitEntry->m_List);
    UnLockWrite(&g_WaitListLock);

	// 等40秒，环3是30秒超时
    WaitTimeOut.QuadPart = -40 * 10000000;

    LockWrite(&g_PendingIrpListLock);
    bSuccess = CompletePendingIrp(&g_PendingIrpList, lpNewOpInfo);//查看是否有未完成的pendingIRP，直接将该OperInfo传给R3
    UnLockWrite(&g_PendingIrpListLock);

	if (bSuccess == FALSE)	//完成pending irp失败，将lpNewOpInfo插入operlist
	{
        LockWrite(&g_OperListLock);
        InsertTailList(&g_OperList, &lpNewOpInfo->m_List); //插入OperList,等待R3来读取
        UnLockWrite(&g_OperListLock);
   
        lpNewOpInfo = NULL;
	}

	Status = KeWaitForSingleObject(&lpNewWaitEntry->m_ulWaitEvent, 
		Executive, KernelMode, FALSE, &WaitTimeOut);//等待R3下发允许或阻止操作

    LockWrite(&g_WaitListLock);
    RemoveEntryList(&lpNewWaitEntry->m_List);
    UnLockWrite(&g_WaitListLock);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"POP WINDOWS FIRST\n"));
    if (Status != STATUS_TIMEOUT)
    {
        if (lpNewWaitEntry->m_bBlocked == TRUE)
        {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"POP WINDOWS R3Result_Block\n"));
            NotifyResult = R3Result_Block;
        }
        else
        {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"POP WINDOWS R3Result_Pass\n"));
            NotifyResult = R3Result_Pass;
        }
    }
    else
    {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"POP WINDOWS R3Result_DefaultNon\n"));
        NotifyResult =  R3Result_DefaultNon;
    }

End:
    if (lpNewWaitEntry != NULL)
    {
        ExFreePool(lpNewWaitEntry);
    }
    if (lpNewOpInfo != NULL)
    {
        ExFreePool(lpNewOpInfo);
    }
    return NotifyResult;
}


//处理应用层的DeviceIoControl()
NTSTATUS DispatchControl(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    PIO_STACK_LOCATION      	lpIrpStack			= NULL;
    PVOID                   	inputBuffer			= NULL;
    PVOID                   	outputBuffer		= NULL;
    ULONG                   	inputBufferLength	= 0;
    ULONG                   	outputBufferLength	= 0;
    ULONG                   	ioControlCode		= 0;
    NTSTATUS		     		ntStatus			= STATUS_SUCCESS;
	
    ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	//获取当前IRP堆栈位置
	lpIrpStack = IoGetCurrentIrpStackLocation (Irp);
	//获得输入缓冲和长度
	inputBuffer = Irp->AssociatedIrp.SystemBuffer;
	inputBufferLength = lpIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	//获得输出缓冲和长度
	outputBuffer = Irp->AssociatedIrp.SystemBuffer;
	outputBufferLength = lpIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	//获取控制码
	ioControlCode = lpIrpStack->Parameters.DeviceIoControl.IoControlCode;
		
	switch (ioControlCode ) 
	{
		case IOCTL_SEND_RESULT_TO_R0://R3向内核传递弹窗结果，将对应的WaitID事件设置用户选择结果
		{
				RING3_REPLY			*lpReply		= NULL;
				WAIT_LIST_ENTRY		*lpWaitEntry	= NULL;
							
				if (lpIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(RING3_REPLY))
				{
						Irp->IoStatus.Information = 0;
						Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
						break;
				}
				lpReply = (RING3_REPLY*)Irp->AssociatedIrp.SystemBuffer;
							
				LockWrite(&g_WaitListLock);
				lpWaitEntry = FindWaitEntryByID(&g_WaitList, lpReply->m_ulWaitID);//根据WaitID，找到对应的拦截事件
							
				if (lpWaitEntry != NULL)
				{
						lpWaitEntry->m_bBlocked = lpReply->m_ulBlocked;
						KeSetEvent(&lpWaitEntry->m_ulWaitEvent, 0, FALSE);//设置EVENT事件，唤醒GetResultFromUser()里的等待事件
				}
							
				UnLockWrite(&g_WaitListLock);
							
				Irp->IoStatus.Information = 0;
				ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
		}
		break;

		case IOCTL_XXX_ATTACK://攻击拦截模仿
		{
				R3_RESULT notifyResult = R3Result_DefaultNon; 

							
				notifyResult = GetResultFromUser(PsGetCurrentProcessId(), NULL);//从R3获得弹框结果，是阻止还是放过
				if (notifyResult == R3Result_Block)
				{
						DbgPrint("阻止\n");
						*(ULONG *)outputBuffer = 0;
						ntStatus = STATUS_SUCCESS;
				}
				else if (notifyResult == R3Result_Pass)
				{
						DbgPrint("允许\n");
						*(ULONG *)outputBuffer = 1;
						ntStatus = STATUS_SUCCESS;
				}
				else
				{
						DbgPrint("超时允许\n");
						*(ULONG *)outputBuffer = 1;
						ntStatus = STATUS_SUCCESS;
				}

		}
		Irp->IoStatus.Information = sizeof(ULONG);
		Irp->IoStatus.Status = ntStatus;
		break;

		default:
		break;
	}
		
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return ntStatus;  
}

//驱动Unload（）函数
VOID DriverUnload (
    IN PDRIVER_OBJECT	pDriverObject) 
{
	UNICODE_STRING         deviceLink = {0};

	RemoveHook();

	DeleteLock(&g_OperListLock);
	DeleteLock(&g_WaitListLock);
	DeleteLock(&g_PendingIrpListLock);

	RtlInitUnicodeString( &deviceLink, LINK_NAME);
	IoDeleteSymbolicLink( &deviceLink);
	IoDeleteDevice( pDriverObject->DeviceObject );


	return;
}

//处理应用层的create()函数
NTSTATUS DispatchCreate (
	IN PDEVICE_OBJECT	pDevObj,
	IN PIRP	pIrp) 
{
	//设置IO状态信息
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	//完成IRP操作，不向下层驱动发送
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

//处理应用层的close()函数
NTSTATUS DispatchClose (
    IN PDEVICE_OBJECT	pDevObj,
    IN PIRP	pIrp) 
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

//驱动程序入口，完成各种初始化工作，创建设备对象
NTSTATUS DriverEntry (
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath) 
{
	NTSTATUS 		status		= STATUS_SUCCESS;
	PDEVICE_OBJECT 	pDevObj		= NULL;
	UNICODE_STRING 	uDevName	= {0};
	UNICODE_STRING 	uLinkName	= {0};
	DbgPrint("Driver Load begin!\n");

	InitLock(&g_OperListLock);
	InitLock(&g_WaitListLock);
	InitLock(&g_PendingIrpListLock);
	
	InitList(&g_OperList);
	InitList(&g_WaitList);
	InitList(&g_PendingIrpList);


	//初始化各个例程

	pDriverObject->MajorFunction[IRP_MJ_CREATE] =
				DispatchCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] =
				DispatchClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] =
				DispatchRead;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = 
				DispatchControl;
	pDriverObject->DriverUnload	= 
				DriverUnload;

	RtlInitUnicodeString(&uDevName, DEVICE_NAME);
	//创建驱动设备
	status = IoCreateDevice( pDriverObject,
			0,//sizeof(DEVICE_EXTENSION)
			&uDevName,
			FILE_DEVICE_UNKNOWN,
			0, FALSE,
			&pDevObj );
	if (!NT_SUCCESS(status))
	{
		DbgPrint("IoCreateDevice Failed:%x\n", status);
		return status;
	}

	pDevObj->Flags |= DO_BUFFERED_IO;
	RtlInitUnicodeString(&uLinkName, LINK_NAME);
	//创建符号链接
	status = IoCreateSymbolicLink( &uLinkName, &uDevName );
	if (!NT_SUCCESS(status)) 
	{
		//STATUS_INSUFFICIENT_RESOURCES 	资源不足
		//STATUS_OBJECT_NAME_EXISTS 		指定对象名存在
		//STATUS_OBJECT_NAME_COLLISION 	对象名有冲突
		DbgPrint("IoCreateSymbolicLink Failed:%x\n", status);
		IoDeleteDevice( pDevObj );
		return status;
	}
	StartHook();

	DbgPrint("Driver Load success!\n");
	return status;
}

