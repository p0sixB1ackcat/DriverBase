#include <ntifs.h>
#include <wdm.h>


#define DeviceName L"\\Device\\PBCDriver"
#define SymbolicLinkName L"\\DosDevices\\PBCDriver"

#define PBCBASECTLCODE 0x800

#define PBCCTLCODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN, PBCBASECTLCODE + i, METHOD_BUFFERED, FILE_ALL_ACCESS)

#define PBCINLINEHOOK_CTL PBCCTLCODE(1)
#define PBCKILLTHREADBYAPC_CTL PBCCTLCODE(2)

VOID InitializerGlobalVar(VOID);
VOID LockByEResource(ERESOURCE *pLock);
VOID UnlockByEResource(ERESOURCE *pLock);

ERESOURCE g_MyDriverObjLock = {0x00};
static PDRIVER_OBJECT g_MyDriverObject = NULL;

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint(L"PBCDriver is Unload!\n");

}

NTSTATUS DispatchIoCommon(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;
	
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return ntStatus;
}

NTSTATUS DispatchIoControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	IO_STACK_LOCATION *pCurrentIrpStack = NULL;
	ULONG dwIoCtrlCode = 0;
	ULONG dwThreadId = 0;

	pCurrentIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	dwIoCtrlCode = pCurrentIrpStack->Parameters.DeviceIoControl.IoControlCode;
	
	switch (dwIoCtrlCode)
	{
		case PBCINLINEHOOK_CTL:
		{
			pIrp->AssociatedIrp.SystemBuffer;
		}
		break;
		case PBCKILLTHREADBYAPC_CTL:
		{
			dwThreadId = *((ULONG *)pIrp->AssociatedIrp.SystemBuffer);
			DbgPrint("R3 Send ThreadId is %d\n", dwThreadId);
		}
		break;
	}

	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = sizeof(dwThreadId);
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return ntStatus;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING uDeviceName = {0x00};
	UNICODE_STRING uSymbolicLinkName = {0x00};
	PDEVICE_OBJECT pDeviceObject = NULL;
	ULONG i;

	RtlInitUnicodeString(&uDeviceName, DeviceName);

	ntStatus = IoCreateDevice(pDriverObject,0,&uDeviceName,FILE_DEVICE_UNKNOWN,0,FALSE,&pDeviceObject);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint(L"IoCreateDevice fail:%d!\n", ntStatus);
		goto ret;
	}
	
	RtlInitUnicodeString(&uSymbolicLinkName, SymbolicLinkName);
	ntStatus = IoCreateSymbolicLink(&uSymbolicLinkName, &uDeviceName);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint(L"IoCreateSymbolicLink fail:%d\n", ntStatus);
		goto ret;
	}

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = DispatchIoCommon;
	}

	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoControl;
	pDriverObject->DriverUnload = DriverUnload;

	InitializerGlobalVar();

	g_MyDriverObject = pDriverObject;

	DbgPrint(L"PBCDriver DriverEntry Success!\n");
ret:
	
	return ntStatus;
}

VOID InitializerGlobalVar(VOID)
{
	ExInitializeResourceLite(&g_MyDriverObjLock);
}

VOID LockByEResource(ERESOURCE *pLock)
{
	KeEnterCriticalRegion();
	ExAcquireResourceExclusive(pLock,TRUE);
}

VOID UnlockByEResource(ERESOURCE *pLock)
{
	ExReleaseResourceLite(pLock);

	KeLeaveCriticalRegion();
}