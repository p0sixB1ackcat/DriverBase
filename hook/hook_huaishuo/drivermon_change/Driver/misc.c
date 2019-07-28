#include "precomp.h"

BOOL obQueryObjectName(PVOID pObject, PUNICODE_STRING objName, BOOL allocateName)
{
	PVOID buffer = NULL;
	DWORD reqSize = 0;
	NTSTATUS status = 0;
	__try
	{
		reqSize = sizeof(OBJECT_NAME_INFORMATION) + (MAX_PATH + 32)*sizeof(WCHAR);

		buffer = ExAllocatePoolWithTag(PagedPool, reqSize, 'CORP');

		if(buffer == NULL)
			return FALSE;

		status = ObQueryNameString(pObject, 
								   buffer,
								   reqSize,
								   &reqSize);

		if((status == STATUS_INFO_LENGTH_MISMATCH) ||
		   (status == STATUS_BUFFER_OVERFLOW) ||
		   (status == STATUS_BUFFER_TOO_SMALL))
		{
			ExFreePool(buffer);
			buffer = NULL;

			buffer = ExAllocatePoolWithTag(PagedPool, reqSize, 'rtpR');

			if(buffer == NULL)
			{
				return FALSE;
			}
			
			status = ObQueryNameString(pObject, 
								   buffer,
								   reqSize,
								   &reqSize);

		}

		if(NT_SUCCESS(status))
		{ 
			OBJECT_NAME_INFORMATION * pNameInfo = (OBJECT_NAME_INFORMATION *)buffer;

			if(allocateName)
			{
				objName->Buffer = ExAllocatePoolWithTag(PagedPool, pNameInfo->Name.Length + sizeof(WCHAR), 'rtpR');
		
				if(objName->Buffer)
				{
					RtlZeroMemory(objName->Buffer, pNameInfo->Name.Length + sizeof(WCHAR));
					objName->Length = 0;
					objName->MaximumLength = pNameInfo->Name.Length;
					RtlCopyUnicodeString(objName, &pNameInfo->Name);
				}
				else
					status = STATUS_INSUFFICIENT_RESOURCES;
				
			}
			else
				RtlCopyUnicodeString(objName, &pNameInfo->Name);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		status = GetExceptionCode();
	}

	if(buffer)
	{
		ExFreePool(buffer);
		buffer = NULL;
	}

	return NT_SUCCESS(status);
}

BOOL ntGetNameFromObjectAttributes(POBJECT_ATTRIBUTES ObjectAttributes,
								   WCHAR * NameBuffer,
								   DWORD NameBufferSize)
{
	BOOL rtn = FALSE;
	UNICODE_STRING uTarget = {0,0,0};
	UNICODE_STRING CapturedName = {0,0,0};
	KPROCESSOR_MODE PrevMode = ExGetPreviousMode();
	OBJECT_ATTRIBUTES CapturedAttributes;
	BOOL bGotRootDirectory = FALSE;

	__try
	{
		if(PrevMode != KernelMode)
		{
			CapturedAttributes = ProbeAndReadObjectAttributes(ObjectAttributes);
		}
		else
			CapturedAttributes = *ObjectAttributes;

		if((CapturedAttributes.ObjectName == NULL) &&
		   (CapturedAttributes.RootDirectory == NULL))
		{
			return FALSE;
		}

		uTarget.Length = 0;
		uTarget.Buffer = NameBuffer;
		uTarget.MaximumLength = (USHORT)NameBufferSize;

		if(CapturedAttributes.RootDirectory)
		{
			PVOID Object = NULL;
			NTSTATUS status = 0;

			status = ObReferenceObjectByHandle(
					CapturedAttributes.RootDirectory,
					0,
					NULL,
					PrevMode,
					&Object,
					NULL
					);

			if(NT_SUCCESS(status))
			{
				if(obQueryObjectName(Object, &uTarget, FALSE) == FALSE)
				{
					ObDereferenceObject(Object);
					return FALSE;
				}

				bGotRootDirectory = TRUE;
				ObDereferenceObject(Object);
			}
			else
			{
				return FALSE;
			}

			if(CapturedAttributes.ObjectName)
				RtlAppendUnicodeToString(&uTarget, L"\\");
		}

		if(CapturedAttributes.ObjectName)
		{
			if(PrevMode != KernelMode)
			{
				CapturedName = ProbeAndReadUnicodeString(CapturedAttributes.ObjectName);

				ProbeForRead(CapturedName.Buffer,
							 CapturedName.Length,
							 sizeof(WCHAR));
			}
			else
				CapturedName = *CapturedAttributes.ObjectName;

			if(bGotRootDirectory)
			{
				RtlAppendUnicodeStringToString(&uTarget, &CapturedName);
			}
			else
			{
				RtlCopyUnicodeString(&uTarget, &CapturedName);
			}

			rtn = TRUE;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		rtn = FALSE;
	}

	return rtn;
}

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

BOOL ntIsDosDeviceName(WCHAR * filename)
{
	int i = 0;
	
	for(i = 0; i < MAX_PATH; i++)
	{
		if(filename[i] == L'\0')
			break;
		
		if((filename[i] == L':') && ((i == 1) || ( i == 5)))
		{
			return TRUE;
		}
	}
	
	return FALSE;
}


NTSTATUS ntQuerySymbolicLinkName(PUNICODE_STRING SymbolicLinkName, 
				PUNICODE_STRING LinkTarget)
{
	OBJECT_ATTRIBUTES oa; 
	NTSTATUS status = 0; 
	HANDLE LinkHandle = 0; 

	InitializeObjectAttributes(&oa, 
							   SymbolicLinkName, 
							   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 
							   0, 
							   0); 

	status = ZwOpenSymbolicLinkObject(&LinkHandle, GENERIC_READ, &oa); 

	if (NT_SUCCESS(status) == FALSE) 
	{ 
		return status; 
	} 


	status = ZwQuerySymbolicLinkObject(LinkHandle, LinkTarget, NULL); 
	ZwClose(LinkHandle); 
	return status; 
}

BOOL ntQueryVolumeName(WCHAR ch, WCHAR * name, USHORT size)
{
	WCHAR szVolume[7] = L"\\??\\C:";
	UNICODE_STRING LinkName;
	UNICODE_STRING VolName;

	RtlInitUnicodeString(&LinkName, szVolume);

	VolName.Buffer = name;
	VolName.Length = 0;
	VolName.MaximumLength = size;

	szVolume[4] = ch;

	return NT_SUCCESS(ntQuerySymbolicLinkName(&LinkName, &VolName));
}

BOOL isRootDir(WCHAR * dir)
{
	SIZE_T len = wcslen(dir);
	
	if((len == 23) &&
		(_wcsnicmp(dir, L"\\Device\\HarddiskVolume", 22) == 0))
		return TRUE;
	
	if((len == 2) && (dir[1] == L':'))
		return TRUE;
	
	if((len == 6) && 
		(_wcsnicmp(dir, L"\\??\\", 4) == 0) &&
		(dir[5] == L':'))
		return TRUE;
	
	if((len == 14) && 
		(_wcsnicmp(dir, L"\\DosDevices\\", 12) == 0) &&
		(dir[13] == L':'))
		return TRUE;
	
	return FALSE;
}


BOOL IsDirSep(WCHAR ch) 
{
    return (ch == L'\\' || ch == L'/');
}



BOOL ntQueryDirectory(WCHAR * rootdir, 
					  WCHAR * shortname, 
					  WCHAR *longname, 
					  ULONG size)
{
	UNICODE_STRING uRootDir;
	UNICODE_STRING uShortName;
	UNICODE_STRING uLongName;
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK Iosb;
	PFILE_BOTH_DIR_INFORMATION pInfo = NULL;
	NTSTATUS Status = 0;
	HANDLE hRootDir = 0;
	BYTE  * Buffer = NULL;
	WCHAR * szRoot = NULL;

	RtlZeroMemory(&Iosb, sizeof(IO_STATUS_BLOCK));
	Iosb.Status = STATUS_NO_SUCH_FILE;

	szRoot = ExAllocatePoolWithTag(PagedPool,
								  MAX_PATH * sizeof(WCHAR),
								  'SPIH');
	if(szRoot == NULL)
	{
		return FALSE;
	}

	RtlZeroMemory(szRoot, MAX_PATH * sizeof(WCHAR));

	wcsncpy(szRoot, rootdir, MAX_PATH);

	RtlInitUnicodeString(&uRootDir, szRoot);
	RtlInitUnicodeString(&uShortName, shortname);

	if(isRootDir(szRoot))
		RtlAppendUnicodeToString(&uRootDir, L"\\");

	InitializeObjectAttributes(&oa,
							   &uRootDir,
							   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
							   0, 
							   0);  
	
	Status = ZwCreateFile(&hRootDir,
							GENERIC_READ | SYNCHRONIZE,
							&oa,
							&Iosb,
							0, 
							FILE_ATTRIBUTE_DIRECTORY, 
							FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
							FILE_OPEN, 
							FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT , 
							0,
							0);

	if (!NT_SUCCESS(Status)) 
	{ 
		ExFreePool(szRoot);
		return FALSE;
	}

	ExFreePool(szRoot);

	Buffer = ExAllocatePoolWithTag(PagedPool,
						  1024,
						  'SPIH');
	if(Buffer == NULL)
	{
		ZwClose(hRootDir);
		return FALSE;
	}

	RtlZeroMemory(Buffer, 1024);

	Status = ZwQueryDirectoryFile(hRootDir,
									NULL,
									0, // No APC routine
									0, // No APC context
									&Iosb,
									Buffer,
									1024,
									FileBothDirectoryInformation,
									TRUE,
									&uShortName,
									TRUE);

	if (!NT_SUCCESS(Status)) 
	{
		ExFreePool(Buffer);
		ZwClose(hRootDir);
		return FALSE;
	}

	ZwClose(hRootDir);

	pInfo = (PFILE_BOTH_DIR_INFORMATION) Buffer;
	
	if(pInfo->FileNameLength == 0)
	{
		ExFreePool(Buffer);
		return FALSE;
	}

	uShortName.Length = uShortName.MaximumLength = (USHORT)pInfo->FileNameLength;
	uShortName.Buffer = pInfo->FileName;

	if(size < uShortName.Length)
	{	
		ExFreePool(Buffer);
		return FALSE;
	}

	uLongName.Length = 0;
	uLongName.MaximumLength = (USHORT)size;
	uLongName.Buffer = longname;

	RtlCopyUnicodeString(&uLongName, &uShortName);
	ExFreePool(Buffer);
	return TRUE;
}

BOOL ntFindFile(WCHAR * fullpath, WCHAR * longname, ULONG size)
{
	BOOL rtn = FALSE;
	WCHAR * pchScan = fullpath;
	WCHAR * pchEnd = NULL;
	
	while(*pchScan)
	{
		if(IsDirSep(*pchScan))
			pchEnd = pchScan;
		
		pchScan++;
	}
	
	if(pchEnd)
	{
		*pchEnd++ = L'\0';
		rtn = ntQueryDirectory(fullpath, pchEnd, longname, size);
		*(--pchEnd) = L'\\';
	}
	return rtn;
}

BOOL ntGetLongName(WCHAR * shortname, WCHAR * longname, ULONG size)
{
	WCHAR * szResult = NULL;
	WCHAR* pchResult = NULL;
	WCHAR* pchScan = shortname;
	INT offset = 0;
  
	szResult = ExAllocatePoolWithTag(PagedPool,
						  sizeof(WCHAR) * (MAX_PATH * 2 + 1),
						  'SPIH');

	if(szResult == NULL)
		return FALSE;
	
	RtlZeroMemory(szResult, sizeof(WCHAR) * (MAX_PATH * 2 + 1));
	pchResult = szResult;

	if (pchScan[0] && pchScan[1] == L':') 
	{
		*pchResult++ = L'\\';
		*pchResult++ = L'?';
		*pchResult++ = L'?';
		*pchResult++ = L'\\';
		*pchResult++ = *pchScan++;
		*pchResult++ = *pchScan++;
		offset = 4;
	}
	else if (IsDirSep(pchScan[0]) && IsDirSep(pchScan[1]))
	{
		*pchResult++ = L'\\';
		*pchResult++ = L'D';
		*pchResult++ = L'e';
		*pchResult++ = L'v';
		*pchResult++ = L'i';
		*pchResult++ = L'c';
		*pchResult++ = L'e';
		*pchResult++ = L'\\';
		*pchResult++ = L'L';
		*pchResult++ = L'a';
		*pchResult++ = L'n';
		*pchResult++ = L'M';
		*pchResult++ = L'a';
		*pchResult++ = L'n';
		*pchResult++ = L'R';
		*pchResult++ = L'e';
		*pchResult++ = L'd';
		*pchResult++ = L'i';
		*pchResult++ = L'r';
		*pchResult++ = L'e';
		*pchResult++ = L'c';
		*pchResult++ = L't';
		*pchResult++ = L'o';
		*pchResult++ = L'r';
		*pchResult++ = *pchScan++;
		*pchScan++;
		while (*pchScan && !IsDirSep(*pchScan))
			*pchResult++ = *pchScan++;

		offset = 24;
	}
	else if (_wcsnicmp(pchScan, L"\\DosDevices\\", 12) == 0)
	{
		RtlStringCbCopyW(pchResult, sizeof(WCHAR) * (MAX_PATH * 2 + 1), L"\\??\\");
		pchResult += 4;
		pchScan += 12;
		while (*pchScan && !IsDirSep(*pchScan))
			*pchResult++ = *pchScan++;
		offset = 4;
	}
	else if (_wcsnicmp(pchScan, L"\\Device\\HardDiskVolume", 22) == 0)
	{
		RtlStringCbCopyW(pchResult, sizeof(WCHAR) * (MAX_PATH * 2 + 1),L"\\Device\\HardDiskVolume");
		pchResult += 22;
		pchScan += 22;
		while (*pchScan && !IsDirSep(*pchScan))
			*pchResult++ = *pchScan++;
	}
	else if (_wcsnicmp(pchScan, L"\\??\\", 4) == 0)
	{
		RtlStringCbCopyW(pchResult, sizeof(WCHAR) * (MAX_PATH * 2 + 1), L"\\??\\");
		pchResult += 4;
		pchScan += 4;

		while (*pchScan && !IsDirSep(*pchScan))
			*pchResult++ = *pchScan++;
	}
	else
	{
		ExFreePool(szResult);
		return FALSE;
	}

	while (IsDirSep(*pchScan)) 
	{
		BOOL bShort = FALSE;
		WCHAR* pchEnd = NULL;
		WCHAR* pchReplace = NULL;
		*pchResult++ = *pchScan++;

		pchEnd = pchScan;
		pchReplace = pchResult;

		while (*pchEnd && !IsDirSep(*pchEnd))
		{
			if(*pchEnd == L'~')
				bShort = TRUE;

			*pchResult++ = *pchEnd++;
		}

		*pchResult = L'\0';
  
		if(bShort)
		{
			WCHAR  * szLong = NULL;
			
			szLong = ExAllocatePoolWithTag(PagedPool,
						  sizeof(WCHAR) * MAX_PATH,
						  'SPIH');
			if(szLong)
			{
				RtlZeroMemory(szLong,  sizeof(WCHAR) * MAX_PATH);

				if(ntFindFile(szResult, szLong, sizeof(WCHAR) * MAX_PATH))
				{
					RtlStringCbCopyW(pchReplace, sizeof(WCHAR) * (MAX_PATH * 2 + 1), szLong);
					pchResult = pchReplace + wcslen(pchReplace);
				}

				ExFreePool(szLong);
			}
		}

		pchScan = pchEnd;
	}

	wcsncpy(longname, szResult + offset, size/sizeof(WCHAR));
	ExFreePool(szResult);
	return TRUE;
}

BOOL ntIsDOS8Dot3Name(WCHAR * filename)
{
	int i = 0;
	
	for(i = 0; i < MAX_PATH; i++)
	{
		if(filename[i] == L'\0')
			break;
		
		if(filename[i] == L'~')
		{
			return TRUE;
		}
	}
	
	return FALSE;
}


BOOL NTAPI ntGetNtDeviceName(WCHAR * filename, WCHAR * ntname)
{
	UNICODE_STRING uVolName = {0,0,0};
	WCHAR volName[MAX_PATH] = L"";
	WCHAR tmpName[MAX_PATH] = L"";
	WCHAR chVol = L'\0';
	WCHAR * pPath = NULL;
	BOOL bExpanded = FALSE;
	int i = 0;
	
	if(ntIsDOS8Dot3Name(filename))
	{
		bExpanded = TRUE;
		ntGetLongName(filename, tmpName, MAX_PATH*sizeof(WCHAR));
	}
	else
		RtlStringCbCopyW(tmpName, MAX_PATH * sizeof(WCHAR), filename);

	for(i = 1; i < MAX_PATH - 1; i++)
	{
		if(tmpName[i] == L':')
		{
			pPath = &tmpName[(i + 1) % MAX_PATH];
			chVol = tmpName[i - 1];
			break;
		}
	}

	if(pPath == NULL)
	{
		if(bExpanded)
		{
			//If Nt device name is passed and was 8.3, return the expanded version
			RtlStringCbCopyW(ntname, MAX_PATH * sizeof(WCHAR), tmpName);
			return TRUE;
		}

		return FALSE;
	}

	if(chVol == L'?')
	{
		uVolName.Length = 0;
		uVolName.MaximumLength = MAX_PATH * sizeof(WCHAR);
		uVolName.Buffer = ntname;
		RtlAppendUnicodeToString(&uVolName, L"\\Device\\HarddiskVolume?");
		RtlAppendUnicodeToString(&uVolName, pPath);
		return TRUE;
	}
	else if(ntQueryVolumeName(chVol, volName, MAX_PATH * sizeof(WCHAR)))
	{
		uVolName.Length = 0;
		uVolName.MaximumLength = MAX_PATH * sizeof(WCHAR);
		uVolName.Buffer = ntname;
		RtlAppendUnicodeToString(&uVolName, volName);
		RtlAppendUnicodeToString(&uVolName, pPath);
		return TRUE;
	}

	return FALSE;
}

VOID ntQueryRegStr( HANDLE key, const char * name, WCHAR * rtnBuf, int bufLen, const WCHAR * defValue )
{
	
	NTSTATUS rc;
	char * buf;
	ULONG len = sizeof (buf);
	ANSI_STRING aName;
	UNICODE_STRING uName;
	UNICODE_STRING uRtn;
	
	RtlInitAnsiString( &aName, name );
	
	if(RtlAnsiStringToUnicodeString( &uName, &aName, TRUE ) != STATUS_SUCCESS)
	{
		RtlStringCbCopyW(rtnBuf, bufLen * sizeof(WCHAR), defValue);
		rtnBuf[bufLen-1] = 0;
		return;
	}
	uName.Buffer[uName.Length/2] = 0;
	
	// get the size
	rc = ZwQueryValueKey ( key, &uName, KeyValuePartialInformation, NULL, 0, &len );
	if ((rc == STATUS_OBJECT_NAME_NOT_FOUND) || (len == 0)) 
	{
		RtlFreeUnicodeString( &uName );
		RtlStringCbCopyW(rtnBuf, bufLen * sizeof(WCHAR), defValue);
		rtnBuf[bufLen-1] = 0;
		return;
	}
	
	// get memory to use
	buf = ExAllocatePoolWithTag(PagedPool, len + 2,  'rtpR');
	if (buf == NULL) 
	{
		RtlFreeUnicodeString( &uName );
		RtlStringCbCopyW(rtnBuf, bufLen * sizeof(WCHAR), defValue);
		rtnBuf[bufLen-1] = 0;
		return;
	}
	
	// get it
	rc = ZwQueryValueKey ( key, &uName, KeyValuePartialInformation, buf, len, &len );
	
	// string free
	RtlFreeUnicodeString( &uName );
	
	if ((! NT_SUCCESS(rc)) || (len == 0))
		RtlStringCbCopyW(rtnBuf, bufLen * sizeof(WCHAR), defValue);
	else 
	{
		// make ansi
		RtlInitUnicodeString( &uName, (PCWSTR) ((PKEY_VALUE_PARTIAL_INFORMATION)buf)->Data );
		uRtn.Length = 0;
		uRtn.MaximumLength = bufLen * sizeof(WCHAR);
		uRtn.Buffer = rtnBuf;
		RtlCopyUnicodeString( &uRtn, &uName);
	}
	rtnBuf[bufLen-1] = 0;
	
	// free the buffer
	ExFreePool( buf );
}

BOOL ntGetDriverImagePath(PUNICODE_STRING uReg, WCHAR * filepath)
{
	HANDLE key = NULL;
	OBJECT_ATTRIBUTES oa;
	memset( &oa, 0, sizeof(oa) );
	
	InitializeObjectAttributes( &oa, uReg, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );
	
	if(NT_SUCCESS( ZwOpenKey( &key, KEY_READ, &oa )))
	{
		WCHAR path[MAX_PATH] = L"";
		
		RtlStringCbCopyW(path, sizeof(path), filepath);
		ntQueryRegStr( key, "ImagePath", filepath, MAX_PATH, path );
		
		if(ntIsDOS8Dot3Name(filepath))
		{
			WCHAR tmpName[MAX_PATH] = L"";
			
			RtlStringCbCopyW(tmpName, MAX_PATH * sizeof(WCHAR), filepath);
			ntGetLongName(tmpName, filepath, MAX_PATH*sizeof(WCHAR));
		}
		
		ZwClose(key);
		return TRUE;
	}
	
	return FALSE;
}