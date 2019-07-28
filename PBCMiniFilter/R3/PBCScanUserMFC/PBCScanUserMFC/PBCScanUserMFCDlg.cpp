
// PBCScanUserMFCDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PBCScanUserMFC.h"
#include "PBCScanUserMFCDlg.h"
#include "afxdialogex.h"
#include <fltUser.h>
#include "scanuk.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define SCANNER_DEFAULT_REQUEST_COUNT 5
#define SCANNER_DEFAULT_THREAD_COUNT 2
#define SCANNER_MAX_THREAD_COUNT  64

typedef struct _SCANNER_MESSAGE
{
	FILTER_MESSAGE_HEADER MessageHeader;

	SCANNER_NOTIFICATION Notification;

	OVERLAPPED Ovlp;

}SCANNER_MESSAGE,*PSCANNER_MESSAGE;

typedef struct _SCANNER_THREAD_CONTEXT
{
	HANDLE Port;
	HANDLE Completion;

}SCANNER_THREAD_CONTEXT,*PSCANNER_THREAD_CONTEXT;
 
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


// CPBCScanUserMFCDlg 对话框



CPBCScanUserMFCDlg::CPBCScanUserMFCDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_PBCSCANUSERMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPBCScanUserMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPBCScanUserMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CPBCScanUserMFCDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CPBCScanUserMFCDlg 消息处理程序

BOOL CPBCScanUserMFCDlg::OnInitDialog()
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

void CPBCScanUserMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CPBCScanUserMFCDlg::OnPaint()
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
HCURSOR CPBCScanUserMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

DWORD ScannerWorker(PSCANNER_THREAD_CONTEXT lpContext)
{
	HANDLE hPort;
	HANDLE hIoCompletion;


	return 1;
}

VOID ConnectR0Port(VOID)
{
	HRESULT hr;
	HANDLE hPort = NULL;
	HANDLE hIoCompletion = NULL;
	SCANNER_THREAD_CONTEXT context;
	ULONG i,j;
	HANDLE threads[SCANNER_DEFAULT_THREAD_COUNT] = {0x00};
	DWORD threadId = 0;
	PSCANNER_MESSAGE lpMessage;

	hr = FilterConnectCommunicationPort(ScannerPortName
		, 0
		, NULL
		, 0
		, NULL
		, &hPort);
	if (IS_ERROR(hr))
	{
		MessageBox(0,_T("连接内核端口失败"), _T("message"), 0);
		goto ret;
	}

	hIoCompletion = CreateIoCompletionPort(hPort
		, NULL
		, 0
		, SCANNER_DEFAULT_THREAD_COUNT);
	if (hIoCompletion == NULL)
	{
		MessageBox(0,_T("创建完成端口失败!"), _T("message"), 0);
		goto ret;
	}

	FilterSendMessage(hPort, _T("QQ,360,txt,inf,doc,bat,cmd"), wcslen(_T("QQ,360,txt,inf,doc,bat,cmd")), NULL, 0, NULL);
	context.Port = hPort;
	context.Completion = hIoCompletion;

	for (i = 0; i < SCANNER_DEFAULT_THREAD_COUNT; ++i)
	{
		threads[i] = CreateThread(NULL
		,0
		,(LPTHREAD_START_ROUTINE)ScannerWorker
		,&context
		,0
		,&threadId);

		if (threads[i] == NULL)
		{
			printf("Create Thread Error:%d!\n",GetLastError());
			goto ret;
		}

		for (j = 0; j < SCANNER_DEFAULT_REQUEST_COUNT; ++j)
		{
			lpMessage = (PSCANNER_MESSAGE)malloc(sizeof(SCANNER_MESSAGE));
			if (!lpMessage)
			{
				printf("Malloc Error:%d!\n",GetLastError());
				goto ret;
			}

			memset(&lpMessage->Ovlp,0x00,sizeof(OVERLAPPED));

			hr = FilterGetMessage(hPort
				, &lpMessage->MessageHeader
				, sizeof(FILTER_MESSAGE_HEADER)
				, &lpMessage->Ovlp);
			if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
			{
				free(lpMessage);
				lpMessage = NULL;
				goto ret;
			}
		}

	}

	WaitForMultipleObjectsEx(SCANNER_DEFAULT_REQUEST_COUNT
	,threads
	,TRUE
	,INFINITE
	,FALSE);


ret:
	if (hr)
	{
		CloseHandle(hPort);
	}
	if (hIoCompletion)
	{
		CloseHandle(hIoCompletion);
	}
}

void CPBCScanUserMFCDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//CDialogEx::OnOK();

	ConnectR0Port();

}
