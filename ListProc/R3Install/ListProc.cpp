// ListProc.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ListProc.h"
#include "LstProcDlg.h"
#include "Instdrv.h"
//#pragma comment(lib,"appface.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
TCHAR ac_driverLabel[] = _T("PBCListProc");
TCHAR ac_driverName[] = _T("PBCListProc.sys");
TCHAR ac_driverPath[MAX_PATH] = _T("\0");

BOOL Initialized = FALSE;
HANDLE gh_Device = INVALID_HANDLE_VALUE;
/////////////////////////////////////////////////////////////////////////////
// CListProcApp

BEGIN_MESSAGE_MAP(CListProcApp, CWinApp)
	//{{AFX_MSG_MAP(CListProcApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListProcApp construction

CListProcApp::CListProcApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CListProcApp object

CListProcApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CListProcApp initialization

BOOL CListProcApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
	//SkinStart(_T("IDR_MY_URF2"),NULL,GTP_LOAD_RESOURCE,NULL,_T("MYRESTYPE"));
	CLstProcDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	//SkinRemove() ;
	return FALSE;
}
DWORD InitDriver()
{

	int iRetCode = ERROR_SUCCESS;
	HANDLE h_Device = INVALID_HANDLE_VALUE;
	DWORD d_error;
	
	int time = 0;
	if (Initialized)
	{
		return iRetCode;
	}
	try {
		again:

		if(strlen(ac_driverPath)<=0)
		{
			GetCurrentDirectory(MAX_PATH, ac_driverPath);
		
			strncat(ac_driverPath, _T("\\"), MAX_PATH-strlen(ac_driverPath));
			strncat(ac_driverPath, 
				ac_driverName, 
				MAX_PATH-strlen(ac_driverPath));
		}

		
		LoadDeviceDriver(ac_driverLabel, ac_driverPath ,&h_Device, &d_error);
		if (h_Device == INVALID_HANDLE_VALUE)
		{

			time++;
			if(time<10)
				goto again;
			throw d_error;
		}
		gh_Device = h_Device;
	}catch (DWORD error) {
		LPVOID lpMsgBuf = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
						(LPTSTR) &lpMsgBuf, 0, NULL);
		MessageBox( NULL, (LPCTSTR)ac_driverPath, _T("Info"), MB_OK | MB_ICONINFORMATION ); 
		MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION ); 
		if (lpMsgBuf)
			LocalFree(lpMsgBuf);

		return -1;	
	}

	Initialized = TRUE;
	return (iRetCode);
} //InitDriver()