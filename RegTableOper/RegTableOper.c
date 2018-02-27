#include <ntddk.h>

NTSTATUS PBCRegTableOper(VOID);

NTSTATUS PBCCreateKey(WCHAR *);

NTSTATUS PBCDeleteKey(WCHAR *);

NTSTATUS PBCDeleteValueKey(WCHAR *);

NTSTATUS PBCSetValueKey(WCHAR *);

NTSTATUS PBCQueryValueKey(WCHAR *);

NTSTATUS PBCEnumerateSubValueKey(WCHAR *);

NTSTATUS PBCEnumateSubKey(WCHAR *);

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS ioStatus;

	ioStatus = PBCDeleteValueKey(L"\\Registry\\Machine\\SoftWare\\p0sixB1ackcat");

	if(!NT_SUCCESS(ioStatus))
	{
		DbgPrint("PBCDeleteValueKey failed:%x\n",ioStatus);
	}

	ioStatus = PBCDeleteKey(L"\\Registry\\Machine\\SoftWare\\p0sixB1ackcat");

	if(!NT_SUCCESS(ioStatus))
	{
		DbgPrint("PBCDeleteKey failed:%x\n",ioStatus);
	}
	
	DbgPrint("p0sixB1ackcat RegTableOper exit!\n");
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING uPath)
{
	NTSTATUS ioStatus = STATUS_SUCCESS;
	pDriverObject->DriverUnload = DriverUnload;

	DbgPrint("p0sixB1ackcat RegTableOper start!\n");

	PBCRegTableOper();
	return ioStatus;

}

NTSTATUS PBCRegTableOper(VOID)
{
	WCHAR *szRegTableRootKeyPath = L"\\Registry\\Machine\\SoftWare\\p0sixB1ackcat";

	NTSTATUS ioStatus;

	ioStatus = PBCCreateKey(szRegTableRootKeyPath);
	if(!NT_SUCCESS(ioStatus))
	{
		DbgPrint("PBCCreateKey failed:%d\n",ioStatus);
		return ioStatus;
	}

	ioStatus = PBCSetValueKey(szRegTableRootKeyPath);
	if(!NT_SUCCESS(ioStatus))
	{
		DbgPrint("PBCSetValueKey failed:%x\n",ioStatus);
		return ioStatus;
	}

	ioStatus = PBCQueryValueKey(szRegTableRootKeyPath);
	if(!NT_SUCCESS(ioStatus))
	{
		DbgPrint("PBCQueryValueKey falied:%x\n",ioStatus);
		return ioStatus;
	}

	ioStatus = PBCEnumerateSubValueKey(szRegTableRootKeyPath);
	if(!NT_SUCCESS(ioStatus))
	{
		DbgPrint("PBCEnumateSubValueKey failed:%x\n",ioStatus);
		return ioStatus;
	}

	ioStatus = PBCEnumateSubKey(szRegTableRootKeyPath);
	if(!NT_SUCCESS(ioStatus))
	{
		DbgPrint("PBCEnumateSubKey failed:%x\n",ioStatus);
		return ioStatus;
	}

	return ioStatus;

}

NTSTATUS PBCCreateKey(WCHAR *szKeyPath)
{
	NTSTATUS ioStatus;
	HANDLE hRootKeyHandle = NULL;
	UNICODE_STRING uKeyPath = {0x00};
	OBJECT_ATTRIBUTES objRootKey = {0x00};
	UNICODE_STRING uSubKeyPath = {0x00};
	OBJECT_ATTRIBUTES objSubKey = {0x00};
	HANDLE hSubKeyHandle = NULL;
	ULONG result;

	RtlInitUnicodeString(&uKeyPath,szKeyPath);

	InitializeObjectAttributes(&objRootKey,
		&uKeyPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	ioStatus = ZwCreateKey(&hRootKeyHandle,
		KEY_ALL_ACCESS,
		&objRootKey,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		&result);

	if(!NT_SUCCESS(ioStatus))
	{
		return ioStatus;
	}

	RtlInitUnicodeString(&uSubKeyPath,L"KernelDriver");

	//这里在ntdef.h里始终不能确定这个宏函数参数，so，查windows kit document!
	/*
	VOID 
	InitializeObjectAttributes(
	OUT POBJECT_ATTRIBUTES  InitializedAttributes,
	IN PUNICODE_STRING  ObjectName,
	IN ULONG  Attributes,
	IN HANDLE  RootDirectory,
	IN PSECURITY_DESCRIPTOR  SecurityDescriptor
	);
	*/
	//倒数第二个参数可以设置文件的上层目录，注册表也是通用的
	InitializeObjectAttributes(&objSubKey,
		&uSubKeyPath,
		OBJ_CASE_INSENSITIVE,
		hRootKeyHandle,
		NULL);

	ioStatus = ZwCreateKey(&hSubKeyHandle,
		KEY_ALL_ACCESS,
		&objSubKey,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		&result);

	if(!NT_SUCCESS(ioStatus))
	{
		ZwClose(hRootKeyHandle);
		return ioStatus;
	}

	ZwClose(hRootKeyHandle);
	ZwClose(hSubKeyHandle);
	return ioStatus;

}

NTSTATUS PBCDeleteKey(WCHAR *szKeyPath)
{
	NTSTATUS ioStatus;
	UNICODE_STRING uKeyPath = {0x00};
	OBJECT_ATTRIBUTES objKeyAttributes = {0x00};
	HANDLE hKeyHandle = NULL;
	UNICODE_STRING uSubKeyPath = {0x00};
	OBJECT_ATTRIBUTES objSubAttributes = {0x00};
	HANDLE hSubKeyHandle = NULL;

	RtlInitUnicodeString(&uKeyPath,szKeyPath);

	InitializeObjectAttributes(&objKeyAttributes,
		&uKeyPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	//和文件操作一样，操作前都要先打开文件
	ioStatus = ZwOpenKey(&hKeyHandle,
		KEY_ALL_ACCESS,
		&objKeyAttributes);


	if(!NT_SUCCESS(ioStatus))
	{
		return ioStatus;
	}

	ioStatus = ZwDeleteKey(hKeyHandle);
	
	//如果删除失败，表示当前目录下还有子目录，所以要将里面的子目录删除
	//这里只有一层，且只有一个目录，删除失败仅仅当做该目录下还包含子目录的情况
	if(!NT_SUCCESS(ioStatus))
	{
		RtlInitUnicodeString(&uSubKeyPath,L"KernelDriver");

		InitializeObjectAttributes(&objSubAttributes,
			&uSubKeyPath,
			OBJ_CASE_INSENSITIVE,
			hKeyHandle,
			NULL);

		ioStatus = ZwOpenKey(&hSubKeyHandle,
			KEY_ALL_ACCESS,
			&objSubAttributes);

		if(!NT_SUCCESS(ioStatus))
		{
			return ioStatus;
		}
		ioStatus = ZwDeleteKey(hSubKeyHandle);
		if(!NT_SUCCESS(ioStatus))
		{
			goto ret;
		}
	}
	if(!NT_SUCCESS(ioStatus))
	{
		goto ret;
	}

	ioStatus = ZwDeleteKey(hKeyHandle);

ret:
	ZwClose(hKeyHandle);
	ZwClose(hSubKeyHandle);
	return ioStatus;
}

NTSTATUS PBCDeleteValueKey(WCHAR *szKeyPath)
{
	UNICODE_STRING ukeyPath = {0x00};
	HANDLE hKeyHandle = NULL;
	UNICODE_STRING uValueKeyName = {0x00};
	OBJECT_ATTRIBUTES objKeyAttributes = {0x00};
	NTSTATUS ntStatus;

	RtlInitUnicodeString(&ukeyPath,szKeyPath);

	InitializeObjectAttributes(&objKeyAttributes,
		&ukeyPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	ntStatus = ZwOpenKey(&hKeyHandle,
		KEY_ALL_ACCESS,
		&objKeyAttributes);
	
	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	RtlInitUnicodeString(&uValueKeyName,L"REG_SZ");

	ntStatus = ZwDeleteValueKey(hKeyHandle,&uValueKeyName);

	ZwClose(hKeyHandle);
	return ntStatus;

}



NTSTATUS PBCSetValueKey(WCHAR *szKeyPath)
{
	HANDLE hKeyHandle = NULL;
	UNICODE_STRING uKeyPath = {0x00};
	OBJECT_ATTRIBUTES objKeyAttributes = {0x00};
	NTSTATUS ntStatus;
	UNICODE_STRING uValuePath = {0x00};
	OBJECT_ATTRIBUTES objValueAttributes = {0x00};
	ULONG uValue;
	WCHAR *szStr = L"sub key string";
	PVOID buffer = NULL;

	RtlInitUnicodeString(&uKeyPath,szKeyPath);
	
	InitializeObjectAttributes(&objKeyAttributes,
		&uKeyPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	//首先要获取注册表中目标根目录的路径句柄
	ntStatus = ZwOpenKey(&hKeyHandle,
		KEY_ALL_ACCESS,
		&objKeyAttributes);

	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}
	
	RtlInitUnicodeString(&uValuePath,L"REG_DWORD");

	InitializeObjectAttributes(&objValueAttributes,
		&uValuePath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	uValue = 0x400;
	ntStatus = ZwSetValueKey(hKeyHandle,
		&uValuePath,
		0,
		REG_DWORD,
		&uValue,
		sizeof(uValue));

	RtlInitUnicodeString(&uValuePath,L"REG_SZ");
	
	ntStatus = ZwSetValueKey(hKeyHandle,
		&uValuePath,
		0,
		REG_SZ,
		(PVOID)szStr,
		wcslen(szStr) * sizeof(WCHAR) + sizeof(WCHAR));

	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	buffer = ExAllocatePoolWithTag(PagedPool,1024,"LOOC");
	if(!buffer)
	{
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		return ntStatus;
	}

	//buff是指针，不能用sizeof计算他指向内容的长度了。
	//RtlFillMemory(buffer,sizeof(buffer),0xff);
	RtlFillMemory(buffer,1024,0xff);

	RtlInitUnicodeString(&uValuePath,L"REG_BINARY");

	ntStatus = ZwSetValueKey(hKeyHandle,
		&uValuePath,
		0,
		REG_BINARY,
		buffer,
		1024);
	
	ExFreePool(buffer);
	buffer = NULL;
	ZwClose(hKeyHandle);

	return ntStatus;
	
}


NTSTATUS PBCQueryValueKey(WCHAR *szKeyPath)
{
	UNICODE_STRING uKeyPath = {0x00};
	OBJECT_ATTRIBUTES objKeyPathAttributes = {0x00};
	NTSTATUS ntStatus;
	HANDLE hKeyPathHandle = NULL;
	UNICODE_STRING uValueName = {0x00};
	ULONG resultSize;
	ULONG uValue;
	PKEY_VALUE_PARTIAL_INFORMATION outputBuffer = NULL;

	RtlInitUnicodeString(&uKeyPath,szKeyPath);

	InitializeObjectAttributes(&objKeyPathAttributes,
		&uKeyPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	ntStatus = ZwOpenKey(&hKeyPathHandle,
		KEY_ALL_ACCESS,
		&objKeyPathAttributes);

	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	RtlInitUnicodeString(&uValueName,L"REG_DWORD");

	ntStatus = ZwQueryValueKey(hKeyPathHandle,
		&uValueName,
		KeyValuePartialInformation,
		NULL,
		0,
		&resultSize);

	if(ntStatus != STATUS_BUFFER_OVERFLOW &&
		ntStatus != STATUS_BUFFER_TOO_SMALL)
	{
		ZwClose(hKeyPathHandle);
		return ntStatus;
	}

	outputBuffer = ExAllocatePoolWithTag(PagedPool,resultSize,"DAER");
	if(!outputBuffer)
	{
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		ZwClose(hKeyPathHandle);
		return ntStatus;
	}

	ntStatus = ZwQueryValueKey(hKeyPathHandle,
		&uValueName,
		KeyValuePartialInformation,
		outputBuffer,
		resultSize,
		&resultSize);

	if(!NT_SUCCESS(ntStatus))
	{
		ExFreePool(outputBuffer);
		ZwClose(hKeyPathHandle);
		return ntStatus;
	}

	if(outputBuffer->Type == REG_DWORD && outputBuffer->DataLength == sizeof(ULONG))
	{
		DbgPrint("one value is %x\n",outputBuffer->Data);
	}

	ExFreePool(outputBuffer);

	RtlInitUnicodeString(&uValueName,L"REG_SZ");

	ntStatus = ZwQueryValueKey(hKeyPathHandle,
		&uValueName,
		KeyValuePartialInformation,
		NULL,
		0,
		&resultSize);
	
	if(ntStatus == STATUS_OBJECT_NAME_NOT_FOUND || resultSize == 0)
	{
		ZwClose(hKeyPathHandle);
		return ntStatus;
	}
	
	outputBuffer = ExAllocatePoolWithTag(PagedPool,resultSize,"DAER");

	if(!outputBuffer)
	{
		ZwClose(hKeyPathHandle);
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		return ntStatus;
	}
	
	ntStatus = ZwQueryValueKey(hKeyPathHandle,
		&uValueName,
		KeyValuePartialInformation,
		outputBuffer,
		resultSize,
		&resultSize);
	if(!NT_SUCCESS(ntStatus))
	{
		ExFreePool(outputBuffer);
		ZwClose(hKeyPathHandle);
		return ntStatus;
	}

	if(outputBuffer->Type == REG_SZ)
	{
		UNICODE_STRING tmp;
		tmp.Length = outputBuffer->DataLength;
		tmp.Buffer = outputBuffer->Data;
		//这里如果不写，&tmp的方式不能访问字符串
		tmp.MaximumLength = outputBuffer->DataLength;
		DbgPrint("tmp is %wZ\n",&tmp);
		DbgPrint("outputBuffer->data is %S\n",outputBuffer->Data);
	}

	ExFreePool(outputBuffer);
	outputBuffer = NULL;
	ZwClose(hKeyPathHandle);
	return ntStatus;

}

NTSTATUS PBCEnumerateSubValueKey(WCHAR *szKeyPath)
{
	UNICODE_STRING uKeyPath = {0x00};
	UNICODE_STRING uValueName = {0x00};
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES objKeyPathAttributes = {0x00};
	HANDLE hKeyPathHandle = NULL;
	NTSTATUS ntStatus;
	ULONG resultSize;
	PKEY_FULL_INFORMATION pFullInfoBuffer = NULL;
	PKEY_VALUE_BASIC_INFORMATION pBasicInfoBuffer = NULL;
	ULONG index;

	RtlInitUnicodeString(&uKeyPath,szKeyPath);

	InitializeObjectAttributes(&objKeyPathAttributes,
		&uKeyPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	ntStatus = ZwOpenKey(&hKeyPathHandle,
		KEY_ALL_ACCESS,
		&objKeyPathAttributes);

	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}
	
	//第一次查询，长度为0,表示获取该数据的实际长度
	//因为函数内部会将该数据的实际长度存放在最后一个参数中
	ntStatus = ZwQueryKey(hKeyPathHandle,
		KeyFullInformation,
		NULL,
		0,
		&resultSize);

	//上面这种查询方式，返回的定不是STATUS_SUCCESS，所以不能用nt_SUCCESS来判断
	if(ntStatus != STATUS_BUFFER_OVERFLOW &&
		ntStatus != STATUS_BUFFER_TOO_SMALL)
	{
		ZwClose(hKeyPathHandle);
		return ntStatus;
	}
	
	pFullInfoBuffer = ExAllocatePoolWithTag(PagedPool,resultSize,"SGER");

	if(!pFullInfoBuffer)
	{
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		ZwClose(hKeyPathHandle);
		return ntStatus;
	}

	ntStatus = ZwQueryKey(hKeyPathHandle,
		KeyFullInformation,
		pFullInfoBuffer,
		resultSize,
		&resultSize);
	
	if(!NT_SUCCESS(ntStatus))
	{
		ZwClose(hKeyPathHandle);
		ExFreePool(pFullInfoBuffer);
		return ntStatus;
	}

	for(index = 0; index < pFullInfoBuffer->Values;index++)
	{
		ntStatus = ZwEnumerateValueKey(hKeyPathHandle,
			index,
			KeyValueBasicInformation,
			NULL,
			0,
			&resultSize);

		if(ntStatus != STATUS_BUFFER_OVERFLOW &&
			ntStatus != STATUS_BUFFER_TOO_SMALL)
		{
			ZwClose(hKeyPathHandle);
			ExFreePool(pFullInfoBuffer);
			return ntStatus;
		}

		pBasicInfoBuffer = ExAllocatePoolWithTag(PagedPool,resultSize,"SGER");

		if(!pBasicInfoBuffer)
		{
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			ZwClose(hKeyPathHandle);
			ExFreePool(pFullInfoBuffer);
			return ntStatus;
		}

		ntStatus = ZwEnumerateValueKey(hKeyPathHandle,
			index,
			KeyValueBasicInformation,
			pBasicInfoBuffer,
			resultSize,
			&resultSize);

		if(!NT_SUCCESS(ntStatus))
		{
			ZwClose(hKeyPathHandle);
			ExFreePool(pFullInfoBuffer);
			ExFreePool(pBasicInfoBuffer);
			return ntStatus;
		}

		
		uStr.Length = uStr.MaximumLength = pBasicInfoBuffer->NameLength;
		uStr.Buffer = pBasicInfoBuffer->Name;
		
		DbgPrint("sub key is %wZ,",&uStr);
		switch(pBasicInfoBuffer->Type)
		{
			case REG_SZ:
				DbgPrint("type is REG_SZ\n");
				break;
			case REG_DWORD:
				DbgPrint("type is REG_DWORD\n");
				break;
			case REG_BINARY:
				DbgPrint("type is REG_BINARY\n");
				break;
			case REG_MULTI_SZ:
				DbgPrint("type is REG_MULTI_SZ\n");
				break;
		}

	}

	ZwClose(hKeyPathHandle);
	ExFreePool(pFullInfoBuffer);
	ExFreePool(pBasicInfoBuffer);
	return ntStatus;

}

NTSTATUS PBCEnumateSubKey(WCHAR *szKeyPath)
{
	UNICODE_STRING uKeyPath = {0x00};
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES objKeyPathAttributes = {0x00};
	NTSTATUS ntStatus;
	HANDLE hKeyHandle = NULL;
	PKEY_FULL_INFORMATION pFullBuffer = NULL;
	PKEY_BASIC_INFORMATION pInfoBuffer = NULL;
	ULONG resultSize;
	ULONG index;

	RtlInitUnicodeString(&uKeyPath,szKeyPath);

	InitializeObjectAttributes(&objKeyPathAttributes,
		&uKeyPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	ntStatus = ZwOpenKey(&hKeyHandle,
		KEY_ALL_ACCESS,
		&objKeyPathAttributes);

	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}
	
	ntStatus = ZwQueryKey(hKeyHandle,
		KeyFullInformation,
		NULL,
		0,
		&resultSize);

	if(ntStatus != STATUS_BUFFER_OVERFLOW &&
		ntStatus != STATUS_BUFFER_TOO_SMALL)
	{
		ZwClose(hKeyHandle);
		return ntStatus;
	}

	pFullBuffer = ExAllocatePoolWithTag(PagedPool,resultSize,"SGER");

	if(!pFullBuffer)
	{
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		ZwClose(hKeyHandle);
		return ntStatus;
	}

	ntStatus = ZwQueryKey(hKeyHandle,
		KeyFullInformation,
		pFullBuffer,
		resultSize,
		&resultSize);

	if(!NT_SUCCESS(ntStatus))
	{
		ZwClose(hKeyHandle);
		ExFreePool(pFullBuffer);
		return ntStatus;
	}

	for(index = 0; index < pFullBuffer->SubKeys; index++)
	{
		ntStatus = ZwEnumerateKey(hKeyHandle,
			index,
			KeyBasicInformation,
			NULL,
			0,
			&resultSize);
		
		if(ntStatus != STATUS_BUFFER_OVERFLOW &&
			ntStatus != STATUS_BUFFER_TOO_SMALL)
		{
			ZwClose(hKeyHandle);
			ExFreePool(pFullBuffer);
			return ntStatus;
		}

		pInfoBuffer = ExAllocatePoolWithTag(PagedPool,resultSize,"SGER");

		if(!pInfoBuffer)
		{
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			ZwClose(hKeyHandle);
			ExFreePool(pFullBuffer);
			return ntStatus;
		}

		ntStatus = ZwEnumerateKey(hKeyHandle,
			0,
			KeyBasicInformation,
			pInfoBuffer,
			resultSize,
			&resultSize);

		if(!NT_SUCCESS(ntStatus))
		{
			ZwClose(hKeyHandle);
			ExFreePool(pFullBuffer);
			ExFreePool(pInfoBuffer);
			return ntStatus;
		}

		uStr.Length = uStr.MaximumLength = pInfoBuffer->NameLength;
		uStr.Buffer = pInfoBuffer->Name;

		DbgPrint("sub key name is %wZ\n",&uStr);

	}

	ZwClose(hKeyHandle);
	ExFreePool(pFullBuffer);
	ExFreePool(pInfoBuffer);
	return ntStatus;

}