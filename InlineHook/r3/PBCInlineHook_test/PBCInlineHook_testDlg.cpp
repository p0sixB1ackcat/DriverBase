
// PBCInlineHook_testDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PBCInlineHook_test.h"
#include "PBCInlineHook_testDlg.h"
#include "afxdialogex.h"
#include <winioctl.h>

#define PBCBASECTLCODE 0x800

#define PBCCTLCODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN, PBCBASECTLCODE + i, METHOD_BUFFERED, FILE_ALL_ACCESS)

#define PBCINLINEHOOK_CTL PBCCTLCODE(1)
#define PBCKILLTHREADBYAPC_CTL PBCCTLCODE(2)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CPBCInlineHook_testDlg 对话框



CPBCInlineHook_testDlg::CPBCInlineHook_testDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_PBCINLINEHOOK_TEST_DIALOG, pParent)
	, m_FilePath(_T(""))
	, m_errorstr(_T(""))
	, m_ThreadId(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPBCInlineHook_testDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_FilePath);
	DDX_Text(pDX, IDC_EDIT2, m_errorstr);
	DDX_Text(pDX, IDC_THREAD_EDIT, m_ThreadId);
}

BEGIN_MESSAGE_MAP(CPBCInlineHook_testDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_LOADDRIBUTTON, &CPBCInlineHook_testDlg::OnBnClickedLoaddributton)
	ON_BN_CLICKED(IDC_UnInstallDriverButton, &CPBCInlineHook_testDlg::OnBnClickedUninstalldriverbutton)
	ON_BN_CLICKED(IDC_SendIoCtrlButton, &CPBCInlineHook_testDlg::OnBnClickedSendioctrlbutton)
	ON_BN_CLICKED(IDOK, &CPBCInlineHook_testDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CPBCInlineHook_testDlg 消息处理程序

BOOL CPBCInlineHook_testDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPBCInlineHook_testDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPBCInlineHook_testDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPBCInlineHook_testDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




void CPBCInlineHook_testDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	// TODO: Add your message handler code here and/or call default
	UINT count;
	TCHAR filePath[MAX_PATH] = { 0 };

	count = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
	if (count == 1)
	{
		DragQueryFile(hDropInfo, 0, filePath, sizeof(filePath));
		m_FilePath = filePath;
		//CheckNow(m_strPath);
		UpdateData(FALSE);
		DragFinish(hDropInfo);

		CDialog::OnDropFiles(hDropInfo);
		return;

	}
	else
	{
		//m_vectorFile.clear();
		for (UINT i = 0; i < count; i++)
		{
			int pathLen = DragQueryFile(hDropInfo, i, filePath, sizeof(filePath));
			//m_strPath = filePath;
			//m_vectorFile.push_back(filePath);
			//break;
		}
		//DoAllCheck();
		//CheckNow(m_strPath);
		UpdateData(FALSE);
		DragFinish(hDropInfo);
	}

    

	CDialogEx::OnDropFiles(hDropInfo);
}

BOOL LoadDriver(TCHAR* lpszDriverName, TCHAR* lpszDriverPath,CString &errorstr)
{
	TCHAR szDriverImagePath[256] = { 0 };
	//得到完整的驱动路径
	GetFullPathName(lpszDriverPath, 256, szDriverImagePath, NULL);

	BOOL bRet = FALSE;

	SC_HANDLE hServiceMgr = NULL;//SCM管理器的句柄
	SC_HANDLE hServiceDDK = NULL;//NT驱动程序的服务句柄

	//打开服务控制管理器
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	
	if (hServiceMgr == NULL)
	{
		//OpenSCManager失败
		//printf( "OpenSCManager() Failed %d ! \n", GetLastError() );
		bRet = FALSE;
		errorstr = "OpenSCManager faild!";
		goto BeforeLeave;
	}
	else
	{
		////OpenSCManager成功
		printf("OpenSCManager() ok ! \n");
	}

	//创建驱动所对应的服务
	hServiceDDK = CreateService(hServiceMgr,
		lpszDriverName, //驱动程序的在注册表中的名字  
		lpszDriverName, // 注册表驱动程序的 DisplayName 值  
		SERVICE_ALL_ACCESS, // 加载驱动程序的访问权限  
		SERVICE_KERNEL_DRIVER,// 表示加载的服务是驱动程序  
		SERVICE_DEMAND_START, // 注册表驱动程序的 Start 值  
		SERVICE_ERROR_IGNORE, // 注册表驱动程序的 ErrorControl 值  
		szDriverImagePath, // 注册表驱动程序的 ImagePath 值  
		NULL,  //GroupOrder HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GroupOrderList
		NULL,
		NULL,
		NULL,
		NULL);

	DWORD dwRtn;
	//判断服务是否失败
	if (hServiceDDK == NULL)
	{
		dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_EXISTS)
		{
			//由于其他原因创建服务失败
			//printf( "CrateService() Failed %d ! \n", dwRtn ); 
			errorstr = "other error!";
			bRet = FALSE;
			goto BeforeLeave;
			
		}
		else
		{
			//服务创建失败，是由于服务已经创立过
			printf("CrateService() Faild Service is ERROR_IO_PENDING or ERROR_SERVICE_EXISTS! \n");
			errorstr = "service is created!\n";
		}

		// 驱动程序已经加载，只需要打开  
		hServiceDDK = OpenService(hServiceMgr, lpszDriverName, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL)
		{
			//如果打开服务也失败，则意味错误
			dwRtn = GetLastError();
			//printf( "OpenService() Failed %d ! \n", dwRtn );  
			bRet = FALSE;
			errorstr = "OpenService faild!";
			goto BeforeLeave;
		}
		else
		{
			//printf( "OpenService() ok ! \n" );
		}
	}
	else
	{
		//printf( "CrateService() ok ! \n" );
	}

	//开启此项服务
	bRet = StartService(hServiceDDK, NULL, NULL);
	if (!bRet)
	{
		DWORD dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_ALREADY_RUNNING)
		{
			//printf( "StartService() Failed %d ! \n", dwRtn );  
			bRet = FALSE;
			errorstr = "start service faild!";
			goto BeforeLeave;
		}
		else
		{
			if (dwRtn == ERROR_IO_PENDING)
			{
				//设备被挂住
				//printf( "StartService() Failed ERROR_IO_PENDING ! \n");
				errorstr = "device is pending!";
				bRet = FALSE;
				goto BeforeLeave;
			}
			else
			{
				//服务已经开启
				//printf( "StartService() Failed ERROR_SERVICE_ALREADY_RUNNING ! \n");
				bRet = TRUE;
				errorstr = "service is already running!";
				goto BeforeLeave;
			}
		}
	}
	bRet = TRUE;
	//离开前关闭句柄
BeforeLeave:
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}

void CPBCInlineHook_testDlg::OnBnClickedLoaddributton()
{
	if (m_FilePath.GetLength() > 1)
	{
		TCHAR *T_Sysname, *T_SysPath;
		T_Sysname = (TCHAR *)malloc(sizeof(TCHAR)*MAX_PATH);
		T_SysPath = (TCHAR *)malloc(sizeof(TCHAR)*MAX_PATH);
		memset(T_Sysname, 0x00, sizeof(TCHAR)*MAX_PATH);
		memset(T_SysPath, 0x00, sizeof(TCHAR)*MAX_PATH);
		
		//wcstombs(T_SysPath, m_FilePath, m_FilePath.GetLength());
		_memccpy(T_SysPath, m_FilePath,sizeof(TCHAR)*MAX_PATH,sizeof(TCHAR)*MAX_PATH);
		//wcscpy_s(T_SysPath,MAX_PATH, m_FilePath);
		TCHAR *p1, *p2;
		p1 = T_SysPath;
		p2 = NULL;
		while (*p1 != '\0')
		{
			if (*p1++ == '\\')
			{
				p2 = p1;
			}
			
		}
		if (!p2)
		{
			printf("file path error!\n");
			goto ret;
			return;
		}

		size_t copysize = (size_t)((wcslen(p2) * 2) - (4 * sizeof(TCHAR)));
		
		if (copysize >= MAX_PATH || copysize < 0)
		{
			copysize = 18;
		}

		
		_memccpy(T_Sysname, p2, copysize, copysize);
		BOOL bLoad = FALSE;
		
		bLoad = LoadDriver(T_Sysname, T_SysPath, m_errorstr);
		if (bLoad == FALSE)
		{
			UpdateData(FALSE);
			char buffer[100];
			sprintf_s(buffer, 100, "loading driver faild:len is %d,p2 is %ws!", copysize,p2);
			MessageBoxA(0, buffer,"message", 0);
			goto ret;
		}
		m_errorstr = "load driver success!";
		UpdateData(FALSE);
	ret:
		free(T_Sysname);
		free(T_SysPath);
		T_Sysname = T_SysPath = NULL;
	}
}

BOOL UnloadDriver(TCHAR * szSvrName,CString &errorstr)
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;//SCM管理器的句柄
	SC_HANDLE hServiceDDK = NULL;//NT驱动程序的服务句柄
	SERVICE_STATUS SvrSta;
	//打开SCM管理器
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		//带开SCM管理器失败
		printf("OpenSCManager() Failed %d ! \n", GetLastError());
		errorstr = "OpenSCManager faild!";
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		//带开SCM管理器失败成功
		printf("OpenSCManager() ok ! \n");
	}
	//打开驱动所对应的服务
	hServiceDDK = OpenService(hServiceMgr, szSvrName, SERVICE_ALL_ACCESS);

	if (hServiceDDK == NULL)
	{
		//打开驱动所对应的服务失败
		printf("OpenService() Failed %d ! \n", GetLastError());
		bRet = FALSE;
		errorstr = "OpenService faild!";
		goto BeforeLeave;
	}
	else
	{
		printf("OpenService() ok ! \n");
	}
	//停止驱动程序，如果停止失败，只有重新启动才能，再动态加载。  
	if (!ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta))
	{
		printf("ControlService() Failed %d !\n", GetLastError());
		errorstr = "ControlService faild!";
	}
	else
	{
		//打开驱动所对应的失败
		printf("ControlService() ok !\n");
	}
	//动态卸载驱动程序。  
	if (!DeleteService(hServiceDDK))
	{
		//卸载失败
		printf("DeleteSrevice() Failed %d !\n", GetLastError());
		errorstr = "DeleteService faild!";
	}
	else
	{
		//卸载成功
		printf("DelServer:eleteSrevice() ok !\n");
	}
	bRet = TRUE;
BeforeLeave:
	//离开前关闭打开的句柄
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}

void CPBCInlineHook_testDlg::OnBnClickedUninstalldriverbutton()
{
	TCHAR *lptSysPath, *p1, *p2;
	TCHAR *lptSysName = NULL;
	BOOL bIsUnload = FALSE;
	if (m_FilePath.GetLength() <= 0)
	{
		MessageBoxA(0,"path is NULL!","message",0);
		return;
	}

	lptSysPath = (TCHAR *)malloc(sizeof(TCHAR)*MAX_PATH);
	if (!lptSysPath)
	{
		MessageBoxA(0,"Unistall vaild!","message",0);
		goto ret;
	}

	_memccpy(lptSysPath, m_FilePath, sizeof(TCHAR)*MAX_PATH, sizeof(TCHAR)*MAX_PATH);
	p1 = lptSysPath;
	p2 = NULL;
	while (*p1 != '\0')
	{
		if (*p1++ == '\\')
		{
			p2 = p1;
		}
	}

	if (p2)
	{
		lptSysName = (TCHAR *)malloc(sizeof(TCHAR) * MAX_PATH);
		memset(lptSysName, 0x00, sizeof(TCHAR) * MAX_PATH);
		_memccpy(lptSysName, p2, sizeof(TCHAR)*MAX_PATH, wcslen(p2) * sizeof(TCHAR) - 4 * sizeof(TCHAR));

		
		bIsUnload = UnloadDriver(lptSysName, m_errorstr);
		if (!bIsUnload)
		{
			UpdateData(FALSE);
			MessageBoxA(0, "UnInstall Driver faild!", "message", 0);
			goto ret;
		}
		m_errorstr = "UnInstall Driver Success!";
		UpdateData(FALSE);

	}
	ret:
	if (lptSysPath)
	{
		free(lptSysPath);
		lptSysPath = NULL;
	}
	if (lptSysName)
	{
		free(lptSysName);
		lptSysName = NULL;
	}

}


void CPBCInlineHook_testDlg::OnBnClickedSendioctrlbutton()
{
	BOOL DeviceIoCtrlResult = FALSE;
	DWORD errorCode;
	ULONG BufferSize = sizeof(TCHAR) * 0x100;
	TCHAR *inputBuffer = (TCHAR *)malloc(BufferSize);
	TCHAR *outputBuffer = (TCHAR *)malloc(BufferSize);
	DWORD dwResult = 0;
	if (!inputBuffer || !outputBuffer)
	{
		MessageBoxA(0, "malloc faild!", "message", 0);
		return;
	}
	
	// TODO: 在此添加控件通知处理程序代码
	HANDLE hFile = CreateFile(_T("\\\\.\\PBCSsdtHook")
	,GENERIC_WRITE | GENERIC_READ
	,0
	,NULL
	,OPEN_EXISTING
	,0
	,NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		errorCode = GetLastError();
		TCHAR lpszErrorBuffer[30] = {0x00};
		TCHAR *errorMsg = (TCHAR *)malloc(sizeof(TCHAR) * MAX_PATH);
		memset(errorMsg, 0x00, sizeof(TCHAR) * MAX_PATH);
		_memccpy(errorMsg, m_FilePath, wcslen(m_FilePath) * sizeof(TCHAR), sizeof(TCHAR) * MAX_PATH);
		wcscat(errorMsg, _T(":CreateFile error = "));
		wsprintfW(lpszErrorBuffer, _T("0x%x!"), errorCode);
		wcscat(errorMsg, lpszErrorBuffer);
		MessageBoxW( errorMsg, (TCHAR *)"message", 0);
		return;
	}

	
	DeviceIoCtrlResult = DeviceIoControl(hFile
		, PBCINLINEHOOK_CTL
		, inputBuffer
		, BufferSize
		, outputBuffer
		, BufferSize
		, &dwResult
		, NULL);
	if (!DeviceIoCtrlResult)
	{
		errorCode = GetLastError();
		TCHAR errorBuffer[100] = {0x00};
		wsprintfW(errorBuffer, _T("%ws:0x%x"), _T("DeviceIoCtrl faild"), errorCode);
		MessageBoxW(errorBuffer, _T("message"), 0);
		goto ret;
	}

	if (inputBuffer)
	{
		free(inputBuffer);
		inputBuffer = NULL;
	}
	if (outputBuffer)
	{
		free(outputBuffer);
		outputBuffer = NULL;
	}
	ret:
	CloseHandle(hFile);
}


void CPBCInlineHook_testDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	ULONG Status = ERROR_SUCCESS;
	HANDLE hDevice = 0;
	BYTE inputBuffer[20] = {0};
	ULONG dwRetLen = 0;
	LPWSTR p = NULL;
	BYTE *b = NULL;
	ULONG i = 0;

	UpdateData(TRUE);

	if (!m_ThreadId || !m_ThreadId.GetLength())
	{
		MessageBox(L"请输入线程ID",L"Message");
		return;
	}

	hDevice = CreateFile(L"\\\\.\\PBCDriver",GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		Status = GetLastError();
		MessageBox(L"CreateFile Fail!\n");
		return;
	}

	p = m_ThreadId.GetBuffer();
	b = inputBuffer;

	while(i < m_ThreadId.GetLength())
	{
		b[i] = p[i];
		b[i] -= '0';
		++i;
	}

	Status = DeviceIoControl(hDevice, PBCKILLTHREADBYAPC_CTL, inputBuffer, sizeof(inputBuffer), NULL, 0, &dwRetLen, NULL);

	if (!Status)
	{
		MessageBox(L"DeviceIoControl fail!\n");
		return;
	}


}
