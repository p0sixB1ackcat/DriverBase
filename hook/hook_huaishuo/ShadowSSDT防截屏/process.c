#include<Ntifs.h>
#include <ntddk.h>
#include <Ntstrsafe.h>

#define MAX_PATH (260)


EXTERN_C NTSYSAPI NTSTATUS NTAPI 
	ZwQueryInformationProcess(
	__in       HANDLE ProcessHandle,
	__in       PROCESSINFOCLASS ProcessInformationClass,
	__out      PVOID ProcessInformation,
	__in       ULONG ProcessInformationLength,
	__out_opt  PULONG ReturnLength
	);

NTSTATUS  GetProcessFullNameByPid(HANDLE nPid, PUNICODE_STRING  FullPath)
{

    HANDLE               hFile      = NULL;
    ULONG                nNeedSize	= 0;
    NTSTATUS             nStatus    = STATUS_SUCCESS;
    NTSTATUS             nDeviceStatus = STATUS_DEVICE_DOES_NOT_EXIST;
    PEPROCESS            Process    = NULL;
    KAPC_STATE           ApcState   = {0};			
    PVOID                lpBuffer   = NULL;
    OBJECT_ATTRIBUTES	 ObjectAttributes = {0};
    IO_STATUS_BLOCK      IoStatus   = {0}; 
    PFILE_OBJECT         FileObject = NULL;
    PFILE_NAME_INFORMATION FileName = NULL;   
    WCHAR                FileBuffer[MAX_PATH] = {0};
    DECLARE_UNICODE_STRING_SIZE(ProcessPath,MAX_PATH);
    DECLARE_UNICODE_STRING_SIZE(DosDeviceName,MAX_PATH);
    
    PAGED_CODE();

    nStatus = PsLookupProcessByProcessId(nPid, &Process);
    if(NT_ERROR(nStatus))
    {
        KdPrint(("%s error PsLookupProcessByProcessId.\n",__FUNCTION__));
        return nStatus;
    }



    __try
    {

        KeStackAttachProcess(Process, &ApcState);
        
        nStatus = ZwQueryInformationProcess(
            NtCurrentProcess(),
            ProcessImageFileName,
            NULL,
            0,
            &nNeedSize
            );

        if (STATUS_INFO_LENGTH_MISMATCH != nStatus)
        {
            KdPrint(("%s NtQueryInformationProcess error.\n",__FUNCTION__)); 
            nStatus = STATUS_MEMORY_NOT_ALLOCATED;
            __leave;

        }

        lpBuffer = ExAllocatePoolWithTag(NonPagedPool, nNeedSize,'GetP');
        if (lpBuffer == NULL)
        {
            KdPrint(("%s ExAllocatePoolWithTag error.\n",__FUNCTION__));
            nStatus = STATUS_MEMORY_NOT_ALLOCATED;
            __leave; 
        }

       nStatus =  ZwQueryInformationProcess(
           NtCurrentProcess(),
           ProcessImageFileName, 
           lpBuffer, 
           nNeedSize,
           &nNeedSize
           );

       if (NT_ERROR(nStatus))
       {
           KdPrint(("%s NtQueryInformationProcess error2.\n",__FUNCTION__));
           __leave;
       }

       RtlCopyUnicodeString(&ProcessPath,(PUNICODE_STRING)lpBuffer);
       InitializeObjectAttributes(
           &ObjectAttributes,
           &ProcessPath,
           OBJ_CASE_INSENSITIVE,
           NULL,
           NULL
           );

       nStatus = ZwCreateFile(
           &hFile,
           FILE_READ_ATTRIBUTES,
           &ObjectAttributes,
           &IoStatus,
           NULL,
           FILE_ATTRIBUTE_NORMAL,
           0,
           FILE_OPEN,
           FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
           NULL,
           0
           );  

       if (NT_ERROR(nStatus))
       {
           hFile = NULL;
           __leave;
       }

       nStatus = ObReferenceObjectByHandle(
           hFile, 
           0,
           *IoFileObjectType, 
           KernelMode, 
           (PVOID*)&FileObject,
           NULL
           );

       if (NT_ERROR(nStatus))
       {
           FileObject = NULL;
           __leave;
       }
       
       FileName = (PFILE_NAME_INFORMATION)FileBuffer;
       
       nStatus = ZwQueryInformationFile(
           hFile,
           &IoStatus,
           FileName,
           sizeof(WCHAR)*MAX_PATH,
           FileNameInformation
           );

       if (NT_ERROR(nStatus))
       {
           __leave;
       }

       if (FileObject->DeviceObject == NULL)
       {
           nDeviceStatus = STATUS_DEVICE_DOES_NOT_EXIST;
           __leave;
       }

       nDeviceStatus = RtlVolumeDeviceToDosName(FileObject->DeviceObject,&DosDeviceName);

    }
    __finally
    {
        if (NULL != FileObject)
        {
            ObDereferenceObject(FileObject);
        }

        if (NULL != hFile)
        {
            ZwClose(hFile);
        }

        if (NULL != lpBuffer)
        {
            ExFreePool(lpBuffer);
        }

        KeUnstackDetachProcess(&ApcState);


    }

    if (NT_SUCCESS(nStatus))
    {
        RtlInitUnicodeString(&ProcessPath,FileName->FileName);

        if (NT_SUCCESS(nDeviceStatus))
        {
            RtlCopyUnicodeString(FullPath,&DosDeviceName);
            RtlUnicodeStringCat(FullPath,&ProcessPath);
        }
        else
        {
            RtlCopyUnicodeString(FullPath,&ProcessPath);
        }
    }


    return nStatus;
}
