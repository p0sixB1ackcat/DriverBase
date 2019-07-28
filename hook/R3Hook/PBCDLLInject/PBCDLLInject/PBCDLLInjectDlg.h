
// PBCDLLInjectDlg.h : 头文件
//

#pragma once

typedef ULONG(WINAPI *NTCREATETHREADEX)(PHANDLE pThreadHandle, ACCESS_MASK DesiredAccess, LPVOID ObjectAttributes, HANDLE ProcessHandle, LPTHREAD_START_ROUTINE pThreadStartRoutine, PVOID lpParameter, BOOL CreateSuspended, ULONG StackZeroBits, ULONG SizeOfStackCommit, ULONG SizeOfStackReserve, LPVOID lpBytesBuffer);


// CPBCDLLInjectDlg 对话框
class CPBCDLLInjectDlg : public CDialogEx
{
// 构造
public:
	CPBCDLLInjectDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PBCDLLINJECT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	BOOL CPBCDLLInjectDlg::CheckInput(void);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnCancel();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_strProcId;
	afx_msg void OnBnClickedCreateremotethreadinject();
	afx_msg void OnBnClickedNtcreatethreadButton();
	afx_msg void OnBnClickedQueueapcBtn();
	afx_msg void OnBnClickedSetthreadcontextBtn();
	static UINT32 WINAPI SetThreadContextRoutine(PVOID pContext);
	afx_msg void OnBnClickedSetwindowshookexBtn();
};
