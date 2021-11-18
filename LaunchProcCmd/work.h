#include <ntddk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wdm.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PATH 256
#define FLS_MAXIMUM_AVAILABLE 128
#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60

#ifndef WIN64
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE32
#else
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE64
#endif

#define ARRAYSIZEOF(x)	sizeof (x) / sizeof (x[0])

	typedef ULONG GDI_HANDLE_BUFFER[GDI_HANDLE_BUFFER_SIZE];

	VOID SetWork(IN BOOLEAN bState);

	//结构体
	typedef struct _KAPC_STATE {
		LIST_ENTRY ApcListHead[MaximumMode];//线程的apc链表 只有两个 内核态和用户态
		struct _KPROCESS *Process; //线程 挂靠的进程
		BOOLEAN KernelApcInProgress;    //线程正在处理APC对象
		BOOLEAN KernelApcPending;    //线程有内核apc等待交付
		BOOLEAN UserApcPending;     //有用户态的等待
	} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;

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
	} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;




	typedef struct _RTL_CRITICAL_SECTION *PRTL_CRITICAL_SECTION;

	typedef struct _PEB_LDR_DATA
	{
		ULONG Length;
		BOOLEAN Initialized;
		HANDLE SsHandle;
		LIST_ENTRY InLoadOrderModuleList;
		LIST_ENTRY InMemoryOrderModuleList;
		LIST_ENTRY InInitializationOrderModuleList;
		PVOID EntryInProgress;
		BOOLEAN ShutdownInProgress;
		HANDLE ShutdownThreadId;
	} PEB_LDR_DATA, *PPEB_LDR_DATA;

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


	NTSTATUS PsLookupProcessByProcessId(__in HANDLE ulProcId, __out PEPROCESS * Process);
	NTSTATUS PsReferenceProcessFilePointer(IN PEPROCESS pProcess, OUT PFILE_OBJECT* ppFileObject);
	VOID KeStackAttachProcess(_Inout_ PRKPROCESS   Process, _Out_   PRKAPC_STATE ApcState);
	VOID KeUnstackDetachProcess(_In_ PRKAPC_STATE ApcState);
	PPEB PsGetProcessPeb(PEPROCESS Process);
	NTSTATUS NtTerminateProcess(IN HANDLE ProcessHandle, IN NTSTATUS ExitStatus);
	NTSTATUS ObOpenObjectByPointer(
		__in PVOID Object,
		__in ULONG HandleAttributes,
		__in_opt PACCESS_STATE PassedAccessState,
		__in ACCESS_MASK DesiredAccess,
		__in_opt POBJECT_TYPE  ObjectType,
		__in KPROCESSOR_MODE AccessMode,
		__out PHANDLE Handle);
	NTKERNELAPI	UCHAR *	PsGetProcessImageFileName(__in PEPROCESS Process);

#ifdef __cplusplus
}; // extern "C"
#endif