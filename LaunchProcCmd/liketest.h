#ifndef _IMAGENOTIFYDLL_H
#define _IMAGENOTIFYDLL_H 1

#include <devioctl.h>
#include <ntddk.h>

#define DEVICE_NAME L"\\Device\\liketest"     // Driver Name
#define LINK_NAME   L"\\DosDevices\\liketest"    // Link Name

//
// The device driver IOCTLs
//

#define IOCTL_BASE	0x800
#define MY_CTL_CODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)


///////////////////////////////////////////////////////////////////////////////

//NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString);
//NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp);
//NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp);
//VOID DriverUnload(PDRIVER_OBJECT pDriverObj);
//NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp);

//NTSTATUS PsSetCreateProcessNotifyRoutineEx(
//	_In_ PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine,
//	_In_ BOOLEAN                           Remove
//	);

///////////////////////////////////////////////////////////////////////////////
#endif

