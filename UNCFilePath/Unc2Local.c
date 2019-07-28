/*

实现Unc路径 转换成本地路径。
UNC (Universal Naming Convention)  通用命名规则
\\servername\sharename\directory\filename
SharedDocs\\hi.txt  ->  D:\\Docs\\hi.txt
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\LanmanServer\Shares

by kyle
*/

//#define MyPrint
#define MyPrint DbgPrint

#include <ntifs.h>
#include <ntimage.h>
#include <string.h>
#include <ntstrsafe.h>

#include <wdm.h>

#include <ntddk.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define SAFE_MAX_PATH  (MAX_PATH+64)

#define MEM_TAG		('kyle')

VOID UnloadDriver(PDRIVER_OBJECT pDriverObject)
{

}
//实现 strchr功能
WCHAR * RtlUnicodeStringChr(PUNICODE_STRING IN pStr, WCHAR chr);
//实现strstr功能
WCHAR * RtlUnicodeStringStr(PUNICODE_STRING IN pStr, PUNICODE_STRING pStrDst);


//SharedDocs\\111.txt -> c:\\document and settings\\ Documents\\111.txt
//这里木有校验文件是否存在 只是根据注册表转换路径
NTSTATUS Unc2Local(PUNICODE_STRING IN pstrUnc, PUNICODE_STRING OUT pstrLocal);

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pReg)
{
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	DECLARE_UNICODE_STRING_SIZE(strUnc, SAFE_MAX_PATH);
	DECLARE_UNICODE_STRING_SIZE(strLocal, SAFE_MAX_PATH);
	RtlInitUnicodeString(&strUnc, L"12\\UNCFilePath.sys");
	if (STATUS_SUCCESS == Unc2Local(&strUnc, &strLocal))
	{
		MyPrint("本地路径:%wZ \n", &strLocal);
	}
	else
	{
		MyPrint("出错鸟。\n");
	}

	pDriverObject->DriverUnload = UnloadDriver;
	return STATUS_SUCCESS;
}

WCHAR * RtlUnicodeStringChr(PUNICODE_STRING IN pStr, WCHAR chr)
{
	ULONG i = 0;
	ULONG uSize = pStr->Length >> 1;

	for (i=0; i<uSize; i++)
	{
		if (pStr->Buffer[i] == chr)
		{
			return pStr->Buffer + i;
		}
	}

	return NULL;
}

WCHAR * RtlUnicodeStringStr(PUNICODE_STRING IN pSource, PUNICODE_STRING IN pStrDst)
{
	ULONG i = 0;
	ULONG uLengthSetp = 0;
	ULONG uLengthSrc = 0;
	ULONG uLengthDst = 0;
	UNICODE_STRING str1 = {0};
	UNICODE_STRING str2 = {0};

	uLengthSrc = pSource->Length;
	uLengthDst = pStrDst->Length;
	
	if (uLengthSrc < uLengthDst)
	{
		return NULL;
	}
	
	uLengthSetp = ((uLengthSrc - uLengthDst) >> 1) + 1;
	for (i=0; i<uLengthSetp; i++)
	{
		str1.Length = str1.MaximumLength = (USHORT)uLengthDst;
		str2.Length = str2.MaximumLength = (USHORT)uLengthDst;
		str1.Buffer = pSource->Buffer+i;
		str2.Buffer = pStrDst->Buffer;

		if ( 0 == RtlCompareUnicodeString(&str1, &str2, TRUE))
		{
			return pSource->Buffer + i;
		}
	}
	return NULL;
}


//SharedDocs\\111.txt -> C:\\Documents and Settings\\All Users\\Documents\\111.txt
NTSTATUS Unc2Local(PUNICODE_STRING IN pstrUnc, PUNICODE_STRING OUT pstrLocal)
{
	NTSTATUS						status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES		objectAttr = {0};
	HANDLE							hRegister = NULL;
	UNICODE_STRING			ustrReg = {0};
	ULONG							uResult = 0;
	WCHAR							*pTmp = NULL;
	DECLARE_UNICODE_STRING_SIZE(strShare, SAFE_MAX_PATH);
	DECLARE_UNICODE_STRING_SIZE(strName, SAFE_MAX_PATH);
	PKEY_VALUE_PARTIAL_INFORMATION pkpi = NULL;
	pTmp = RtlUnicodeStringChr(pstrUnc, L'\\');
	if (NULL == pTmp)
	{
		status = STATUS_INVALID_PARAMETER;
		return status;
	}
	
	//取 SharedDocs
	strShare.Length = (USHORT)((ULONG)pTmp - (ULONG)pstrUnc->Buffer);
	RtlCopyMemory(strShare.Buffer, pstrUnc->Buffer, strShare.Length);

	//取 \\111.txt
	strName.Length = pstrUnc->Length - strShare.Length;
	RtlCopyMemory(strName.Buffer, pTmp, strName.Length);

	//查看 HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\LanmanServer\Shares   
	//有木有 SharedDocs 字段

	RtlInitUnicodeString(&ustrReg, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Shares");
	InitializeObjectAttributes(&objectAttr,
		&ustrReg,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		NULL);

	status = ZwCreateKey( 
		&hRegister,
		KEY_ALL_ACCESS,
		&objectAttr,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		&uResult);

	if (!NT_SUCCESS(status))
	{
		goto __error;
	}

	status = ZwQueryValueKey(hRegister,
		&strShare,
		KeyValuePartialInformation,
		NULL,
		0,
		&uResult);

	//非法UNC路径
	if (status != STATUS_BUFFER_OVERFLOW &&
		status != STATUS_BUFFER_TOO_SMALL)
	{
		goto __error;
	}

	pkpi = 	(PKEY_VALUE_PARTIAL_INFORMATION)
		ExAllocatePoolWithTag(PagedPool, uResult, MEM_TAG);
	if (pkpi == NULL)
	{
		status = STATUS_MEMORY_NOT_ALLOCATED;
		goto __error;
	}

	status = ZwQueryValueKey(hRegister,
		&strShare,
		KeyValuePartialInformation,
		pkpi,
		uResult,
		&uResult);
	if (!NT_SUCCESS(status))
	{
		goto __error;
	}
	
	//类型必须是这个
	if (pkpi->Type != REG_MULTI_SZ)
	{
		goto __error;
	}
	//解析出Path=来
	ustrReg.Length = (USHORT)pkpi->DataLength;
	ustrReg.MaximumLength = (USHORT)pkpi->DataLength;
	ustrReg.Buffer = (WCHAR*)(pkpi->Data);

	RtlInitUnicodeString(&strShare, L"path=");
	pTmp = RtlUnicodeStringStr(&ustrReg, &strShare);//strstr
	if (NULL == pTmp)
	{
		status = STATUS_UNSUCCESSFUL;
		goto __error;
	}
	else
	{
		RtlInitUnicodeString(&strShare, pTmp + wcslen(L"path="));
		RtlCopyUnicodeString(pstrLocal, &strShare);
		RtlAppendUnicodeStringToString(pstrLocal, &strName);
		status = STATUS_SUCCESS;
	}

__error:
	if (pkpi)
	{
		ExFreePool(pkpi);
	}
	if (hRegister)
	{
		ZwClose(hRegister);
	}

	return status;
}