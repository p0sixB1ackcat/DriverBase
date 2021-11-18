#include <ntddk.h>

#define IOCTRL_BASIC 0x8000
#define MYIOCTRL_CODE(i)\
	CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTRL_BASIC + i,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTRL_HELLO MYIOCTRL_CODE(0)
#define IOCTRL_PRINT MYIOCTRL_CODE(1)
#define IOCTRL_BYE MYIOCTRL_CODE(2)
#define SystemHandleInformation 16

const WCHAR aDeviceName[] = L"\\DosDevices\\PBCDeleteFile";

const WCHAR aDeviceSymbolicName[] = L"\\Device\\PBCDeleteFile";

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
	USHORT UniqueProcessId;
	USHORT CreatorBackTraceIndex;
	UCHAR ObjectTypeIndex;
	UCHAR HandleAttributes;
	USHORT HandleValue;
	PVOID Object;
	ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG NumberOfHandles;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

NTSTATUS PBCOpenFile(WCHAR *,
					 PHANDLE,
					 ACCESS_MASK,
					 ULONG);
BOOLEAN PBCDeleteFile(WCHAR *);

BOOLEAN PBCCloseHandle(WCHAR *);

NTSTATUS PBCQuerySymbolicLink(PUNICODE_STRING,PUNICODE_STRING);

NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(ULONG SystemInformationClass,
												 PULONG SystemInformation,
												 ULONG SystemInformationLength,
												 PULONG returnLength);
NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
				  IN HANDLE SourceProcessHandle,
				  IN HANDLE SourceHandle,
				  IN HANDLE TargetProcessHandle OPTIONAL,
				  OUT PHANDLE TargetHandle OPTIONAL,
				  IN ACCESS_MASK DesiredAccess,
				  IN ULONG HandleAttributes,
				  IN ULONG Options
				  );

NTSTATUS
ObQueryNameString(
				  IN PVOID,
				  OUT POBJECT_NAME_INFORMATION,
				  IN ULONG,
				  OUT PULONG
				  );

NTSTATUS
NTAPI
PBCSkillFileCompleation(PDEVICE_OBJECT,
						PIRP,
						PVOID);

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	if(pDriverObject)
	{
		UNICODE_STRING uDeviceSymblicLinkName = {0x00};
		RtlInitUnicodeString(&uDeviceSymblicLinkName,aDeviceSymbolicName);
		IoDeleteSymbolicLink(&uDeviceSymblicLinkName);

		IoDeleteDevice(pDriverObject->DeviceObject);
	}

	DbgPrint("PBCDeleteFile:goodbye!\n");

	return;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING pDriverUpath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING uDeviceName = {0x00};
	UNICODE_STRING uDeviceSymbolicName = {0x00};
	PDEVICE_OBJECT deviceObject;
	

	pDriverObject->DriverUnload = DriverUnload;
	
	
	RtlInitUnicodeString(&uDeviceName,aDeviceName);
	RtlInitUnicodeString(&uDeviceSymbolicName,aDeviceSymbolicName);

	ntStatus = IoCreateDevice(pDriverObject,
		0,
		&uDeviceName,
		0x0000800a,
		0,
		TRUE,
		&deviceObject);

	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	ntStatus = PBCDeleteFile(L"\\??\\c:\\haha.doc");
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PBCDeleteFile(\\c:\\haha.doc) faild:%x\n",ntStatus);
		return ntStatus;
	}
	
	ntStatus = PBCDeleteFile(L"\\??\\c:\\PCHunter32.exe");
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PBCDeleteFile(\\c:\\PCHunter32.exe) faild:%x\n",ntStatus);
		return ntStatus;
	}
	

	return STATUS_SUCCESS;
}

NTSTATUS PBCOpenFile(WCHAR *szFilePath,
					 PHANDLE pFileHandle,
					 ACCESS_MASK accessmaskm,
					 ULONG share)
{
	NTSTATUS ntStatus;
	UNICODE_STRING uFilePath = {0x00};
	OBJECT_ATTRIBUTES objFileAttributes = {0x00};
	IO_STATUS_BLOCK ioStatusBlock = {0x00};

	if(KeGetCurrentIrql() > PASSIVE_LEVEL)
		return 0;
	RtlInitUnicodeString(&uFilePath,szFilePath);

	InitializeObjectAttributes(&objFileAttributes,
		&uFilePath,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		NULL);
	
	ntStatus = IoCreateFile(pFileHandle,
		accessmaskm,
		&objFileAttributes,
		&ioStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		share,
		FILE_OPEN,
		0,
		NULL,
		0,
		0,
		NULL,
		IO_NO_PARAMETER_CHECKING);

	return ntStatus;
}

/*
	获取符号链接对应的设备名称
	pSymbolicLinkName:\DosDevice\c:
	pOutPut:接收源
*/
NTSTATUS PBCQuerySymbolicLink(PUNICODE_STRING pSymbolicLinkName,PUNICODE_STRING pOutPut)
{
	NTSTATUS ntStatus;
	OBJECT_ATTRIBUTES objSymbolicLinkAttributes = {0x00};
	HANDLE hDeviceHandle = NULL;
	
	//首先，也要初始化一个文件属性
	InitializeObjectAttributes(&objSymbolicLinkAttributes,
		pSymbolicLinkName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	//获取符号链接对应的设备句柄
	ntStatus = ZwOpenSymbolicLinkObject(&hDeviceHandle,
		GENERIC_READ,
		&objSymbolicLinkAttributes);
	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}
	
	//给外界传来的output分配空间，并设置属性
	pOutPut->Length = 0;
	pOutPut->MaximumLength = 0x400 * sizeof(WCHAR);
	pOutPut->Buffer = ExAllocatePoolWithTag(PagedPool,pOutPut->MaximumLength,'A0');

	if(!pOutPut->Buffer)
	{
		DbgPrint("alloc memory faild \n");
		ZwClose(hDeviceHandle);
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		return ntStatus;
	}

	RtlZeroMemory(pOutPut->Buffer,pOutPut->MaximumLength);

	ntStatus = ZwQuerySymbolicLinkObject(hDeviceHandle,pOutPut,NULL);

	ZwClose(hDeviceHandle);

	//如果查询失败，别忘了释放pOutPut->Buffer
	if(!NT_SUCCESS(ntStatus))
	{
		ExFreePool(pOutPut->Buffer);
	}

	return ntStatus;
	
}

BOOLEAN PBCCloseHandle(WCHAR *szFilePath)
{
	ULONG size;
	ULONG index;
	NTSTATUS ntStatus;
	PVOID buffer = NULL;
	BOOLEAN bRet = FALSE;
	PSYSTEM_HANDLE_INFORMATION handleTable = NULL;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO currentHandleEntryInfo;
	HANDLE currentHandle = NULL;
	ULONG handleNumber = 0;
	UNICODE_STRING uVolume = {0x00};
	UNICODE_STRING uLink = {0x00};
	UNICODE_STRING uLinkName = {0x00};
	WCHAR letterTmp[3] = {0x00};
	WCHAR *aFileName = NULL;
	UNICODE_STRING delFileName = {0x00};
	HANDLE hProcess;
	ULONG uProcessId;
	CLIENT_ID cid;
	OBJECT_ATTRIBUTES objProcessAttributes = {0x00};
	HANDLE hCopyHandle = NULL;
	PVOID fileObject = NULL;
	POBJECT_NAME_INFORMATION pObjectHandleInformation = NULL;
	ULONG oBresultSize;

	//使用这种方式，巧妙的获取全局句柄表的长度，并且获得全局句柄中的数据
	for(size = 1; ; size *= 2)
	{
		buffer = ExAllocatePoolWithTag(PagedPool,size,'CCBP');
		
		if(!buffer)
		{
			DbgPrint("alloc memory faild\n");
			goto ret;
		}
		RtlZeroMemory(buffer,size);
		ntStatus = ZwQuerySystemInformation(SystemHandleInformation,buffer,size,NULL);
		if(!NT_SUCCESS(ntStatus))
		{
			//如果返回的错误码是legth mismatch，则说明传过去的空间不足
			//则结束本次操作，跳出扩大size，重新分配并获取全局句柄表
			if(ntStatus == STATUS_INFO_LENGTH_MISMATCH)
			{
				ExFreePool(buffer);
				buffer = NULL;
			}
			else
			{
				//否则，说明是其他错误，那么就直接返回吧
				DbgPrint("ZwQuerySystemInformation faild:%x\n",ntStatus);
				goto ret;
			}
		}
		else
		{
			//此时，说明获取成功，则结束循环
			break;
		}
	}
	
	handleTable = (PSYSTEM_HANDLE_INFORMATION)buffer;
	handleNumber = handleTable->NumberOfHandles;
	
	//"\??\c:\Dri\xxx.txt"
	letterTmp[0] = szFilePath[4];
	letterTmp[1] = szFilePath[5];
	letterTmp[2] = 0;

	RtlInitUnicodeString(&uVolume,letterTmp);
	RtlInitUnicodeString(&uLink,L"\\DosDevices\\");
	
	uLinkName.Buffer = ExAllocatePoolWithTag(PagedPool,256 + sizeof(WCHAR),'KNIL');
	uLinkName.MaximumLength = 256;

	RtlCopyUnicodeString(&uLinkName,&uLink);

	ntStatus = RtlAppendUnicodeStringToString(&uLinkName,&uVolume);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("RtlAppendUnicodeStringToString faild:%x\n",ntStatus);
		goto ret;
	}
	
	ntStatus = PBCQuerySymbolicLink(&uLinkName,&delFileName);
	RtlFreeUnicodeString(&uLinkName);

	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PBCQuerySymbolicLink faild:%x\n",ntStatus);
		goto ret;
	}
	
	aFileName = &szFilePath[6];
	RtlAppendUnicodeToString(&delFileName,aFileName);
	DbgPrint("delFileName is %wZ\n",&delFileName);
	
	//遍历句柄表
	for(index = 0; index < handleNumber; index++)
	{
		//获取当前句柄表中的结点
		currentHandleEntryInfo = handleTable->Handles[index];
		//获取当前索引号的句柄
		currentHandle = (HANDLE)currentHandleEntryInfo.HandleValue;

		if(currentHandleEntryInfo.ObjectTypeIndex != 25 && currentHandleEntryInfo.ObjectTypeIndex != 28)
		{
			continue;
		}
		//根据当前句柄获取该句柄对应文件的独占进程ID
		uProcessId = (ULONG)currentHandleEntryInfo.UniqueProcessId;

		cid.UniqueProcess = (HANDLE)uProcessId;
		cid.UniqueThread = (HANDLE)0;
		InitializeObjectAttributes(&objProcessAttributes,NULL,0,NULL,NULL);

		ntStatus = ZwOpenProcess(&hProcess,
			PROCESS_DUP_HANDLE,
			&objProcessAttributes,
			&cid);
		if(!NT_SUCCESS(ntStatus))
		{
			DbgPrint("ZwOpenProcess id:%d,faild:%x\n",uProcessId,ntStatus);
			continue;
		}
		
		//第一次拷贝文件句柄，目的是获取该句柄的文件内核对象，从而获取文件内核对象中的文件名
		ntStatus = ZwDuplicateObject(hProcess,currentHandle,NtCurrentProcess(),&hCopyHandle,
			PROCESS_ALL_ACCESS,0,DUPLICATE_SAME_ACCESS);

		if(!NT_SUCCESS(ntStatus))
		{
			DbgPrint("ZwDuplicateObject faild:%x\n",ntStatus);
			continue;
		}

		//根据拷贝过来的被独占的文件句柄，获取该文件的文件内核对象
		ntStatus = ObReferenceObjectByHandle(hCopyHandle,
			FILE_ANY_ACCESS,
			0,
			KernelMode,
			&fileObject,
			NULL);
		if(!NT_SUCCESS(ntStatus))
		{
			ObDereferenceObject(fileObject);
			continue;
		}

		pObjectHandleInformation = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(NonPagedPool,
			1024 * sizeof(WCHAR) + sizeof(OBJECT_NAME_INFORMATION),
			'0C');
		if(!pObjectHandleInformation)
		{
			DbgPrint("alloc memory with pObjectHandleInformation faild\n");
			continue;
		}

		//获取该文件内核对象的OBJECT_HANDLE_INFORMATION
		ntStatus = ObQueryNameString(fileObject,
			pObjectHandleInformation,
			sizeof(OBJECT_NAME_INFORMATION) + 1024 * sizeof(WCHAR),
			&oBresultSize);
		if(!NT_SUCCESS(ntStatus))
		{
			ObDereferenceObject(fileObject);
			continue;
		}
		
		//0表示匹配到了
		if(RtlCompareUnicodeString(&delFileName,&pObjectHandleInformation->Name,TRUE) == 0)
		{
			ObDereferenceObject(fileObject);
			//将第一次拷贝到我们自己进程的文件句柄关闭
			ZwClose(hCopyHandle);
			
			//第二次拷贝，设置DUPLICATE_CLOSE_SOURCE，将原来占有该文件的进程关闭占有属性
			ntStatus = ZwDuplicateObject(hProcess,
				currentHandle,
				NtCurrentProcess(),
				&hCopyHandle,
				PROCESS_ALL_ACCESS,
				0,
				DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
			if(!NT_SUCCESS(ntStatus))
			{
				DbgPrint("ZwDuplicateObject2 failed:%x\n",ntStatus);
			}
			else
			{
				bRet = TRUE;
				ZwClose(hCopyHandle);
			}
			break;
		}
		ExFreePool(pObjectHandleInformation);
		pObjectHandleInformation = NULL;

		ObDereferenceObject(fileObject);
		ZwClose(currentHandle);
		ZwClose(hProcess);

	}

ret:
	if(buffer)
	{
		ExFreePool(buffer);
		buffer = NULL;
	}
	if(uLinkName.Buffer)
	{
		ExFreePool(uLinkName.Buffer);
	}
	if(delFileName.Buffer)
	{
		ExFreePool(delFileName.Buffer);
	}
	if(pObjectHandleInformation)
	{
		ExFreePool(pObjectHandleInformation);
	}
	return bRet;
}

BOOLEAN PBCDeleteFile(WCHAR *szFilePath)
{
	NTSTATUS ntStatus;
	BOOLEAN bRet = FALSE;
	HANDLE hFileHandle = NULL;
	PFILE_OBJECT fileObject = NULL;
	PDEVICE_OBJECT fileDeviceObject = NULL;
	PIRP customIrp = NULL;
	FILE_DISPOSITION_INFORMATION fileDisInformation;
	KEVENT event;
	IO_STATUS_BLOCK ioStatusBlock;
	PIO_STACK_LOCATION customIrpStack = NULL;
	PSECTION_OBJECT_POINTERS pSectionObjectPointers = NULL;

	//这里，设置openfile的ACCESS_MASK类型参数时，将FILE_READ_ACCESS传进去了
	//导致在关闭句柄时，拷贝目标文件的独占进程句柄时，会提示0c000022，STATUS_ACCESS_DENIED
	ntStatus = PBCOpenFile(szFilePath,
		&hFileHandle,
		FILE_READ_ATTRIBUTES | DELETE,
		FILE_SHARE_DELETE);
	
	if(!NT_SUCCESS(ntStatus))
	{
		if(ntStatus == STATUS_OBJECT_NAME_NOT_FOUND ||
			ntStatus == STATUS_OBJECT_PATH_NOT_FOUND)
		{
			DbgPrint("no such file\n");
			goto ret;
		}
		else
		{
			bRet = PBCCloseHandle(szFilePath);
			if(!bRet)
			{
				goto ret;
			}
			//重新打开
			bRet = PBCOpenFile(szFilePath,&hFileHandle,FILE_READ_ATTRIBUTES | DELETE | FILE_WRITE_ACCESS,
				FILE_SHARE_READ | FILE_SHARE_WRITE |FILE_SHARE_DELETE);
			if(!bRet)
			{
				goto ret;
			}

		}
	}

	//说明独占的文件已经被我们获取到，并关闭了独占的属性
	//接着，获取该文件的文件内核对象
	ntStatus = ObReferenceObjectByHandle(hFileHandle,
		DELETE,
		*IoFileObjectType,
		KernelMode,
		&fileObject,
		NULL);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ObReferenceObjectByHandle faild:%x\n",ntStatus);
		ZwClose(hFileHandle);
		return FALSE;
	}

	//根据文件内核对象fileobject获取对应的设备对象deviceobject
	fileDeviceObject = IoGetRelatedDeviceObject(fileObject);
	//初始化irp
	customIrp = IoAllocateIrp(fileDeviceObject->StackSize,TRUE);
	
	if(!customIrp)
	{
		ObDereferenceObject(fileObject);
		ZwClose(hFileHandle);
		goto ret;
	}

	fileDisInformation.DeleteFile = TRUE;
	
	//初始化一个事件
	//&&**&&%……&%￥%￥%……*&……&*……
	KeInitializeEvent(&event,SynchronizationEvent,FALSE);

	//对自定义Irp进行属性设置
	//设置irp的dobuffer io模式的buffer
	customIrp->AssociatedIrp.SystemBuffer = &fileDisInformation;
	customIrp->Tail.Overlay.OriginalFileObject = fileObject;
	customIrp->Tail.Overlay.Thread = NtCurrentThread();
	customIrp->UserEvent = &event;
	customIrp->UserIosb = &ioStatusBlock;
	customIrp->RequestorMode = KernelMode;

	//得到irp的栈
	customIrpStack = IoGetNextIrpStackLocation(customIrp);
	customIrpStack->DeviceObject = fileDeviceObject;
	customIrpStack->FileObject = fileObject;
	customIrpStack->MajorFunction = IRP_MJ_SET_INFORMATION;
	customIrpStack->Parameters.SetFile.Length = sizeof(FILE_DISPOSITION_INFORMATION);
	customIrpStack->Parameters.SetFile.FileInformationClass = FileDispositionInformation;
	customIrpStack->Parameters.SetFile.FileObject = fileObject;
	

	IoSetCompletionRoutine(customIrp,
		PBCSkillFileCompleation,
		&event,
		TRUE,
		TRUE,
		TRUE);
	
	//对一个正在运行的exe，将关键属性抹掉，删除时系统就不会拒绝了
	pSectionObjectPointers = fileObject->SectionObjectPointer;
	if(pSectionObjectPointers)
	{
		pSectionObjectPointers->ImageSectionObject = 0;
		pSectionObjectPointers->DataSectionObject = 0;
	}

	ntStatus = IoCallDriver(fileDeviceObject,customIrp);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoCallDriver faild:%x\n",ntStatus);
		goto ret;
	}

	//接着，就是等待event执行结束了
	KeWaitForSingleObject(&event,
		Executive,
		KernelMode,
		TRUE,
		NULL);

	ObDereferenceObject(fileObject);
	ZwClose(hFileHandle);


ret:
	return bRet;
}

NTSTATUS
NTAPI
PBCSkillFileCompleation(PDEVICE_OBJECT pDeviceObject,
						PIRP irp,
						PVOID context)
{
	irp->UserIosb->Status = irp->IoStatus.Status;
	irp->UserIosb->Information = irp->IoStatus.Information;

	KeSetEvent(irp->UserEvent,IO_NO_INCREMENT,FALSE);

	IoFreeIrp(irp);

	return STATUS_MORE_PROCESSING_REQUIRED;
}