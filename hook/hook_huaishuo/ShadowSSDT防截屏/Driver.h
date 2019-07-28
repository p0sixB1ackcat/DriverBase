#ifdef __cplusplus
extern "C"
{
#endif
/*加载头文件*/
///////////////////
#include <Ntifs.h>
#include <ntddk.h>
#include <windef.h>
//////////////////
#ifdef __cplusplus
}
#endif

#define IOCTL_BASE	0x800
#define MY_CTL_CODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HELLO	MY_CTL_CODE(0)


#define DEVICE_NAME L"\\Device\\MyShaDow"
#define LINE_NAME L"\\??\\MyShaDow"                //"\\dosdevices\\HelloDDK"

#define DELAY_ONE_MICROSECOND (-10)
#define DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)

#define PAGEDCODE  code_seg("PAGE")   //代码放到分页内存
#define LOCKEDCODE code_seg()
#define INITCODE   code_seg("INIT")

#define PAGEDDATA  data_seg("PAGE")   //数据放到分页内存
#define LOCKEDDATA data_seg()
#define INITDATA   data_seg("INIT")

//求数组的大小
#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

//设置扩展结构体
typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT pDevice;         //设备句柄
	UNICODE_STRING ustrDeviceName;	//设备名称
	UNICODE_STRING ustrSymLinkName;	//符号链接名
}DEVICE_EXTENSION,*PDEVICE_EXTENSION;


///////////////////////////////////////////////////////////////////////////////////////////
//SSDT表结构
typedef struct ServiceDescriptorEntry 
{
	PVOID *ServiceTableBase;
	ULONG *ServiceCounterTableBase; //Used only in checked build
	ULONG NumberOfServices;
	PVOID *ParamTableBase;
} ServiceDescriptorTableEntry, *PServiceDescriptorTableEntry;

//ShadowSSDT表的地址
PServiceDescriptorTableEntry KeServiceDescriptorTableShadow = NULL;



#define ObjectNameInformation  1          

#define SystemHandleInformation 0x10

typedef struct _SYSTEM_HANDLE_INFORMATION {
	ULONG ProcessId;
	UCHAR ObjectTypeNumber;
	UCHAR Flags;
	USHORT Handle;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
} _SYSTEM_HANDLE_INFORMATION, *P_SYSTEM_HANDLE_INFORMATION;


typedef struct _SYSTEM_HANDLE_INformATION_EX {
	ULONG NumberOfHandles;
	_SYSTEM_HANDLE_INFORMATION Information[1];
} _SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////

/*
ULONG KdPrintEx(
_In_  ULONG ComponentId,
_In_  ULONG Level,
_In_  PCSTR Format,
... arguments
);

ComponentId：指定调用此例程的组件
	DPFLTR_IHVVIDEO_ID：    视频驱动程序
	DPFLTR_IHVAUDIO_ID：    音频驱动程序
	DPFLTR_IHVNETWORK_ID：  网络驱动程序
	DPFLTR_IHVSTREAMING_ID：内核流驱动程序
	DPFLTR_IHVBUS_ID：      总线驱动程序
	DPFLTR_IHVDRIVER_ID：   任何其他类型的驱动程序


Level：消息重要性位域与 ComponentId 指定的组件的筛选器掩码相比较
*/

NTSTATUS DriverEntry( IN PDRIVER_OBJECT pDriverObject,IN PUNICODE_STRING pRegistryPath);

//卸载历程
void DriverUnload(IN PDRIVER_OBJECT pDriverObject);
//默认卸载历程
NTSTATUS DDKDispatchRoutine(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
//创建设备
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject);