#include "precomp.h"

#pragma pack(1)
typedef struct ServiceDescriptorEntry {
    unsigned int *ServiceTableBase;
    unsigned int *ServiceCounterTableBase; //Used only in checked build
    unsigned int NumberOfServices;
    unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()

__declspec(dllimport)  ServiceDescriptorTableEntry_t KeServiceDescriptorTable;
#define SYSTEMSERVICE(_function) KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)_function+1)]
#define SDT     SYSTEMSERVICE
#define KSDT KeServiceDescriptorTable

void StartHook(void);
void RemoveHook(void);


#define SystemLoadAndCallImage 38
#define SystemLoadImage        26

typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE 
{ 
	UNICODE_STRING ModuleName;    //模块名称
} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE; 


//LPC消息类型
typedef enum _LPC_MSG_TYPE

{
	LPC_NEW_MSG,
	LPC_REQUEST,     //当客户端使用 NtRequestWaitReplyPort() 函数发送请求时，服务端接收此消息
	LPC_REPLY,       //当服务器回复此请求时，客户从 NtRequestWaitReplyPort() 函数接收此类消息
	LPC_DATAGRAM,
	LPC_LOST_REPLY,
	LPC_PORT_CLOSED, //当客户端关闭端口句柄时，服务端接收此消息
	LPC_CLIENT_DIED, //当客户端退出时，服务端接受此程序
	LPC_EXCEPTION,
	LPC_DEBUG_EVENT,
	LPC_ERROR_EVENT,
	LPC_CONNECTION_REQUEST, //当客户端调用NtConnectPort或者NtSecureConnectPort连接端口时，相应的服务端接收此消息
} LPC_MSG_TYPE;


//LPC消息结构体
typedef struct LpcMessage 
{
	WORD         ActualMessageLength;
	WORD         TotalMessageLength;
	DWORD        MessageType;
	DWORD        ClientProcessId;
	DWORD        ClientThreadId;
	DWORD        MessageId;
	DWORD        SharedSectionSize;
	BYTE         MessageData[1];      //变长结构题
} LPCMESSAGE, *PLPCMESSAGE;

typedef struct _LPCP_NONPAGED_PORT_QUEUE
{
	KSEMAPHORE Semaphore;
	PVOID BackPointer;
} LPCP_NONPAGED_PORT_QUEUE, *PLPCP_NONPAGED_PORT_QUEUE;

typedef struct _LPCP_PORT_QUEUE
{
	PLPCP_NONPAGED_PORT_QUEUE NonPagedPortQueue;
	PKSEMAPHORE Semaphore;
	LIST_ENTRY ReceiveHead;
} LPCP_PORT_QUEUE, *PLPCP_PORT_QUEUE;

typedef struct _LPCP_PORT_OBJECT
{
	struct _LPCP_PORT_OBJECT* ConnectionPort;
	struct _LPCP_PORT_OBJECT* ConnectedPort;
	LPCP_PORT_QUEUE MsgQueue;
	CLIENT_ID Creator;
	PVOID ClientSectionBase;
	PVOID ServerSectionBase;
	PVOID PortContext;
	PETHREAD ClientThread;
	SECURITY_QUALITY_OF_SERVICE SecurityQos;
	SECURITY_CLIENT_CONTEXT StaticSecurity;
	LIST_ENTRY LpcReplyChainHead;
	LIST_ENTRY LpcDataInfoChainHead;
	union
	{
		PEPROCESS ServerProcess;
		PEPROCESS MappingProcess;
	};
	WORD MaxMessageLength;
	WORD MaxConnectionInfoLength;
	ULONG Flags;
	KEVENT WaitEvent;
} LPCP_PORT_OBJECT, *PLPCP_PORT_OBJECT;




//////////////////////////////////////////////////////////////HOOK
//原函数不直接声明了，因为WIN7 Xp的SSDT表不完全一样，所以函数地址动态获取
/*
NTKERNELAPI NTSTATUS ZwLoadDriver(
  IN PUNICODE_STRING DriverServiceName );


//根据ZW函数获取NT地址
NTSYSAPI NTSTATUS NTAPI ZwSetSystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN SIZE_T SystemInformationLength
	);

NTSYSAPI NTSTATUS NTAPI ZwRequestWaitReplyPort(
	IN HANDLE PortHandle,
	OUT PLPCMESSAGE LpcRequest,
	IN PLPCMESSAGE LpcReply
	);


NTSTATUS NTAPI ZwAlpcSendWaitReceivePort(
	HANDLE PortHandle,
	DWORD SendFlags,
	PLPCMESSAGE SendMessage ,
	PVOID InMessageBuffer ,
	PLPCMESSAGE ReceiveBuffer ,
	PULONG ReceiveBufferSize ,
	PVOID OutMessageBuffer ,
	PLARGE_INTEGER Timeout);
*/

//HOOK函数
NTSTATUS Hook_ZwLoadDriver(
  IN PUNICODE_STRING DriverServiceName
  );

NTSTATUS NTAPI HOOK_NtSetSystemInformation
	(
	IN ULONG SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength
	);

NTSTATUS NTAPI HOOK_NtRequestWaitReplyPort(
	IN HANDLE PortHandle,
	OUT PLPCMESSAGE LpcReply,
	IN PLPCMESSAGE LpcRequest
	);

NTSTATUS NTAPI HOOK_NTAlpcSendWaitReceivePort(
	HANDLE PortHandle,
	DWORD SendFlags,
	PLPCMESSAGE SendMessage ,
	PVOID InMessageBuffer ,
	PLPCMESSAGE ReceiveBuffer ,
	PULONG ReceiveBufferSize ,
	PVOID OutMessageBuffer ,
	PLARGE_INTEGER Timeout);

//////////////////////////////////////////////////////////////HOOK

//函数指针
typedef NTSTATUS (*ZWLOADDRIVER)(
  IN PUNICODE_STRING DriverServiceName );

typedef NTSTATUS (*NtSETSYSTEMINFORMATION)
	(
	IN ULONG SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength
	);

typedef NTSTATUS (*NtREQUESTWAITREPLYPORT)(
	IN HANDLE PortHandle,
	OUT PLPCMESSAGE LpcRequest,
	IN PLPCMESSAGE LpcReply
	);

typedef NTSTATUS  (*NTALPCSENDWAITRECEIVEPORT)(
	HANDLE PortHandle,
	DWORD SendFlags,
	PLPCMESSAGE SendMessage ,
	PVOID InMessageBuffer ,
	PLPCMESSAGE ReceiveBuffer ,
	PULONG ReceiveBufferSize ,
	PVOID OutMessageBuffer ,
	PLARGE_INTEGER Timeout);

//ZW函数地址，用来获取SSDT表中NT函数地址
static ZWLOADDRIVER              g_pZwLoadDriver = NULL;
static NtSETSYSTEMINFORMATION    g_pZwSetSystemInformation = NULL;
static NtREQUESTWAITREPLYPORT    g_pZwRequestWaitReplyPort = NULL;
static NTALPCSENDWAITRECEIVEPORT g_pZwAlpcSendWaitReceivePort = NULL;

//NT函数原地址
static ZWLOADDRIVER              g_pOldZwLoadDriver = NULL;
static NtSETSYSTEMINFORMATION    g_pOldNtSetSystemInformation = NULL;
static NtREQUESTWAITREPLYPORT    g_pOldNtRequestWaitReplyPort = NULL;
static NTALPCSENDWAITRECEIVEPORT g_pOldNTAlpcSendWaitReceivePort = NULL;


static HANDLE g_Pid = 0;    //加载驱动的进程PID


//正则匹配
BOOLEAN IsPatternMatch(PUNICODE_STRING Expression, PUNICODE_STRING Name, BOOLEAN IgnoreCase)
{
	return FsRtlIsNameInExpression(
		Expression,
		Name,
		IgnoreCase,//如果这里设置为TRUE,那么Expression必须是大写的
		NULL
		); 
}

NTSTATUS Hook_ZwLoadDriver(IN PUNICODE_STRING DriverServiceName )
{
	UNICODE_STRING			uPath						= {0};
	NTSTATUS				status						= STATUS_SUCCESS;
	BOOL					skipOriginal				= FALSE;
	WCHAR					szTargetDriver[MAX_PATH]	= {0};
	WCHAR					szTarget[MAX_PATH]			= {0};
	R3_RESULT				CallBackResult				= R3Result_Pass;
	WCHAR					wszPath[MAX_PATH]			= {0};
	UNICODE_STRING  usDestPath = {0};
	WCHAR				wszProcessPath[MAX_PATH] = {0};
	ULONG_PTR ulPtr = 0;
	UNICODE_STRING uExpression;
	DECLARE_UNICODE_STRING_SIZE(StrProcessName, 260);
	__try
	{
		UNICODE_STRING CapturedName;
		
		if((ExGetPreviousMode() == KernelMode) || 
			(DriverServiceName == NULL))
		{
			skipOriginal = TRUE;
			status = g_pOldZwLoadDriver(DriverServiceName);
			return status;
		}
		
		uPath.Length = 0;
		uPath.MaximumLength = MAX_PATH * sizeof(WCHAR);
		uPath.Buffer = wszPath;
		
		
		CapturedName = ProbeAndReadUnicodeString(DriverServiceName);
		
		//内存来自应用层，所以得测试是否可读
		ProbeForRead(CapturedName.Buffer, 
			CapturedName.Length,
			sizeof(WCHAR));
		
		RtlCopyUnicodeString(&uPath, &CapturedName);
		
		if(ntGetDriverImagePath(&uPath, szTargetDriver))
		{
			
// 			if(ntIsDosDeviceName(szTargetDriver))
// 			{
// 				if( ntGetNtDeviceName(szTargetDriver, 
// 					szTarget))
// 				{
// 					RtlStringCbCopyW(szTargetDriver, 
// 						sizeof(szTargetDriver), 
// 						szTarget);
// 				}
// 			}
			
			DbgPrint("Driver:%ws will be loaded\n", szTargetDriver);
			//修改弹窗函数，加入目标路径
			usDestPath.Buffer = szTargetDriver;
			usDestPath.Length = wcslen(szTargetDriver)*sizeof(WCHAR);
			usDestPath.MaximumLength = sizeof(szTargetDriver)*sizeof(WCHAR);
			RtlInitUnicodeString(&uExpression, L"*SERVICES.EXE");
			ulPtr = (ULONG_PTR)PsGetCurrentProcessId();
			GetProcessFullNameByPid((HANDLE)ulPtr, &StrProcessName);
			//简单的就先做下正则匹配哦
			if(IsPatternMatch(&uExpression, &StrProcessName, TRUE))
			{
				ulPtr=(ULONG_PTR)g_Pid;
			}
			CallBackResult = GetResultFromUser((HANDLE)ulPtr,&usDestPath);
			if (CallBackResult == R3Result_Block)
			{
				return STATUS_ACCESS_DENIED;
			}
			
			skipOriginal = TRUE;
			status = g_pOldZwLoadDriver(DriverServiceName);
			return status;
		}	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		
	}
	
	if(skipOriginal)
		return status;
	
	return g_pOldZwLoadDriver(DriverServiceName);
}


NTSTATUS NTAPI HOOK_NtSetSystemInformation
	(
	IN ULONG SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength
	)
{

	R3_RESULT CallBackResult = R3Result_Pass;
	PSYSTEM_LOAD_AND_CALL_IMAGE pLoadImg = NULL;
	ULONG_PTR ulPtr = 0;
	UNICODE_STRING uExpression;
	DECLARE_UNICODE_STRING_SIZE(StrProcessName, 260);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"HOOK_NtSetSystemInformation  enter Class=%d\n", SystemInformationClass));
	if (SystemInformationClass == SystemLoadAndCallImage ||
		SystemInformationClass == SystemLoadImage)
	{
		pLoadImg = (PSYSTEM_LOAD_AND_CALL_IMAGE)SystemInformation;
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"HOOK_NtSetSystemInformation  loadDriver Class=%d img=%wZ\n", &(pLoadImg->ModuleName)));
		//效验当前加载进程ID
		RtlInitUnicodeString(&uExpression, L"*SERVICES.EXE");
		ulPtr = (ULONG_PTR)PsGetCurrentProcessId();
		GetProcessFullNameByPid((HANDLE)ulPtr, &StrProcessName);
		if(IsPatternMatch(&uExpression, &StrProcessName, TRUE))
		{
			ulPtr=(ULONG_PTR)g_Pid;
		}
		//弹框
		CallBackResult = GetResultFromUser((HANDLE)ulPtr,NULL);
		if (CallBackResult == R3Result_Block)
		{
			return STATUS_ACCESS_DENIED;
		}
	}
	if (g_pOldNtSetSystemInformation != NULL)
	{
		return g_pOldNtSetSystemInformation(SystemInformationClass,
			                       SystemInformation,
								   SystemInformationLength);
	}
	return STATUS_FAILED_DRIVER_ENTRY;
}


NTSTATUS NTAPI HOOK_NtRequestWaitReplyPort(
	IN HANDLE PortHandle,
	OUT PLPCMESSAGE LpcRequest,
	IN PLPCMESSAGE LpcReply
	)
{
	
	ULONG *ptr = NULL;
	ULONG i = 0, uactLength = 0;
	PLPCP_PORT_OBJECT  LPCProt = NULL;
	DECLARE_UNICODE_STRING_SIZE (uRealName, 256);
	DECLARE_UNICODE_STRING_SIZE (uLpcName, 256);
	RtlInitUnicodeString(&uRealName,L"\\RPC Control\\ntsvcs");
	ObReferenceObjectByHandle(PortHandle,(ACCESS_MASK)PROCESS_ALL_ACCESS,NULL,KernelMode,(PVOID *)&LPCProt,NULL);//获取对象
	ObQueryNameString(LPCProt->ConnectionPort,(POBJECT_NAME_INFORMATION)&uLpcName,256,&uactLength);
	if(!(RtlCompareUnicodeString(&uLpcName,&uRealName,TRUE)))  //操作的设备是加载驱动的
	{
		ptr=(ULONG *)(LpcRequest->MessageData);
		/*
		for (i=0; i<LpcReply->ActualMessageLength/sizeof(ULONG); i++)
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"%x ", ptr[i])); //输出数据
		}
		*/
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"LPCNAME:%wZ\n",&uLpcName));
		//if(ptr[0]==0x01&&ptr[1]==0x1f0241)//有点问题
		if(ptr[1]==0x1f0241)
		{
			//保存加载进程信息（PID）
			g_Pid=PsGetCurrentProcessId();
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"DriverLoad PID=%d\n",g_Pid));
		}
	}
	else
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"LPCNAME:%wZ\n",&uLpcName));
	}
	ObDereferenceObject(LPCProt);
	return g_pOldNtRequestWaitReplyPort(PortHandle,LpcReply,LpcRequest);
}

NTSTATUS NTAPI HOOK_NTAlpcSendWaitReceivePort(
	HANDLE PortHandle,
	DWORD SendFlags,
	PLPCMESSAGE SendMessage ,
	PVOID InMessageBuffer ,
	PLPCMESSAGE ReceiveBuffer ,
	PULONG ReceiveBufferSize ,
	PVOID OutMessageBuffer ,
	PLARGE_INTEGER Timeout)
{
	ULONG *ptr = NULL;
	ULONG i = 0, uactLength = 0;
	PLPCP_PORT_OBJECT  LPCProt = NULL;

	//测试了半天，ObReferenceObjectByHandle 处理不好有时蓝屏，尝试不处理，直接拿到PID，结果也正确

	//DECLARE_UNICODE_STRING_SIZE (uRealName, 256);
	//DECLARE_UNICODE_STRING_SIZE (uLpcName, 256);
	if (SendMessage)
	{
		/*
		RtlInitUnicodeString(&uRealName,L"\\RPC Control\\ntsvcs");
		ObReferenceObjectByHandle(PortHandle,(ACCESS_MASK)PROCESS_ALL_ACCESS,NULL,KernelMode,(PVOID *)&LPCProt,NULL);//获取对象
		ObQueryNameString(LPCProt->ConnectionPort,(POBJECT_NAME_INFORMATION)&uLpcName,1024,&uactLength);
		/*
		ptr=(ULONG *)(SendMessage->MessageData);
		if(ptr[1]==0x1f0241 && !(RtlCompareUnicodeString(&uLpcName,&uRealName,TRUE)))  //操作的设备是加载驱动的
		{

			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"LPCNAME:%wZ\n",&uLpcName));
			//if(ptr[0]==0x01&&ptr[1]==0x1f0241)//有点问题
			//保存加载进程信息（PID）
			g_Pid=PsGetCurrentProcessId();
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"DriverLoad PID=%d\n",g_Pid));
		}
		else
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,"LPCNAME:%wZ\n",&uLpcName));
		}
		ObDereferenceObject(LPCProt);
		*/
		g_Pid=PsGetCurrentProcessId();
	}

	return g_pOldNTAlpcSendWaitReceivePort(PortHandle,SendFlags,SendMessage,InMessageBuffer, ReceiveBuffer, ReceiveBufferSize, OutMessageBuffer, Timeout);
}

void StartHook (void)
{
    //获取未导出的服务函数索引号
	UNICODE_STRING uInterface ={0};
    ULONG  ulByteReaded;
	ULONG majorVersion, minorVersion;
	//获得操作系统版本
	PsGetVersion( &majorVersion, &minorVersion, NULL, NULL );
    __asm
    {
        push    eax
        mov        eax, CR0
        and        eax, 0FFFEFFFFh
        mov        CR0, eax
        pop        eax
    }
	//XP SSDT表中没有 NtAlpcSendWaitReceivePort  WIN7 调用 NTRequestWaitReplyPort 也无法获得，只能分开处理
	RtlInitUnicodeString (&uInterface, L"ZwLoadDriver"); 
	g_pZwLoadDriver = (ZWLOADDRIVER) MmGetSystemRoutineAddress(&uInterface);
	RtlInitUnicodeString (&uInterface, L"ZwSetSystemInformation"); 
	g_pZwSetSystemInformation = (NtSETSYSTEMINFORMATION) MmGetSystemRoutineAddress(&uInterface);
	if (majorVersion == 5 && minorVersion == 1 )
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL, "comint32: Running on Windows XP\n"));
		RtlInitUnicodeString (&uInterface, L"ZwRequestWaitReplyPort"); 
		g_pZwRequestWaitReplyPort = (NtREQUESTWAITREPLYPORT) MmGetSystemRoutineAddress(&uInterface);
	}
	else if(majorVersion == 6 && minorVersion == 1)
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID,DPFLTR_ERROR_LEVEL, "comint32: Running on Windows 7\n"));
		RtlInitUnicodeString (&uInterface, L"ZwAlpcSendWaitReceivePort"); 
		g_pZwAlpcSendWaitReceivePort = (NTALPCSENDWAITRECEIVEPORT) MmGetSystemRoutineAddress(&uInterface);
	}

	if (g_pZwLoadDriver != NULL)
	{
		 g_pOldZwLoadDriver = (ZWLOADDRIVER)InterlockedExchange((PLONG)&SDT(g_pZwLoadDriver),(LONG)Hook_ZwLoadDriver);
	}
	if (g_pZwSetSystemInformation != NULL)
	{
		g_pOldNtSetSystemInformation = (NtSETSYSTEMINFORMATION)InterlockedExchange((PLONG)&SDT(g_pZwSetSystemInformation), (LONG)HOOK_NtSetSystemInformation);
	}
	if (g_pZwRequestWaitReplyPort != NULL)
	{
		g_pOldNtRequestWaitReplyPort = (NtREQUESTWAITREPLYPORT)InterlockedExchange((PLONG)&SDT(g_pZwRequestWaitReplyPort), (LONG)HOOK_NtRequestWaitReplyPort);
	}
	if (g_pZwAlpcSendWaitReceivePort != NULL)
	{
		g_pOldNTAlpcSendWaitReceivePort = (NTALPCSENDWAITRECEIVEPORT)InterlockedExchange((PLONG)&SDT(g_pZwAlpcSendWaitReceivePort), (LONG)HOOK_NTAlpcSendWaitReceivePort);
	}
    //关闭
    __asm
    {
        push    eax
        mov     eax, CR0
        or      eax, NOT 0FFFEFFFFh
        mov     CR0, eax
        pop     eax
    }
    return ;
}

void RemoveHook (void)
{
    __asm
    {
        push    eax
        mov     eax, CR0
        and     eax, 0FFFEFFFFh
        mov     CR0, eax
        pop     eax
    }
	if (g_pZwLoadDriver != NULL && g_pOldZwLoadDriver != NULL)
	{
		InterlockedExchange( (PLONG) &SDT(g_pZwLoadDriver),  (LONG) g_pOldZwLoadDriver);
	}
	if (g_pZwSetSystemInformation != NULL && g_pOldNtSetSystemInformation != NULL)
	{
		InterlockedExchange( (PLONG) &SDT(g_pZwSetSystemInformation),  (LONG) g_pOldNtSetSystemInformation);
	}
	if (g_pZwRequestWaitReplyPort != NULL && g_pOldNtRequestWaitReplyPort != NULL)
	{
		InterlockedExchange( (PLONG) &SDT(g_pZwRequestWaitReplyPort),  (LONG) g_pOldNtRequestWaitReplyPort);
	}
    if (g_pZwAlpcSendWaitReceivePort != NULL && g_pOldNTAlpcSendWaitReceivePort != NULL)
    {
		InterlockedExchange( (PLONG) &SDT(g_pZwAlpcSendWaitReceivePort),  (LONG) g_pOldNTAlpcSendWaitReceivePort);
    }
    __asm
    {
        push    eax
        mov     eax, CR0
        or      eax, NOT 0FFFEFFFFh
        mov     CR0, eax
        pop     eax
    }
}


