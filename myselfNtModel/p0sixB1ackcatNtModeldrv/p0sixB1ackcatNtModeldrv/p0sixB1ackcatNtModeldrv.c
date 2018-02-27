#include <ntddk.h>

#define DEVICE_NAME L"\\device\\p0sixB1ackcatNtModeldrv"
#define LINK_NAME L"\\dosdevices\\p0sixB1ackcatNtModeldrv"

#define IOCTRL_BASE 0x800

#define MYIOCTRL_CODE(i) \
	CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTRL_BASE+i,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTRL_HELLO MYIOCTRL_CODE(0)
#define IOCTRL_PRINT MYIOCTRL_CODE(1)
#define IOCTRL_BYE MYIOCTRL_CODE(2)

NTSTATUS DispatchCommon(PDEVICE_OBJECT pDeviceObject,PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchCreate(PDEVICE_OBJECT pDeviceObject,PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT pDeviceObject,PIRP pIrp)
{
	PVOID pInputBuffer = NULL;
	PIO_STACK_LOCATION pStack = NULL;
	ULONG uInputLength = 0;
	ULONG uSendDataLength = 0;
	ULONG uMin = 0;
	pInputBuffer = pIrp->AssociatedIrp.SystemBuffer;
	pStack = IoGetCurrentIrpStackLocation(pIrp);

	uInputLength = pStack->Parameters.Read.Length;
	uSendDataLength = (wcslen(L"Hello world") + 1) * sizeof(WCHAR);

	uMin = uSendDataLength > uInputLength ? uInputLength : uSendDataLength;

	RtlCopyMemory(pInputBuffer,L"Hello world",uMin);

	pIrp->IoStatus.Status = 0;
	pIrp->IoStatus.Information = uMin;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;

}

NTSTATUS DispatchWrite(PDEVICE_OBJECT pDeviceObject,PIRP pIrp)
{
	PVOID pInputBuffer = NULL;
	ULONG uInputLength = 0;
	PIO_STACK_LOCATION pStack = NULL;
	PVOID kernelHeap = NULL;
	pInputBuffer = pIrp->AssociatedIrp.SystemBuffer;
	pStack = IoGetCurrentIrpStackLocation(pIrp);
	uInputLength = pStack->Parameters.Write.Length;
	kernelHeap = ExAllocatePoolWithTag(PagedPool,uInputLength,'TSET');
	if(kernelHeap == NULL)
	{
		pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp,IO_NO_INCREMENT);
		return STATUS_INSUFFICIENT_RESOURCES; 
	}
	memset(kernelHeap,0x00,uInputLength);

	RtlCopyMemory(kernelHeap,pInputBuffer,uInputLength);

	ExFreePool(kernelHeap);
	kernelHeap = NULL;

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = uInputLength;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
	
}

NTSTATUS DispatchClean(PDEVICE_OBJECT pDeviceObject,PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchClose(PDEVICE_OBJECT pDeviceObject,PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchIocontrol(PDEVICE_OBJECT pDeviceObject,PIRP pIrp)
{
	PVOID pInputBuffer = NULL;
	PVOID pOutputBuffer = NULL;
	ULONG uInputLength = 0;
	ULONG uOutputLength = 0;
	//获取r3指定的iocontrol控制码
	ULONG uIocontrolCode = 0;
	PIO_STACK_LOCATION pStack = NULL;

	pInputBuffer = pOutputBuffer = pIrp->AssociatedIrp.SystemBuffer;
	pStack = IoGetCurrentIrpStackLocation(pIrp);
	uInputLength = pStack->Parameters.DeviceIoControl.InputBufferLength;
	uOutputLength = pStack->Parameters.DeviceIoControl.OutputBufferLength;

	uIocontrolCode = pStack->Parameters.DeviceIoControl.IoControlCode;
	
	switch(uIocontrolCode)
	{
	case IOCTRL_HELLO:
		DbgPrint("Hello Iocontrol\n");
		break;
	case IOCTRL_PRINT:
		DbgPrint("%ws\n",pInputBuffer);
		break;
	case IOCTRL_BYE:
		DbgPrint("GoodBye Iocontrol\n");
		break;
	default:
		DbgPrint("unknow Iocontrol\n");
		break;
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = uInputLength;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;

}

VOID DRIVERUNLOAD(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING uLinkName = {0};
	RtlInitUnicodeString(&uLinkName,LINK_NAME);
	IoDeleteSymbolicLink(&uLinkName);
	IoDeleteDevice(pDriverObject->DeviceObject);

	DbgPrint("p0sixB1ackcat is unload!!!\n");
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING pRegPath)
{
	NTSTATUS ntStatus = 0;
	UNICODE_STRING uDeviceName = {0};
	UNICODE_STRING uLinkName = {0};
	PDEVICE_OBJECT pDeviceObject = NULL;
	ULONG i = 0;

	RtlInitUnicodeString(&uDeviceName,DEVICE_NAME);
	RtlInitUnicodeString(&uLinkName,LINK_NAME);

	ntStatus = IoCreateDevice(pDriverObject,0,&uDeviceName,FILE_DEVICE_UNKNOWN,0,FALSE,&pDeviceObject);

	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoCreateDevice falied\n");
		return ntStatus;
	}

	ntStatus = IoCreateSymbolicLink(&uLinkName,&uDeviceName);
	if(!NT_SUCCESS(ntStatus))
	{
		IoDeleteDevice(pDeviceObject);
		DbgPrint("IoCreateSymbolicLink failed\n");
		return ntStatus;
	}
	
	//指定read和write的通讯方式
	pDeviceObject->Flags |= DO_BUFFERED_IO;
	
	for(i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
	{
		pDriverObject->MajorFunction[i] = DispatchCommon;
	}

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pDriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchClean;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIocontrol;

	pDriverObject->DriverUnload = DRIVERUNLOAD;

	DbgPrint("p0sixB1ackcat DriverEntry!!!\n");
	ntStatus = STATUS_SUCCESS;
	return ntStatus;


}
