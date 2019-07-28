#include <ntifs.h>        //NT文件文件系统头文件
#include <ntddk.h>        //NT设备驱动头文件
#include <ntstrsafe.h>    //字符串安全函数
#include <windef.h>       //基本windows定义
#include <ntimage.h>      //文件格式
#include "Ioctlcmd.h"
#include "main.h"
#include "hook.h"
#include "misc.h"

NTSTATUS
NTAPI
ZwQueryInformationProcess(
						  __in HANDLE ProcessHandle,
						  __in PROCESSINFOCLASS ProcessInformationClass,
						  __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
						  __in ULONG ProcessInformationLength,
						  __out_opt PULONG ReturnLength
    );