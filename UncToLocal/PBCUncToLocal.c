#include <ntddk.h>
#include <windef.h>

NTSTATUS PBCUncToLocal(__in PUNICODE_STRING,__out PUNICODE_STRING);

WCHAR *PBCUnicodeStringChar(PUNICODE_STRING,WCHAR);

WCHAR *PBCUnicodeStrStr(PUNICODE_STRING,PUNICODE_STRING);

NTSTATUS DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("PBCUncToLocal:goodbye!\n");

	return STATUS_SUCCESS;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING pDriverPath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	DECLARE_UNICODE_STRING_SIZE(uSharePath,MAX_PATH + 64);
	DECLARE_UNICODE_STRING_SIZE(uLocalPath,MAX_PATH + 64);
	//UNICODE_STRING uSharePath = {0x00};
	//UNICODE_STRING uLocalPath = {0x00};
	
	DbgPrint("PBCUncToLocal:hello\n");
	pDriverObject->DriverUnload = DriverUnload;
	
	RtlInitUnicodeString(&uSharePath,L"12\\p0sixB1ackcatDeleteFile.sys");

	ntStatus = PBCUncToLocal(&uSharePath,&uLocalPath);
	
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("转换失败,errorCode:%x\n",ntStatus);
	}
	else
	{
		DbgPrint("%wZ ---> %wZ",&uSharePath,&uLocalPath);
	}

	return STATUS_SUCCESS;
}

NTSTATUS PBCUncToLocal(__in PUNICODE_STRING uPsharePath,__out PUNICODE_STRING uPlocalPath)
{
	NTSTATUS ntStatus;
	WCHAR *pTmp;
	//UNICODE_STRING  uShareNameStr;
	//UNICODE_STRING uFileNameStr;
	UNICODE_STRING uRegTableName = {0x00};
	DECLARE_UNICODE_STRING_SIZE(uFileNameStr,MAX_PATH + 64);
	DECLARE_UNICODE_STRING_SIZE(uShareNameStr,MAX_PATH + 64);
	OBJECT_ATTRIBUTES objRegTableAttributes = {0x00};
	HANDLE hRegTableHandle = NULL;
	ULONG resultSize;
	PKEY_VALUE_PARTIAL_INFORMATION pKeyValuesBuffer = NULL;
	UNICODE_STRING uKeyValueStr = {0x00};

	//12\\p0sixB1ackcatDeleteFile.sys
	//12称为共享路径名，p0six...sys称为文件名
	//通过pTmp指针，将共享路径名和文件名拆分出来

	pTmp = PBCUnicodeStringChar(uPsharePath,'\\');

	//在共享路径中获取共享路径名，也就是共享文件的所在目录路径
	uShareNameStr.Length = (USHORT)((ULONG)pTmp - (ULONG)uPsharePath->Buffer);
	RtlCopyMemory(uShareNameStr.Buffer,uPsharePath->Buffer,uShareNameStr.Length);

	//文件名的长度当然是总的长度减去前面的路径长度了
	uFileNameStr.Length = (USHORT)((ULONG)uPsharePath->Length - uShareNameStr.Length);
	RtlCopyMemory(uFileNameStr.Buffer,pTmp,uFileNameStr.Length);

	//然后，打开注册表相关位置...
	RtlInitUnicodeString(&uRegTableName,L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Shares");

	InitializeObjectAttributes(&objRegTableAttributes,
		&uRegTableName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	ntStatus = ZwOpenKey(&hRegTableHandle,
		KEY_ALL_ACCESS,
		&objRegTableAttributes);
	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	//打开之后，根据文件名，去注册表的目录项\\..\\shares中，查找文件名对应的valueKey
	//2次查询，第一次获取实际output长度，第二次才是真的获取内容
	ntStatus = ZwQueryValueKey(hRegTableHandle,
		&uShareNameStr,
		KeyValuePartialInformation,
		NULL,
		0,
		&resultSize);

	if(ntStatus != STATUS_BUFFER_OVERFLOW && ntStatus != STATUS_BUFFER_TOO_SMALL && !NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	pKeyValuesBuffer = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool,resultSize,'VK');
	if(!pKeyValuesBuffer)
	{
		DbgPrint("alloc memory faild\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	ntStatus = ZwQueryValueKey(hRegTableHandle,
		&uShareNameStr,
		KeyValuePartialInformation,
		pKeyValuesBuffer,
		resultSize,
		&resultSize);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ZwQueryValueKey2 faild\n");
		ExFreePool(pKeyValuesBuffer);
		return ntStatus;
	}

	if(pKeyValuesBuffer->Type != REG_MULTI_SZ)
	{
		DbgPrint("pKeyValueBuffer->type is not REG_MULTI_SZ\n");
		ExFreePool(pKeyValuesBuffer);
		ZwClose(hRegTableHandle);
		return !STATUS_SUCCESS;
	}


	//这里，就会用到一个比较精华型的字符串算法了，将本地路径名从这个valueKey多字符串中抽取出来
	/*
		比如：
		CSCFlags=2048
		MaxUses=4294967295
		Path=C:\12
		Permissions=0
		Remark=
		ShareName=12
		Type=0
		
	*/
	//将上面字符串中C:\12提取出来，注意，不可硬编码

	uKeyValueStr.Buffer = (WCHAR *)pKeyValuesBuffer->Data;
	uKeyValueStr.MaximumLength = uKeyValueStr.Length = (USHORT)pKeyValuesBuffer->DataLength;

	RtlInitUnicodeString(&uShareNameStr,L"path=");

	//返回path=\\.....的首地址
	pTmp = PBCUnicodeStrStr(&uKeyValueStr,&uShareNameStr);

	if(!pTmp)
	{
		ExFreePool(pKeyValuesBuffer);
		ZwClose(hRegTableHandle);
		return !STATUS_SUCCESS;
	}

	//要记住多字符串的规则："str1\nstr2\nstr3\0\0"
	//获取本地路径，不包含文件名
	//这里，"path="前要加L
	RtlInitUnicodeString(&uShareNameStr,pTmp + wcslen(L"path="));

	//将本地路径名和文件名进行拼接
	RtlCopyUnicodeString(uPlocalPath,&uShareNameStr);
	RtlAppendUnicodeStringToString(uPlocalPath,&uFileNameStr);

	ExFreePool(pKeyValuesBuffer);
	ZwClose(hRegTableHandle);
	ntStatus = STATUS_SUCCESS;

	return ntStatus;

}

WCHAR *PBCUnicodeStringChar(PUNICODE_STRING uSourceStr,WCHAR referenceCh)
{
	//长度别忘了，字符串中的每个字符占2个字节
	//所以，这里遍历他的长度的一半就好了
	ULONG souSize = uSourceStr->Length >> 1;
	ULONG index;

	for(index = 0; index < souSize; index++)
	{
		if(*(uSourceStr->Buffer+index) == referenceCh)
		{
			return uSourceStr->Buffer + index;
		}
	}
	return NULL;

}

WCHAR *PBCUnicodeStrStr(PUNICODE_STRING uSource,PUNICODE_STRING uReferenceStr)
{
	ULONG loopNums;
	ULONG index;
	UNICODE_STRING uStr1;
	UNICODE_STRING uStr2;

	if(uSource->Length < uReferenceStr->Length)
	{
		return NULL;
	}

	loopNums = ((uSource->Length - uReferenceStr->Length) >> 1) + 1;

	for(index = 0; index < loopNums; index++)
	{
		uStr1.MaximumLength = uStr1.Length = uReferenceStr->Length;
		uStr2.MaximumLength = uStr2.Length = uReferenceStr->Length;
		uStr1.Buffer = uSource->Buffer + index;
		uStr2.Buffer = uReferenceStr->Buffer;
		
		if(RtlCompareUnicodeString(&uStr1,&uStr2,TRUE) == 0)
		{
			return uSource->Buffer + index;
		}

	}

	return NULL;


}