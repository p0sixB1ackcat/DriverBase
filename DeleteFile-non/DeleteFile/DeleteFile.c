#include <ntddk.h>


#define DEVICE_NAME L"\\device\\DeleteFile"
#define LINK_NAME L"\\dosdevices\\DeleteFile"


VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("Goodbyte!!!\n");
}

NTSTATUS DispatchCommon(PDEVICE_OBJECT pDeviceObject,PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchIocontrol(PDEVICE_OBJECT pDeviceObject,PIRP pIrp)
{	
	NTSTATUS osStatus = 0;
	UNICODE_STRING FileName = {0};
	PIO_STACK_LOCATION  pStack = NULL;
	OBJECT_ATTRIBUTES objAttribute = {0};
	HANDLE handle = NULL;
	IO_STATUS_BLOCK iosb = {0};
	FILE_DISPOSITION_INFORMATION fdinfo = {0};
	
	if(pIrp->AssociatedIrp.SystemBuffer == NULL)
		goto goexit;
	RtlInitUnicodeString(&FileName,pIrp->AssociatedIrp.SystemBuffer);

	DbgPrint("%ws\n",FileName);

	InitializeObjectAttributes(&objAttribute,
		&FileName,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	
	osStatus = ZwCreateFile(&handle,
		SYNCHRONIZE | FILE_WRITE_DATA | DELETE,
		&objAttribute,
		&iosb,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);
	if(osStatus == STATUS_OBJECT_NAME_NOT_FOUND || osStatus == STATUS_OBJECT_PATH_NOT_FOUND)
	{
		DbgPrint("没有找到要删除的文件\n");
		goto goexit;
	}
	else if(!NT_SUCCESS(osStatus))
	{
		DbgPrint("ZwCreateFile failed %d\n",osStatus);
		goto goexit;
	}


	fdinfo.DeleteFile = TRUE;
	osStatus = ZwSetInformationFile(handle,
		&iosb,
		&fdinfo,
		sizeof(fdinfo),
		FileDispositionInformation);
	
	if(!NT_SUCCESS(osStatus))
	{
		DbgPrint("ZwSetInformationFile faliled\n");
		goto goexit;
	}

	DbgPrint("DeleteFile Successfual!!!\n");
	osStatus = STATUS_SUCCESS;
goexit:
	pIrp->IoStatus.Status = osStatus;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	return osStatus;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING pRegistryPath)
{
	UNICODE_STRING uDeviceName = {0};
	UNICODE_STRING uLinkName = {0};
	NTSTATUS sTatus = 0;
	int i = 0;
	PDEVICE_OBJECT pDeviceObject = NULL;
	RtlInitUnicodeString(&uDeviceName,DEVICE_NAME);
	RtlInitUnicodeString(&uLinkName,LINK_NAME);

	sTatus = IoCreateDevice(pDriverObject,
		0,
		&uDeviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&pDeviceObject);
	if(!NT_SUCCESS(sTatus))
	{
		DbgPrint("IoCreateDevice failed!\n");
		return sTatus;
	}
	
	sTatus = IoCreateSymbolicLink(&uLinkName,&uDeviceName);
	if(!NT_SUCCESS(sTatus))
	{
		IoDeleteDevice(pDeviceObject);
		DbgPrint("IoCreateSymbolicLink failed!\n");
		return sTatus;
	}

	for(i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObject->MajorFunction[i] = DispatchCommon;
	}

	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIocontrol;

	pDeviceObject->Flags = DO_BUFFERED_IO;

	pDriverObject->DriverUnload = DriverUnload;

	DbgPrint("DriverEntry!!!\n");

	return STATUS_SUCCESS;
}