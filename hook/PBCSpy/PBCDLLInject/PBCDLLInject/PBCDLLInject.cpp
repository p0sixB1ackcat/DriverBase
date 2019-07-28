// PBCDLLInject.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "PBCDLLInject.h"
#include <Windows.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//share segment with process
#pragma  data_seg(".shared")
HWND hPedit = NULL;
char lpPasswordBuffer[128] = {0x00};
#pragma  data_seg()

#pragma  comment(Linker,"/Section:.shared","RWS")

BEGIN_MESSAGE_MAP(CPBCDLLInjectApp, CWinApp)
END_MESSAGE_MAP()


// CPBCDLLInjectApp 构造

CPBCDLLInjectApp::CPBCDLLInjectApp()
{
	// TODO:  在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CPBCDLLInjectApp 对象

CPBCDLLInjectApp theApp;


// CPBCDLLInjectApp 初始化

BOOL CPBCDLLInjectApp::InitInstance()
{
	CWinApp::InitInstance();
	
	return TRUE;
}

BOOL DllMain(_In_ void* _DllHandle, _In_ unsigned long _Reason, _In_opt_ void* _Reserved)
{

}