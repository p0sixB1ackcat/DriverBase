// mfc1Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "mfc1.h"
#include "mfc1Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// Cmfc1Dlg 对话框




Cmfc1Dlg::Cmfc1Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cmfc1Dlg::IDD, pParent)
	, m_strUserId(_T(""))
	, m_userIdOutput(_T(""))
	, m_Gender(0)
	, m_Doc(FALSE)
	, m_BSinger(FALSE)
	, m_BTest(FALSE)
	, m_BDev(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cmfc1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, EDIT_ID, m_strUserId);
	DDX_Text(pDX, EDIT_ID_OUTPUT, m_userIdOutput);

	DDX_Radio(pDX, IDC_RADIO_WOMAN, m_Gender);
	DDX_Check(pDX, IDC_CHECK_DEC, m_Doc);
	DDX_Check(pDX, IDC_CHECK_SIGNER, m_BSinger);
	DDX_Check(pDX, IDC_CHECK_Test, m_BTest);
	DDX_Check(pDX, IDC_CHECK_DEV, m_BDev);
	DDX_Control(pDX, IDC_LIST_test, m_ListCtrl);
}

BEGIN_MESSAGE_MAP(Cmfc1Dlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &Cmfc1Dlg::OnBnClickedOk)
	ON_EN_CHANGE(EDIT_ID, &Cmfc1Dlg::OnEnChangeId)
	
ON_NOTIFY(NM_CLICK, IDC_LIST_test, &Cmfc1Dlg::OnNMClickListtest)
ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_test, &Cmfc1Dlg::OnLvnColumnclickListtest)
END_MESSAGE_MAP()


// Cmfc1Dlg 消息处理程序

BOOL Cmfc1Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();


	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_ListCtrl.InsertColumn(0,_T("内核钩子"),LVCFMT_LEFT,85);
	m_ListCtrl.InsertColumn(1,_T("应用层钩子"),LVCFMT_LEFT,115);
	m_ListCtrl.InsertColumn(2,_T("ssdt"),LVCFMT_LEFT,60);
	int ilineCount = m_ListCtrl.GetItemCount();

	m_ListCtrl.InsertItem(ilineCount,_T("1"));
	m_ListCtrl.SetItemText(ilineCount,1,_T("内核钩子1"));
	m_ListCtrl.SetItemText(ilineCount,2,_T("应用层钩子1"));

	m_ListCtrl.InsertItem(ilineCount,_T("2"));
	m_ListCtrl.SetItemText(ilineCount,1,_T("内核钩子2"));
	m_ListCtrl.SetItemText(ilineCount,2,_T("应用层钩子2"));

	m_ListCtrl.InsertItem(ilineCount,_T("3"));
	m_ListCtrl.SetItemText(ilineCount,1,_T("内核钩子3"));
	m_ListCtrl.SetItemText(ilineCount,2,_T("应用层钩子3"));


	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Cmfc1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Cmfc1Dlg::OnPaint()
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
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR Cmfc1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void Cmfc1Dlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//OnOK();

	UpdateData(TRUE);

	//m_userIdOutput = m_strUserId;
	CString userId = m_strUserId;
	CString GenderStr = _T("");
	switch(m_Gender)
	{
	case 0:
		GenderStr += _T("女");
		break;
	case 1:
		GenderStr += _T("男");
		break;
	case 2:
		GenderStr += _T("人妖");
		break;
	default:
		GenderStr += _T("未知");
		break;
	}


	

	UpdateData(FALSE);

	MessageBox(m_strUserId,_T("p0sixB1ackcat"),MB_OK);
	MessageBox(GenderStr,_T("性别"),MB_OK);

	while(m_ListCtrl.DeleteItem(0))
	{

	}

}


void Cmfc1Dlg::OnEnChangeId()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}



void Cmfc1Dlg::OnNMClickListtest(NMHDR *pNMHDR, LRESULT *pResult)
{
	int istatus = m_ListCtrl.GetSelectionMark();
	CString strNumOne = m_ListCtrl.GetItemText(istatus,0);
	CString strKernelOne = m_ListCtrl.GetItemText(istatus,1);
	CString strKernelTwo = m_ListCtrl.GetItemText(istatus,2);

}

DWORD Cmfc1Dlg::m_SortColum = 0;
BOOL Cmfc1Dlg::m_bAs = TRUE;



int   CALLBACK Cmfc1Dlg::MyListCompar(LPARAM   lParam1,   LPARAM   lParam2,   LPARAM   lParamSort) 
{ 
	//通过传递的参数来得到CSortList对象指针，从而得到排序方式 
	CListCtrl* pListCtrl = (CListCtrl*) lParamSort;

	//通过ItemData来确定数据 

	int   iCompRes; 
	CString    szComp1 = pListCtrl->GetItemText(lParam1,m_SortColum);
	CString    szComp2 = pListCtrl->GetItemText(lParam2,m_SortColum);

	switch(m_SortColum) 
	{ 
	case(2): 
		//以第一列为根据排序   编号
		//_ttol 
		iCompRes=_ttol(szComp1) <=_ttol(szComp2)?-1:1; 
		break; 

	default: 
		iCompRes=szComp1.Compare(szComp2); 
		break; 
	} 
	//根据当前的排序方式进行调整

	if(m_bAs) 
		return   iCompRes; 
	else 
		return   -iCompRes; 	
} 
void Cmfc1Dlg::OnLvnColumnclickListtest(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	m_SortColum = pNMListView->iSubItem;
	m_bAs=!m_bAs;//升序还是降序

	int count = m_ListCtrl.GetItemCount();   //行数
	for (int i=0; i<count;  i++)  
	{  
		m_ListCtrl.SetItemData(i,i);  
	}
	m_ListCtrl.SortItems(MyListCompar, (LPARAM) &m_ListCtrl);
	*pResult = 0;
}
