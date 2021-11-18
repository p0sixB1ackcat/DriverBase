#include <ntddk.h>

typedef struct _FILE_LIST_ENTRY
{
	LIST_ENTRY Entry;
	PWSTR NameBuffer;

} FILE_LIST_ENTRY,*PFILE_LIST_ENTRY;

typedef struct _FILE_DIRECTORY_INFORMATION {
	ULONG NextEntryOffset;
	ULONG FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

NTSTATUS PBCDeleteFile(WCHAR *);

NTSTATUS PBCDeleteDirectory(WCHAR *szDirPath);

NTSTATUS 
ZwQueryDirectoryFile(
					 __in HANDLE  FileHandle,
					 __in_opt HANDLE  Event,
					 __in_opt PIO_APC_ROUTINE  ApcRoutine,
					 __in_opt PVOID  ApcContext,
					 __out PIO_STATUS_BLOCK  IoStatusBlock,
					 __out PVOID  FileInformation,
					 __in ULONG  Length,
					 __in FILE_INFORMATION_CLASS  FileInformationClass,
					 __in BOOLEAN  ReturnSingleEntry,
					 __in_opt PUNICODE_STRING  FileName,
					 __in BOOLEAN  RestartScan
					 );

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("PBCDeleteDir:goodbye!\n");
	return;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING pDriverPath)
{
	NTSTATUS ntStatus;

	DbgPrint("PBCDeleteDir:hello!\n");
	pDriverObject->DriverUnload = DriverUnload;

	ntStatus = PBCDeleteDirectory(L"\\??\\c:\\test");
	

	return STATUS_SUCCESS;

}

NTSTATUS PBCDeleteDirectory(WCHAR *szDirPath)
{
	NTSTATUS ntStatus;
	UNICODE_STRING uDirPath = {0x00};
	LIST_ENTRY listHeader = {0x00};
	PWSTR tmpNameBuffer = NULL;
	UNICODE_STRING uTmpNameStr = {0x00};
	PFILE_LIST_ENTRY pCurrentListEntry = NULL;
	PFILE_LIST_ENTRY pPreListEntry = NULL;
	HANDLE hFileHandle = NULL;
	OBJECT_ATTRIBUTES objFileAttributes = {0x00};
	IO_STATUS_BLOCK ioStatusBlock = {0x00};
	PVOID queryOutputBuffer = NULL;
	ULONG queryOutputBufferSize;

	PFILE_DIRECTORY_INFORMATION pCurrentDirInfo = NULL;
	BOOLEAN reStartScan;
	FILE_DISPOSITION_INFORMATION fileDisInfo = {0x00};
	

	//初始化...
	RtlInitUnicodeString(&uDirPath,szDirPath);
	
	tmpNameBuffer = (WCHAR *)ExAllocatePoolWithTag(PagedPool,uDirPath.Length + sizeof(WCHAR),'EMAN');
	if(!tmpNameBuffer)
	{
		DbgPrint("alloc memory with tmpNameBuffer faild\n");
		ntStatus = STATUS_BUFFER_ALL_ZEROS;
		return ntStatus;
	}
	
	pCurrentListEntry = (PFILE_LIST_ENTRY)ExAllocatePoolWithTag(PagedPool,sizeof(FILE_LIST_ENTRY),'TSIL');
	if(!pCurrentListEntry)
	{
		DbgPrint("alloc memory with pCurrentListEntry faild\n");
		ntStatus = STATUS_BUFFER_ALL_ZEROS;
		return ntStatus;
	}
	
	RtlCopyMemory(tmpNameBuffer,uDirPath.Buffer,uDirPath.Length);
	tmpNameBuffer[uDirPath.Length / sizeof(WCHAR)] = L'\0';

	InitializeListHead(&listHeader);
	pCurrentListEntry->NameBuffer = tmpNameBuffer;
	InsertHeadList(&listHeader,&pCurrentListEntry->Entry);

	while(!IsListEmpty(&listHeader))//遍历链表，用链表结点表示每个文件夹
	{
		pCurrentListEntry = (PFILE_LIST_ENTRY)RemoveEntryList(&listHeader);
		//如果当前要删除的文件夹还是上一次要删除的，说明上次没有将其删除掉，也就是该文件夹不为空
		if(pPreListEntry == pCurrentListEntry)
		{
			ntStatus = STATUS_DIRECTORY_NOT_EMPTY;
			break;
		}
		//保存本次要删除的，供下一次遍历时做参考
		pPreListEntry = pCurrentListEntry;
		
		RtlInitUnicodeString(&uTmpNameStr,pCurrentListEntry->NameBuffer);

		InitializeObjectAttributes(&objFileAttributes,
			&uTmpNameStr,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
			NULL,
			NULL);

		ntStatus = ZwCreateFile(&hFileHandle,
			FILE_ANY_ACCESS,
			&objFileAttributes,
			&ioStatusBlock,
			NULL,
			0,
			0,
			FILE_OPEN,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);

		if(!NT_SUCCESS(ntStatus))
		{
			DbgPrint("ZwCreateFile failed(%zW):%x\n",&uTmpNameStr,ntStatus);
			break;
		}
		
		reStartScan = TRUE;//表示查询文件时，是否接着上次查询的位置
		//开始扫描
		while(TRUE)
		{
			queryOutputBuffer = NULL;
			queryOutputBufferSize = 64;
			ntStatus = STATUS_BUFFER_OVERFLOW;

			while((ntStatus == STATUS_BUFFER_OVERFLOW) || 
				ntStatus == STATUS_INFO_LENGTH_MISMATCH)
			{
				if(queryOutputBuffer)
				{
					ExFreePool(queryOutputBuffer);
				}
				//由于不能提前获得存储文件夹信息内存的长度
				//所以，需要使用这种方式来分配内存，保证output长度能够容纳
				queryOutputBufferSize *= 2;

				queryOutputBuffer = ExAllocatePoolWithTag(PagedPool,queryOutputBufferSize,'REUQ');
				if(!queryOutputBuffer)
				{
					DbgPrint("alloc memory queryOutputBuffer faild\n");
					//这里用break不太好吧？后面的代码还会执行...
					break;
				}

				ntStatus = ZwQueryDirectoryFile(hFileHandle,
					NULL,
					NULL,
					NULL,
					&ioStatusBlock,
					&queryOutputBuffer,
					queryOutputBufferSize,
					FileDirectoryInformation,
					FALSE,
					NULL,
					reStartScan);
			}

			if(ntStatus == STATUS_NO_MORE_FILES)
			{
				//如果查找到该文件夹的末尾
				//别忘了释放内存
				ExFreePool(queryOutputBuffer);
				//设置成功标记
				ntStatus = STATUS_SUCCESS;
				break;
			}

			if(!NT_SUCCESS(ntStatus))
			{
				//到这里，表示上面的操作有失败...
				//显示error
				DbgPrint("ZwQueryDirectoryFile faild(%wZ):%x\n",pCurrentListEntry->NameBuffer,ntStatus);
				//还是别忘了释放内存，这是作为c程序员内核开发要牢记的，"时刻不忘'躺尸剑法...'"
				ExFreePool(queryOutputBuffer);
				break;
			}

			//到这，就说明一切都已准备就绪...
			pCurrentDirInfo = (PFILE_DIRECTORY_INFORMATION)queryOutputBuffer;

			//获取当前目录下的当前文件
			//重新分配内核堆
			//上级目录名+当前文件名+L"\\"
			tmpNameBuffer = ExAllocatePoolWithTag(PagedPool,
				wcslen(pCurrentListEntry->NameBuffer) + sizeof(pCurrentDirInfo->FileName) + 2 * sizeof(WCHAR),
				'PMT');

			
			RtlZeroMemory(tmpNameBuffer,
				wcslen(pCurrentListEntry->NameBuffer) + sizeof(pCurrentDirInfo->FileName) + 2 * sizeof(WCHAR),
				);
			wcscpy(tmpNameBuffer,pCurrentListEntry->NameBuffer);
			wcscat(tmpNameBuffer,L"\\");
			//不能用这个操作?
			//wcscpy(tmpNameBuffer,pCurrentDirInfo->FileName);
			RtlCopyMemory(tmpNameBuffer,pCurrentDirInfo->FileName,pCurrentDirInfo->FileNameLength);

			RtlInitUnicodeString(&uTmpNameStr,tmpNameBuffer);

			//得到文件名之后，在判断文件类型
			//如果是文件夹类型...
			if(pCurrentDirInfo->FileAttributes | FILE_ATTRIBUTE_DIRECTORY)
			{
				if((pCurrentDirInfo->FileNameLength == sizeof (WCHAR)) && (pCurrentDirInfo->FileName[0] == L'.'))
				{

				}
				else if((pCurrentDirInfo->FileNameLength == 2 * sizeof(WCHAR)) 
					&& (pCurrentDirInfo->FileName[0] == L'.')
					&& (pCurrentDirInfo->FileName[1] == L'.'))
				{

				}
				else
				{
					//如果不是.和..，那么将该文件夹名称插入链表，等和该文件夹同级目录下的非文件夹文件删除完
					//在来删除该文件夹里面的内容
					//注意，这里已经是scaning...
					PFILE_LIST_ENTRY tmpListEntry;

					tmpListEntry = (PFILE_LIST_ENTRY)ExAllocatePoolWithTag(PagedPool,
						sizeof(FILE_LIST_ENTRY),
						"LPMT");
					if(!tmpListEntry)
					{
						DbgPrint("alloc memory tmpListEntry faild(%wZ)!\n",&uTmpNameStr);
						break;
					}

					tmpListEntry->NameBuffer = tmpNameBuffer;
					ExFreePool(tmpNameBuffer);
					InsertHeadList(&listHeader,&tmpListEntry->Entry);
				}
			}
			else
			{
				//否则，非文件夹，直接干掉
				ntStatus = PBCDeleteFile(tmpNameBuffer);
				if(!NT_SUCCESS(ntStatus))
				{
					DbgPrint("PBCDeleteFile faild:%x\n",ntStatus);
					ExFreePool(tmpNameBuffer);
					ExFreePool(queryOutputBuffer);
					break;
				}
			}

			ExFreePool(queryOutputBuffer);
			if(tmpNameBuffer)
			{
				ExFreePool(tmpNameBuffer);
			}
		}//while(TRUE)，和上文的扫描入口做个结束标记
	
		//执行这里，就是说当前文件夹下的所有非文件夹类型的子文件已被删除
		//就要删当前文件夹了
		if(NT_SUCCESS(ntStatus))//还要判断一下上面的操作有没有出错的地方
		{
			fileDisInfo.DeleteFile = TRUE;
			ntStatus = ZwSetInformationFile(hFileHandle,
				&ioStatusBlock,
				&fileDisInfo,
				sizeof(FILE_DISPOSITION_INFORMATION),
				FileDispositionInformation);
			if(!NT_SUCCESS(ntStatus))
			{
				//说明该文件夹下还有子文件夹
				//如果是传过来的根目录，那么就不打印错误信息了
				UNICODE_STRING uCompStr = {0x00};
				RtlInitUnicodeString(&uCompStr,pCurrentListEntry->NameBuffer);
				if(RtlCompareUnicodeString(&uCompStr,&uDirPath,TRUE) != 0)
				{
					DbgPrint("ZwSetInformationFile fail(%ws):%x\n",pCurrentListEntry->NameBuffer,ntStatus);
					break;
				}
			}
		}

		ZwClose(hFileHandle);

		if(NT_SUCCESS(ntStatus))
		{
			//执行到此，说明当前文件夹已被删除
			//将当前文件夹名从链表中移除
			RemoveHeadList(&listHeader);
			ExFreePool(pCurrentListEntry->NameBuffer);
			ExFreePool(pCurrentListEntry);
		}
		//如果是失败，说明该文件夹下还有子文件夹，那么链表头的位置存放着下级目录名
		//也就是不能从链表中移除了
		//先去删除子文件夹

	}//while(!IsListEmpty(&listHeader)) 遍历链表的入口结束标记

	//最后，在对链表进行一次释放
	while(!IsListEmpty(&listHeader))
	{
		PFILE_LIST_ENTRY freeListEntry = (PFILE_LIST_ENTRY)RemoveHeadList(&listHeader);
		ExFreePool(freeListEntry->NameBuffer);
		ExFreePool(freeListEntry);
	}

	return ntStatus;


}

NTSTATUS PBCDeleteFile(WCHAR * szFileName)
{
	UNICODE_STRING uFileName = {0x00};
	OBJECT_ATTRIBUTES objFileAttributes = {0x00};
	HANDLE hFileHandle = NULL;
	NTSTATUS ntStatus;
	IO_STATUS_BLOCK ioStatusBlock = {0x00};
	FILE_DISPOSITION_INFORMATION fileDisInfo = {0x00};

	RtlInitUnicodeString(&uFileName,szFileName);

	InitializeObjectAttributes(&objFileAttributes,
		&uFileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	
	ntStatus = ZwCreateFile(&hFileHandle,
		SYNCHRONIZE | FILE_WRITE_ATTRIBUTES | DELETE,
		&objFileAttributes,
		&ioStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE |FILE_SHARE_DELETE,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT |FILE_DELETE_ON_CLOSE,
		NULL,
		0);
	if(!NT_SUCCESS(ntStatus))
	{
		if(ntStatus == STATUS_ACCESS_DENIED)
		{
			ntStatus = ZwCreateFile(&hFileHandle,
				SYNCHRONIZE | FILE_WRITE_ACCESS | FILE_READ_ATTRIBUTES,
				&objFileAttributes,
				&ioStatusBlock,
				0,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE,
				NULL,
				0);
			
			if(NT_SUCCESS(ntStatus))
			{
				FILE_BASIC_INFORMATION fileBasicInfo = {0x00};

				ntStatus = ZwQueryInformationFile(hFileHandle,
					&ioStatusBlock,
					&fileBasicInfo,
					sizeof(FILE_BASIC_INFORMATION),
					FileBasicInformation);
				if(!NT_SUCCESS(ntStatus))
				{
					DbgPrint("ZwQueryInformationFile faild:%x\n",ntStatus);
				}

				//去只读
				fileBasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
				ntStatus = ZwSetInformationFile(hFileHandle,
					&ioStatusBlock,
					&fileBasicInfo,
					sizeof(FILE_BASIC_INFORMATION),
					FileBasicInformation);
				if(!NT_SUCCESS(ntStatus))
				{
					DbgPrint("ZwSetInformationFile faild:%x\n",ntStatus);
				}

				ZwClose(hFileHandle);
				ntStatus = ZwCreateFile(&hFileHandle,
					SYNCHRONIZE | FILE_WRITE_ATTRIBUTES | DELETE,
					&objFileAttributes,
					&ioStatusBlock,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					FILE_OPEN,
					FILE_SYNCHRONOUS_IO_NONALERT |FILE_DELETE_ON_CLOSE,
					NULL,
					0);
			}

		}

	}

	if(!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	fileDisInfo.DeleteFile = TRUE;
	ntStatus = ZwSetInformationFile(hFileHandle,
		&ioStatusBlock,
		&fileDisInfo,
		sizeof(FILE_DISPOSITION_INFORMATION),
		FileDispositionInformation);
	
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ZwSetInformationFile faild:%x\n",ntStatus);
	}

	ZwClose(hFileHandle);
	return ntStatus;

}