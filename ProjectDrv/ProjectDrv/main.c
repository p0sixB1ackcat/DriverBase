#include <ntifs.h>
#include <ntstrsafe.h>
#include <ntddk.h>
#include <WINDEF.H>
#include <process.h>
#include "globalvar.h"

#define DeviceName L"\\Device\\ProcInfo"
#define SymbolicLinkName L"\\DosDevices\\ProcInfo"


#define MAX_PATH 256
#define FLS_MAXIMUM_AVAILABLE 128
#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60

#define PsGetProcessPebName L"PsGetProcessPeb"
#define PsIsProtectedProcessName L"PsIsProtectedProcess"

typedef PPEB(__stdcall *PSGETPROCESSPEB)(PEPROCESS pEprocess);
typedef BOOLEAN(__stdcall *PSISPROTECTEDPROCESS)(PEPROCESS pEprocess);

PSGETPROCESSPEB PsGetProcessPeb = NULL;
PSISPROTECTEDPROCESS PsIsProtectedProcess = NULL;



#ifndef WIN64
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE32
#else
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE64
#endif

typedef ULONG GDI_HANDLE_BUFFER[GDI_HANDLE_BUFFER_SIZE];

typedef struct _PEB_LDR_DATA {
	BYTE       Reserved1[8];
	PVOID      Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_DRIVE_LETTER_CURDIR {
	USHORT                  Flags;
	USHORT                  Length;
	ULONG                   TimeStamp;
	UNICODE_STRING          DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	ULONG                   MaximumLength;
	ULONG                   Length;
	ULONG                   Flags;
	ULONG                   DebugFlags;
	PVOID                   ConsoleHandle;
	ULONG                   ConsoleFlags;
	HANDLE                  StdInputHandle;
	HANDLE                  StdOutputHandle;
	HANDLE                  StdErrorHandle;
	UNICODE_STRING          CurrentDirectoryPath;
	HANDLE                  CurrentDirectoryHandle;
	UNICODE_STRING          DllPath;
	UNICODE_STRING          ImagePathName;
	UNICODE_STRING          CommandLine;
	PVOID                   Environment;
	ULONG                   StartingPositionLeft;
	ULONG                   StartingPositionTop;
	ULONG                   Width;
	ULONG                   Height;
	ULONG                   CharWidth;
	ULONG                   CharHeight;
	ULONG                   ConsoleTextAttributes;
	ULONG                   WindowFlags;
	ULONG                   ShowWindowFlags;
	UNICODE_STRING          WindowTitle;
	UNICODE_STRING          DesktopName;
	UNICODE_STRING          ShellInfo;
	UNICODE_STRING          RuntimeData;
	RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];

}RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _RTL_CRITICAL_SECTION *PRTL_CRITICAL_SECTION;

typedef struct _PEB
{
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	union
	{
		BOOLEAN BitField;
		struct
		{
			BOOLEAN ImageUsesLargePages : 1;
			BOOLEAN IsProtectedProcess : 1;
			BOOLEAN IsImageDynamicallyRelocated : 1;
			BOOLEAN SkipPatchingUser32Forwarders : 1;
			BOOLEAN IsPackagedProcess : 1;
			BOOLEAN IsAppContainer : 1;
			BOOLEAN IsProtectedProcessLight : 1;
			BOOLEAN SpareBits : 1;
		};
	};
	HANDLE Mutant;

	PVOID ImageBaseAddress;
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;
	PRTL_CRITICAL_SECTION FastPebLock;
	PVOID AtlThunkSListPtr;
	PVOID IFEOKey;
	union
	{
		ULONG CrossProcessFlags;
		struct
		{
			ULONG ProcessInJob : 1;
			ULONG ProcessInitializing : 1;
			ULONG ProcessUsingVEH : 1;
			ULONG ProcessUsingVCH : 1;
			ULONG ProcessUsingFTH : 1;
			ULONG ReservedBits0 : 27;
		};
		ULONG EnvironmentUpdateCount;
	};
	union
	{
		PVOID KernelCallbackTable;
		PVOID UserSharedInfoPtr;
	};
	ULONG SystemReserved[1];
	ULONG AtlThunkSListPtr32;
	PVOID ApiSetMap;
	ULONG TlsExpansionCounter;
	PVOID TlsBitmap;
	ULONG TlsBitmapBits[2];
	PVOID ReadOnlySharedMemoryBase;
	PVOID HotpatchInformation;
	PVOID *ReadOnlyStaticServerData;
	PVOID AnsiCodePageData;
	PVOID OemCodePageData;
	PVOID UnicodeCaseTableData;

	ULONG NumberOfProcessors;
	ULONG NtGlobalFlag;

	LARGE_INTEGER CriticalSectionTimeout;
	SIZE_T HeapSegmentReserve;
	SIZE_T HeapSegmentCommit;
	SIZE_T HeapDeCommitTotalFreeThreshold;
	SIZE_T HeapDeCommitFreeBlockThreshold;

	ULONG NumberOfHeaps;
	ULONG MaximumNumberOfHeaps;
	PVOID *ProcessHeaps;

	PVOID GdiSharedHandleTable;
	PVOID ProcessStarterHelper;
	ULONG GdiDCAttributeList;

	PRTL_CRITICAL_SECTION LoaderLock;

	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	USHORT OSBuildNumber;
	USHORT OSCSDVersion;
	ULONG OSPlatformId;
	ULONG ImageSubsystem;
	ULONG ImageSubsystemMajorVersion;
	ULONG ImageSubsystemMinorVersion;
	ULONG_PTR ImageProcessAffinityMask;
	GDI_HANDLE_BUFFER GdiHandleBuffer;
	PVOID PostProcessInitRoutine;

	PVOID TlsExpansionBitmap;
	ULONG TlsExpansionBitmapBits[32];

	ULONG SessionId;

	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppCompatFlagsUser;
	PVOID pShimData;
	PVOID AppCompatInfo;

	UNICODE_STRING CSDVersion;

	PVOID ActivationContextData;
	PVOID ProcessAssemblyStorageMap;
	PVOID SystemDefaultActivationContextData;
	PVOID SystemAssemblyStorageMap;

	SIZE_T MinimumStackCommit;

	PVOID *FlsCallback;
	LIST_ENTRY FlsListHead;
	PVOID FlsBitmap;
	ULONG FlsBitmapBits[FLS_MAXIMUM_AVAILABLE / (sizeof(ULONG) * 8)];
	ULONG FlsHighIndex;

	PVOID WerRegistrationData;
	PVOID WerShipAssertPtr;
	PVOID pContextData;
	PVOID pImageHeaderHash;
	union
	{
		ULONG TracingFlags;
		struct
		{
			ULONG HeapTracingEnabled : 1;
			ULONG CritSecTracingEnabled : 1;
			ULONG LibLoaderTracingEnabled : 1;
			ULONG SpareTracingBits : 29;
		};
	};
	ULONGLONG CsrServerReadOnlySharedMemoryBase;
} PEB, *PPEB;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,                // 0 Y N
	SystemProcessorInformation,            // 1 Y N
	SystemPerformanceInformation,        // 2 Y N
	SystemTimeOfDayInformation,            // 3 Y N
	SystemNotImplemented1,                // 4 Y N
	SystemProcessesAndThreadsInformation, // 5 Y N
	SystemCallCounts,                    // 6 Y N
	SystemConfigurationInformation,        // 7 Y N
	SystemProcessorTimes,                // 8 Y N
	SystemGlobalFlag,                    // 9 Y Y
	SystemNotImplemented2,                // 10 Y N
	SystemModuleInformation,            // 11 Y N
	SystemLockInformation,                // 12 Y N
	SystemNotImplemented3,                // 13 Y N
	SystemNotImplemented4,                // 14 Y N
	SystemNotImplemented5,                // 15 Y N
	SystemHandleInformation,            // 16 Y N
	SystemObjectInformation,            // 17 Y N
	SystemPagefileInformation,            // 18 Y N
	SystemInstructionEmulationCounts,    // 19 Y N
	SystemInvalidInfoClass1,            // 20
	SystemCacheInformation,                // 21 Y Y
	SystemPoolTagInformation,            // 22 Y N
	SystemProcessorStatistics,            // 23 Y N
	SystemDpcInformation,                // 24 Y Y
	SystemNotImplemented6,                // 25 Y N
	SystemLoadImage,                    // 26 N Y
	SystemUnloadImage,                    // 27 N Y
	SystemTimeAdjustment,                // 28 Y Y
	SystemNotImplemented7,                // 29 Y N
	SystemNotImplemented8,                // 30 Y N
	SystemNotImplemented9,                // 31 Y N
	SystemCrashDumpInformation,            // 32 Y N
	SystemExceptionInformation,            // 33 Y N
	SystemCrashDumpStateInformation,    // 34 Y Y/N
	SystemKernelDebuggerInformation,    // 35 Y N
	SystemContextSwitchInformation,        // 36 Y N
	SystemRegistryQuotaInformation,        // 37 Y Y
	SystemLoadAndCallImage,                // 38 N Y
	SystemPrioritySeparation,            // 39 N Y
	SystemNotImplemented10,                // 40 Y N
	SystemNotImplemented11,                // 41 Y N
	SystemInvalidInfoClass2,            // 42
	SystemInvalidInfoClass3,            // 43
	SystemTimeZoneInformation,            // 44 Y N
	SystemLookasideInformation,            // 45 Y N
	SystemSetTimeSlipEvent,                // 46 N Y
	SystemCreateSession,                // 47 N Y
	SystemDeleteSession,                // 48 N Y
	SystemInvalidInfoClass4,            // 49
	SystemRangeStartInformation,        // 50 Y N
	SystemVerifierInformation,            // 51 Y Y
	SystemAddVerifier,                    // 52 N Y
	SystemSessionProcessesInformation    // 53 Y N
} SYSTEM_INFORMATION_CLASS;

extern NTSTATUS  ZwQueryInformationProcess(
	      HANDLE           ProcessHandle,
	      PROCESSINFOCLASS ProcessInformationClass,
	     PVOID            ProcessInformation,
	      ULONG            ProcessInformationLength,
	 PULONG           ReturnLength
);

extern NTSTATUS __stdcall ZwOpenProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, PHANDLE TokenHandle);

extern NTSTATUS __stdcall ZwQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, SIZE_T Length, PSIZE_T ResultLength);

VOID CreateProcessCallback(
	 HANDLE ParentId,
	 HANDLE ProcessId,
	 BOOLEAN Create
);

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING uSymbolicLinkName = {0x00};
	RtlInitUnicodeString(&uSymbolicLinkName, SymbolicLinkName);
	IoDeleteSymbolicLink(&uSymbolicLinkName);
	IoDeleteDevice(pDriverObject->DeviceObject);

	PsSetCreateProcessNotifyRoutine(CreateProcessCallback, TRUE);
	DbgPrint
	("Unload ProcInfo!\n");
}

NTSTATUS DispatchCommon(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = ntStatus;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	DbgPrint("ProcInfo:DispatchCommon!\n");

	return ntStatus;
}

VOID Initialize(VOID)
{
	UNICODE_STRING uFuncName = {0x00};
	if (g_BuildNumber >= 0xece)
	{
		RtlInitUnicodeString(&uFuncName, L"ZwOpenProcessTokenEx");
		g_ZwOpenProcessTokenEx = (ZWOPENPROCESSTOKENEX)MmGetSystemRoutineAddress(&uFuncName);
	}
}

PVOID GetProcessToken(HANDLE TokenHandle, void *a2, ULONG *a3, PVOID a4, ULONG *ResultLength, PVOID a6)
{
	PVOID TokenInformation;
	PVOID pResult;
	HANDLE TokenHandle_v6;
	ULONG *LengthAddr;
	PVOID P = NULL;
	PVOID pTokenGroups = NULL;
	PVOID pTokenImpersonationLevel = NULL;

	TokenInformation = a2;
	TokenHandle_v6 = TokenHandle;
	if (a3)
		*a3 = 0;
	LengthAddr = ResultLength;
	if (ResultLength)
		*ResultLength = 0;
	if (a6)
		*(ULONG *)a6 = 0;
	if (ZwQueryInformationToken(TokenHandle, TokenUser, 0, 0, ResultLength) != 0xc0000023)
		return 0;
	P = ExAllocatePoolWithTag(NonPagedPool, *ResultLength, 'EKOT');
	if (!P)
		return 0;
	if (ZwQueryInformationToken(TokenHandle_v6, TokenUser, P, *ResultLength, ResultLength) < 0)
	{
		ExFreePoolWithTag(P, 0);
		P = NULL;
	}
	if (!a3)
		goto LABEL_17;

	if (ZwQueryInformationToken(TokenHandle_v6, TokenGroups, 0, 0, ResultLength) != 0xc0000023)
		return 0;

	pTokenGroups = ExAllocatePoolWithTag(NonPagedPool, *ResultLength, 'PGOT');

	*a3 = pTokenGroups;
	if (!pTokenGroups)
		return 0;
	if (ZwQueryInformationToken(TokenHandle_v6, TokenGroups, pTokenGroups, *ResultLength, ResultLength) < 0)
	{
		ExFreePoolWithTag(*a3, 0);
		*a3 = 0;
	}

LABEL_17:
	if (TokenInformation)
	{
		*ResultLength = 56;
		ZwQueryInformationToken(TokenHandle_v6, TokenStatistics, TokenInformation, 0x38, ResultLength);
	}

	if (a4 && ZwQueryInformationToken(TokenHandle_v6, (TOKEN_INFORMATION_CLASS)24, a4, 4, ResultLength) < 0)
		*(ULONG *)a4 = -1;
	if (LengthAddr)
	{
		if (ZwQueryInformationToken(TokenHandle_v6, TokenImpersonationLevel | 0x10, 0, 0, ResultLength) == 0xc0000023)
		{
			pTokenImpersonationLevel = ExAllocatePoolWithTag(NonPagedPool, *ResultLength, 'ELOT');
			if (pTokenImpersonationLevel)
			{
				if (ZwQueryInformationToken(TokenHandle_v6, TokenImpersonationLevel | 0x10, pTokenImpersonationLevel, *ResultLength, ResultLength) < 0)
				{
					ExFreePoolWithTag(pTokenImpersonationLevel, 0);
					pTokenImpersonationLevel = NULL;
				}
			}
		}
	}

	if (a6)
	{
		ZwQueryInformationToken(TokenHandle_v6, TokenSessionId | TokenUser | 0x10, a6, 4u, ResultLength);
	}
	return P;
}

HANDLE GetProcessHandle(HANDLE ProcessId, PVOID edx0,ULONG *a2, ULONG *a3, ULONG *ResultLength, PVOID ProcessInformation, PVOID a6)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES ObjectAttributes;
	LARGE_INTEGER TimeOut;
	CLIENT_ID ClinentId;
	HANDLE TokenHandle;
	HANDLE ProcessHandle;
	ULONG v15;
	PVOID pTokenUser = NULL;

	ProcessHandle = 0;
	v15 = -1;
	TimeOut.QuadPart = 0;
	if (ProcessId)
	{
		ClinentId.UniqueThread = 0;
		ClinentId.UniqueProcess = ProcessId;
		ObjectAttributes.Length = 24;
		ObjectAttributes.RootDirectory = 0;
		ObjectAttributes.Attributes = 512;
		ObjectAttributes.ObjectName = 0;
		ObjectAttributes.SecurityDescriptor = 0;
		ObjectAttributes.SecurityQualityOfService = 0;
		if (ZwOpenProcess(&ProcessHandle, 0, &ObjectAttributes, &ClinentId) < 0)
			return 0;
		if (g_BuildNumber < 0x1770 && ZwWaitForSingleObject(ProcessHandle, 0, &TimeOut) != STATUS_TIMEOUT)
		{
			ZwClose(ProcessHandle);
			return 0;
		}
		
		ZwQueryInformationProcess(ProcessHandle, ProcessSessionInformation, ProcessInformation, 4, NULL);
		if (g_ZwOpenProcessTokenEx)
			ntStatus = g_ZwOpenProcessTokenEx(ProcessHandle, 131080, 512, &TokenHandle);
		else
			ntStatus = ZwOpenProcessToken(ProcessHandle, 0x20008, &TokenHandle);
		if (NT_SUCCESS(ntStatus))
		{
			pTokenUser = GetProcessToken(TokenHandle, edx0, a3, -1, ResultLength, a6);
			*a2 = pTokenUser;
			ZwClose(TokenHandle);
		}
	}

	return ProcessHandle;
}

NTSTATUS GetProcessInfoDataByPid(HANDLE ProcessId, ULONG a2, ULONG a3, ULONG a4)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG arg2;
	PVOID arg3 = NULL;
	PVOID arg4 = NULL;
	ULONG arg5;
	ULONG arg6;
	HANDLE ProcessHandle;
	PROCESS_BASIC_INFORMATION ProcessInformation;
	LARGE_INTEGER ProcessTime;
	LARGE_INTEGER Subtrahend;
	UINT_PTR ResultLength;

	ProcessHandle = GetProcessHandle(ProcessId, &arg2, (int)&arg3, 0, (ULONG)arg4, &arg5,&arg6);
	if (!ProcessHandle)
		return 0xc000000d;
	ntStatus = ZwQueryInformationProcess(ProcessHandle, ProcessBasicInformation, &ProcessInformation, 0x18, 0);

	if (NT_SUCCESS(ntStatus))
	{
		//if ( !sub_10003ED0(ProcessHandle, Address, (int)&DestinationString, (int)&v42, (int)&v46, v29, v30) )
			//sub_10003CD0(v9, &DestinationString);

		ProcessTime.QuadPart = 0;
		ZwQueryInformationProcess(ProcessHandle, ProcessTimes, &ProcessTime, 0x20, 0);
		if (!ProcessTime.QuadPart)
		{
			ResultLength = 48;
			if (ZwQuerySystemInformation(SystemTimeOfDayInformation, &ProcessTime, 0x30, &ResultLength) >= 0)
			{
				Subtrahend.QuadPart = (ProcessTime.QuadPart) & 0xffff0000;
				ProcessTime.QuadPart -= Subtrahend.QuadPart;
			}
				

		}
	}

	return ntStatus;
}

VOID CreateProcessCallback(
	 HANDLE ParentId,
	 HANDLE ProcessId,
	 BOOLEAN Create
)
{
	KIRQL kCurrentIrql;
	UNICODE_STRING uProcessName = { 0x00 };
	UNICODE_STRING uHomePageUrlStr = { 0x00 };
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING uCompStr = { 0x00 };
	PPEB pPeb = NULL;
	PEPROCESS pEprocess = NULL;
	UNICODE_STRING uCommandLineBuffer = { 0x00 };
	KAPC_STATE pKapcState = { 0X00 };
	NTSTRSAFE_PCWSTR lpHomePageUrlStr = L"\" https://www.pediy.com";
	ULONG dwEvelatedId = 0;
	ULONG dwScodeIndex = 0;
	ULONG dwUrlIndex = 0;
	UNICODE_STRING uScodefStr = { 0x00 };

	kCurrentIrql = KeGetCurrentIrql();
	if (kCurrentIrql >= DISPATCH_LEVEL)
	{
		//如果当前irql大于等于DISPATCH_LEVEL时，直接返回
		return;
	}
	
	if (Create)
	{		
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
			
			if (!pPeb->ProcessParameters)
			{
				//ntStatus = STATUS_INVALID_PARAMETER;
				//__leave;
			}
			
			GetProcessInfoDataByPid(ProcessId, 0, 1, 0);

			DbgPrint("进程id:%d\n镜像文件:%wZ\n命令行:%wZ\n当前目录:%wZ\nSessionId:%d\n"
				, ProcessId
				,&pPeb->ProcessParameters->ImagePathName
				,&pPeb->ProcessParameters->CommandLine
				,&pPeb->ProcessParameters->CurrentDirectoryPath
				,pPeb->SessionId);
			DbgPrint("pImageHeaderHash Address is 0x%x!\n", pPeb->pImageHeaderHash);
			
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

VOID CreateCallBack(VOID)
{
	PsSetCreateProcessNotifyRoutine(CreateProcessCallback, FALSE);
}

VOID GetNotExportFunctions(VOID)
{
	UNICODE_STRING uFuncsName = { 0x00 };
	RtlInitUnicodeString(&uFuncsName, PsGetProcessPebName);
	PsGetProcessPeb = (PSGETPROCESSPEB)MmGetSystemRoutineAddress(&uFuncsName);

	RtlInitUnicodeString(&uFuncsName, PsIsProtectedProcessName);
	PsIsProtectedProcess = (PSISPROTECTEDPROCESS)MmGetSystemRoutineAddress(&uFuncsName);

}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING uDeviceName = {0x00};
	UNICODE_STRING uSymbolicLinkName = {0x00};
	PDEVICE_OBJECT pDeviceObject = NULL;
	ULONG i;
	RtlInitUnicodeString(&uDeviceName, DeviceName);
	ntStatus = IoCreateDevice(pDriverObject
		, 0
		, &uDeviceName
		, 0
		, 0
		, FALSE
		, &pDeviceObject
	);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoCreateDevice fail:%d!\n", ntStatus);
		return ntStatus;
	}

	RtlInitUnicodeString(&uSymbolicLinkName, SymbolicLinkName);
	ntStatus = IoCreateSymbolicLink(&uSymbolicLinkName, &uDeviceName);
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = DispatchCommon;
	}

	pDriverObject->DriverUnload = DriverUnload;

	GetNotExportFunctions();

	PsGetVersion(NULL, NULL, &g_BuildNumber, NULL);

	Initialize();

	CreateCallBack();

	DbgPrint("ProcInfo DriverEntry!\n");

	return ntStatus;
}