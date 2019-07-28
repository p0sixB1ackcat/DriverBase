// PBCR3HookDll.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "PBCR3HookDll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//定义NtHookEngine中的函数，用三个函数指针来接收
BOOL(__cdecl *HookFunction)(ULONG_PTR OriginalFunction, ULONG_PTR NewFunction);
VOID(__cdecl *UnHookFunction)(ULONG_PTR Function);
ULONG_PTR(__cdecl *GetOriginalFunction)(ULONG_PTR HookFunction);

//Hook后的函数,签名要和原函数保持一致
int WINAPI PBC_MessageBoxA(__in HWND hWnd, __in LPCSTR lpText, __in LPCSTR lpCaption, __in unsigned int uType);

//定义备份函数
int (WINAPI *OldMessageBoxA)(__in HWND hWnd, __in LPCSTR lpText, __in LPCSTR lpCaption, __in unsigned int uType);

VOID PBC_StartHook(VOID);
VOID PBX_UnHook(VOID);

//
//TODO:  如果此 DLL 相对于 MFC DLL 是动态链接的，
//		则从此 DLL 导出的任何调入
//		MFC 的函数必须将 AFX_MANAGE_STATE 宏添加到
//		该函数的最前面。
//
//		例如: 
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// 此处为普通函数体
//		}
//
//		此宏先于任何 MFC 调用
//		出现在每个函数中十分重要。  这意味着
//		它必须作为函数中的第一个语句
//		出现，甚至先于所有对象变量声明，
//		这是因为它们的构造函数可能生成 MFC
//		DLL 调用。
//
//		有关其他详细信息，
//		请参阅 MFC 技术说明 33 和 58。
//

// CPBCR3HookDllApp

BEGIN_MESSAGE_MAP(CPBCR3HookDllApp, CWinApp)
END_MESSAGE_MAP()


// CPBCR3HookDllApp 构造

CPBCR3HookDllApp::CPBCR3HookDllApp()
{
	// TODO:  在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CPBCR3HookDllApp 对象

CPBCR3HookDllApp theApp;


// CPBCR3HookDllApp 初始化

BOOL CPBCR3HookDllApp::InitInstance()
{
	CWinApp::InitInstance();

	PBC_StartHook();

	return TRUE;
}

extern "C" __declspec(dllexport)int KeyBoardHook()
{
	static int i = 0;
	if(!i)
		MessageBox(0,"你已经被Hook","message",0);
	++i;
	return 1;
}

VOID PBC_StartHook(VOID)
{
	HMODULE hNtHookEngine;
	ULONG_PTR MessageBoxAAddress;
	hNtHookEngine =  LoadLibrary(_T("NtHookEngine.dll"));
	if (!hNtHookEngine)
	{
		MessageBoxA(0,_T("没有找到NtHookEngine.dll"),_T("message"),0);
		return;
	}
	
	HookFunction = (BOOL (__cdecl *)(ULONG_PTR,ULONG_PTR))
		GetProcAddress(hNtHookEngine, (LPCSTR)_T("HookFunction"));

	UnHookFunction = (VOID(__cdecl *)(ULONG_PTR))
		GetProcAddress(hNtHookEngine, (LPCSTR)_T("UnhookFunction"));

	GetOriginalFunction = (ULONG_PTR(__cdecl *)(ULONG_PTR))
		GetProcAddress(hNtHookEngine, (LPCSTR)_T("GetOriginalFunction"));
	
	if (!HookFunction || !UnHookFunction || !GetOriginalFunction)
	{
		MessageBoxA(0,_T("获取NtHookEngine.dll函数失败!"), _T("message"), 0);
		return;
	}
	
	//开始Hook
	MessageBoxAAddress = (ULONG_PTR)GetProcAddress(LoadLibraryA(_T("User32.dll")), _T("MessageBoxA"));
	HookFunction((ULONG_PTR)MessageBoxAAddress, (ULONG_PTR)&PBC_MessageBoxA);
	OldMessageBoxA = (int ( WINAPI *)(HWND,LPCSTR,LPCSTR,UINT))GetOriginalFunction((ULONG_PTR)PBC_MessageBoxA);

}

int WINAPI PBC_MessageBoxA(__in HWND hWnd, __in LPCSTR lpText, __in LPCSTR lpCaption, __in unsigned int uType)
{
	char Buffer[0x200] = {0x00};
	
	sprintf_s(Buffer, 0x200,"PBC:你已经被Hook!%s",lpText);
	return OldMessageBoxA(hWnd, lpText, Buffer, uType);
}

VOID PBC_UnHook(VOID)
{
	ULONG_PTR Original = (ULONG_PTR)GetProcAddress(GetModuleHandle(_T("User32.dll")),(LPCSTR)_T("MessageBoxA"));
	UnHookFunction(Original);
}