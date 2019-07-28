
#ifndef _MAIN_H_
#define _MAIN_H_
const WCHAR deviceLinkBuffer[]  = L"\\DosDevices\\Delfile";
const WCHAR deviceNameBuffer[]  = L"\\Device\\Delfile";
typedef unsigned long DWORD;
#define SystemHandleInformation 16
#define INVALID_PID_VALUE 0xFFFFFFFF
#define FILE_DEVICE_SWAP     0x0000800a

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
USHORT UniqueProcessId;
USHORT CreatorBackTraceIndex;
UCHAR ObjectTypeIndex;
UCHAR HandleAttributes;
USHORT HandleValue;
PVOID Object;
ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
ULONG NumberOfHandles;
SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;


NTSTATUS
ObQueryNameString(
    IN PVOID  Object,
    OUT POBJECT_NAME_INFORMATION  ObjectNameInfo,
    IN ULONG  Length,
    OUT PULONG  ReturnLength
    ); 

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(   
       ULONG    SystemInformationClass,
       PVOID    SystemInformation,
       ULONG    SystemInformationLength,
       PULONG    ReturnLength
       );
NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
      IN HANDLE SourceProcessHandle,
      IN HANDLE SourceHandle,
      IN HANDLE TargetProcessHandle OPTIONAL,
      OUT PHANDLE TargetHandle OPTIONAL,
      IN ACCESS_MASK DesiredAccess,
      IN ULONG HandleAttributes,
      IN ULONG Options
      );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess(    
     OUT PHANDLE             ProcessHandle,
     IN ACCESS_MASK          AccessMask,
     IN POBJECT_ATTRIBUTES   ObjectAttributes,
     IN PCLIENT_ID           ClientId
     );

/* The file name looks like L"\\??\\C:\\hello.doc" */
BOOLEAN dfDelFile(WCHAR* name);

#endif

