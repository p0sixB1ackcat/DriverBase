// mfc1Dlg.h : 头文件
//

#pragma once
#include "afxcmn.h"


// Cmfc1Dlg 对话框
class Cmfc1Dlg : public CDialog
{
// 构造
public:
	Cmfc1Dlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MFC1_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	
	afx_msg void OnEnChangeId();
	CString m_strUserId;
	CString m_userIdOutput;
	int m_Gender;
	BOOL m_Doc;
	BOOL m_BSinger;
	BOOL m_BTest;
	BOOL m_BDev;
	static DWORD m_SortColum;
	static BOOL m_bAs;
	CListCtrl m_ListCtrl;
	afx_msg void OnNMClickListtest(NMHDR *pNMHDR, LRESULT *pResult);
	static int CALLBACK MyListCompar(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	afx_msg void OnLvnColumnclickListtest(NMHDR *pNMHDR, LRESULT *pResult);
};
