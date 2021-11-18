#include <ntifs.h>
#include <ntddk.h>

#define DEVICE_NAME L"\\device\\PBCPopWindow"
#define SYMBOLIC_NAME L"\\dosDevices\\PBCPopWindow"

typedef struct _OP_INFO
{
	WCHAR m_ProcessName[MAX_PATH];
	ULONG m_ProcessId;
	ULONG m_waitId;
	LIST_ENTRY m_entry;

}OP_INFO,*POP_INFO;

typedef struct _WAIT_LIST_ENDTY
{
	LIST_ENTRY m_entry;
	ULONG m_waitId;
	KEVENT m_waitEvent;
	ULONG m_Block;

}WAIT_LIST_ENTRY;


ERESOURCE g_OperListLock;
LIST_ENTRY g_OperListEntry;


ERESOURCE g_WaitListLock;
LIST_ENTRY g_WaitListEntry;

ERESOURCE g_PendingListLock;
LIST_ENTRY g_PendingListEntry;

ULONG g_ulCurrentWaitId = 0;



VOID PBCInitLock(PERESOURCE locked)
{
	ExInitializeResourceLite(locked);
}

VOID PBCInitListEntry(PLIST_ENTRY pListEntry)
{
	InitializeListHead(pListEntry);
}

VOID PBCDeleteLock(ERESOURCE *pResouce)
{
	ExDeleteResourceLite(pResouce);
}

NTSTATUS PBCCommand(PDRIVER_OBJECT pDriverObject,PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS PBCRead(PDRIVER_OBJECT pDriverObject,PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	
	return STATUS_SUCCESS;
}

NTSTATUS PBCIoControl(PDRIVER_OBJECT pDriverObject,PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

PVOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("PBCPopWindow:goodbye!\n");
	return;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING pDriverPath)
{
	NTSTATUS ntStatus;
	UNICODE_STRING uDeviceName = {0x00};
	UNICODE_STRING uSymbolicName = {0x00};
	DEVICE_OBJECT pDeviceObject = {0x00};
	ULONG i;
	

	//初始化三个读写共享资源锁
	InitLock(&g_OperListLock);
	InitLock(&g_WaitListLock);
	InitLock(&g_PendingListLock);

	//初始化三个链表
	InitListEntry(&g_OperListEntry);
	InitListEntry(&g_WaitListEntry);
	InitListEntry(&g_PendingListEntry);

	RtlInitUnicodeString(&uDeviceName,DEVICE_NAME);
	RtlInitUnicodeString(&uSymbolicName,SYMBOLIC_NAME);

	ntStatus = IoCreateDevice(pDriverObject,
		0,
		&uDeviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&pDeviceObject);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoCreateDevice faild:%x\n",ntStatus);
		return ntStatus;
	}
	
	for(i = 0; i < IRP_MJ_DEVICE_CONTROL; i++)
	{
		pDriverObject[i] = Command;
	}
	pDriverObject[IRP_MJ_READ] = PBCRead;
	pDriverObject[IRP_MJ_DEVICE_CONTROL] = PBCIoControl;
	pDriverObject->DriverUnload = DriverUnload;


	return ntStatus;


}