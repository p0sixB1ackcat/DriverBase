// LstProcDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ListProc.h"
#include "LstProcDlg.h"
#include "Instdrv.h"
#include <winioctl.h>
#include "ioctlcmd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CLstProcDlg dialog

extern HANDLE gh_Device;
extern CHAR ac_driverLabel[];
extern CHAR ac_driverName[];
int CLstProcDlg::m_columnClicked= -1;
int CLstProcDlg::m_nClicked = 0;
extern DWORD InitDriver();
extern BOOL Initialized;

CLstProcDlg::CLstProcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLstProcDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLstProcDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CLstProcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLstProcDlg)
	DDX_Control(pDX, IDC_LIST, m_procListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLstProcDlg, CDialog)
	//{{AFX_MSG_MAP(CLstProcDlg)
	ON_BN_CLICKED(IDC_PROC, OnProc)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST, OnColumnclickList)
	ON_COMMAND(ID_KILLPROCESS, OnKillprocess)
	ON_COMMAND(ID_BAIDU, OnBaidu)
	ON_COMMAND(ID_GOOGLE, OnGoogle)
	ON_NOTIFY(NM_RCLICK, IDC_LIST, OnRclickList)
	ON_COMMAND(ID_FORCE_KILLPROC, OnForceKillproc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLstProcDlg message handlers
//Windows NT Functions

typedef BOOL (WINAPI *ENUMPROCESSES)(
  DWORD * lpidProcess,  // array to receive the process identifiers
  DWORD cb,             // size of the array
  DWORD * cbNeeded      // receives the number of bytes returned
);

typedef DWORD (WINAPI *GETMODULEFILENAMEA)( 
  HANDLE hProcess,		// handle to the process
  HMODULE hModule,		// handle to the module
  LPSTR lpstrFileName,	// array to receive filename
  DWORD nSize			// size of filename array.
);

typedef DWORD (WINAPI *GETMODULEFILENAMEW)( 
  HANDLE hProcess,		// handle to the process
  HMODULE hModule,		// handle to the module
  LPWSTR lpstrFileName,	// array to receive filename
  DWORD nSize			// size of filename array.
);

#define	GETMODULEFILENAME	GETMODULEFILENAMEA


typedef BOOL (WINAPI *ENUMPROCESSMODULES)(
  HANDLE hProcess,      // handle to the process
  HMODULE * lphModule,  // array to receive the module handles
  DWORD cb,             // size of the array
  LPDWORD lpcbNeeded    // receives the number of bytes returned
);

int CALLBACK CLstProcDlg::ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{

	CListCtrl* list=(CListCtrl*)lParamSort;
	CString s1,s2;	
	int result,i1,i2,n1,n2;
	LVFINDINFO findInfo;	
	findInfo.flags=LVFI_PARAM;	
	findInfo.lParam=lParam1;	
	n1=list->FindItem(&findInfo,-1);	
	findInfo.lParam=lParam2;	
	n2=list->FindItem(&findInfo,-1);	
	s1=list->GetItemText(n1,m_columnClicked);	
	s2=list->GetItemText(n2,m_columnClicked);	
	switch(m_columnClicked) 
		{	
		case 0:		
			i1=atoi(s1);		
			i2=atoi(s2);		
			if(m_nClicked%2==1)				
				result=i1>i2?1:(i1<i2?-1:0);		
			else 			
				result=i1>i2?-1:(i1<i2?1:0);		
			break;	
		default:			
			if(m_nClicked%2==1)
			{
				int len = strlen(s1);
				for(int i=0;i<len;i++)
					s1.SetAt(i,toupper(s1[i]));
				len = strlen(s2);
				for(int i=0;i<len;i++)
					s2.SetAt(i, toupper(s2[i]));
				result=strcmp(s1,s2);
			}
			else 
			{
				int len = strlen(s1);
				for(int i=0;i<len;i++)
					s1.SetAt(i,toupper(s1[i]));
				len = strlen(s2);
				for(int i=0;i<len;i++)
					s2.SetAt(i,toupper(s2[i]));
				result=strcmp(s2,s1);
			}
			break;
		}		
		return result;
}
void CLstProcDlg::OnProc() 
{
	ENUMPROCESSES       pEnumProcesses = NULL;
	GETMODULEFILENAME   pGetModuleFileName = NULL;
	ENUMPROCESSMODULES  pEnumProcessModules = NULL; 

	HMODULE modPSAPI = LoadLibrary(_T("PSAPI.DLL"));
	pEnumProcesses = (ENUMPROCESSES)GetProcAddress(modPSAPI, "EnumProcesses");
	pGetModuleFileName = (GETMODULEFILENAME)GetProcAddress(modPSAPI, "GetModuleFileNameExA");
	pEnumProcessModules = (ENUMPROCESSMODULES)GetProcAddress(modPSAPI, "EnumProcessModules");
	if(pEnumProcesses == NULL ||
		pGetModuleFileName == NULL ||
		pEnumProcessModules == NULL)
		return;

	DWORD nProcessIDs[1024];
	DWORD nProcessNo;

	BOOL bSuccess = pEnumProcesses(nProcessIDs, sizeof(nProcessIDs), &nProcessNo);
	if ( !bSuccess )
	{
			return;
	}  

	nProcessNo /= sizeof(nProcessIDs[0]);
	m_procListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES); 
	m_procListCtrl.DeleteAllItems();  
	while(m_procListCtrl.DeleteColumn(0));
	m_procListCtrl.InsertColumn(1,"PID",LVCFMT_CENTER,50);  
 
	m_procListCtrl.InsertColumn(2,"映像位置与名称",LVCFMT_LEFT,700);  

	for ( unsigned i=0; i<nProcessNo; i++)
	{
			int m;
			HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, 
								FALSE, nProcessIDs[i]);
			DWORD error=GetLastError();

			HMODULE hModules[1024];
			DWORD nModuleNo;
			TCHAR szFileName[MAX_PATH];

			pEnumProcessModules(process, hModules, sizeof(hModules), &nModuleNo);

			nModuleNo /= sizeof(hModules[0]);

			if ( pGetModuleFileName(process, hModules[0], szFileName, sizeof(szFileName)) )
			{
					CString strPid;
					CString strName;
					strPid.Format("%u",nProcessIDs[i]);
					strName.Format("%s",szFileName);
					m = m_procListCtrl.InsertItem(i, strPid);
					m_procListCtrl.SetItemText(m,1,strName); 
			}
			CloseHandle(process);
	}
	return;
	
}


void CLstProcDlg::OnColumnclickList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	m_nClicked++;
	int col = pNMListView->iSubItem;	
	m_columnClicked=col;	
	int nItemCounter=m_procListCtrl.GetItemCount();
	for(int i=0;i<nItemCounter;i++)
		m_procListCtrl.SetItemData(i,i);
	int result=m_procListCtrl.SortItems((PFNLVCOMPARE)CLstProcDlg::ListViewCompareFunc,(LPARAM)&m_procListCtrl); 	
	*pResult = 0;
}

void CLstProcDlg::OnKillprocess() 
{

	DWORD dw;
	if(m_dwPID==-1)
	{
		MessageBox("请选择进程");
		return;
	}
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, 
								FALSE, m_dwPID);
	if(process!=NULL)
	{
	GetExitCodeProcess(process,&dw); 
	TerminateProcess(process,dw);
	MessageBox(_T("进程已被终止"),_T("终止进程"),MB_OK);
	}
	else
		MessageBox(_T("进程终止失败"),_T("终止进程"),MB_OK);	
}

void CLstProcDlg::OnBaidu() 
{
	// TODO: Add your command handler code here
	ShellExecute(NULL, "open", "http://www.baidu.com/s?wd="+m_strSearch,  
             NULL, NULL, SW_SHOWNORMAL); 
}

void CLstProcDlg::OnGoogle() 
{
	ShellExecute(NULL, "open", "http://www.google.cn/search?q="+m_strSearch,  
             NULL, NULL, SW_SHOWNORMAL); 	
}

void CLstProcDlg::OnRclickList(NMHDR* pNMHDR, LRESULT* pResult) 
{

    LONG pid;
    CMenu menu , *pSubMenu;//定义下面要用到的cmenu对象
	
    menu.LoadMenu(IDR_RIGHT);//装载自定义的右键菜单
    pSubMenu = menu.GetSubMenu(0);//获取第一个弹出菜单，所以第一个菜单必须有子菜单
    CPoint oPoint;//定义一个用于确定光标位置的位置
    GetCursorPos( &oPoint);//获取当前光标的位置，以便使得菜单可以跟随光标

    int istat=m_procListCtrl.GetSelectionMark();//用istat存放当前选定的是第几项
    m_strSearch =m_procListCtrl.GetItemText(istat,1);//获取当前项中的数据，0代表是第0列
	m_dwPID = (pid=atol(m_procListCtrl.GetItemText(istat,0)))>0?pid:-1;
    pSubMenu->TrackPopupMenu (TPM_LEFTALIGN, oPoint.x, oPoint.y, this); //在指定位置显示弹出菜单

	*pResult = 0;	
}

void CLstProcDlg::OnForceKillproc() 
{
	DWORD ret = 0;
	DWORD read;
	if(m_dwPID==-1)
	{
		MessageBox("请选择进程");
		return;
	}
	if(MessageBox(_T("强杀进程可能会造成系统不稳定，确认要进行吗？"),_T("强杀进程"),MB_YESNO)==IDYES)
	{
		InitDriver();
		Initialized = FALSE;
		DeviceIoControl(gh_Device, 
						IOCTL_PROC_KILL,
						&m_dwPID,
						sizeof(m_dwPID),
						&ret,
						sizeof(ret),
						&read,
						NULL);
		CloseHandle(gh_Device);
		UnloadDeviceDriver(ac_driverName);
		if(ret==0)
			MessageBox(_T("成功执行"));
		else
			MessageBox(_T("执行失败"));
		CloseHandle(gh_Device);
		ShellExecute(NULL, "open", "sc","stop LstProc", NULL, SW_HIDE); 

		
	}	
}

BOOL CLstProcDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	OnProc();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
