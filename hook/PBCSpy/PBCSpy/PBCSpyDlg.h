
// PBCSpyDlg.h : 头文件
//
#pragma once
#include "MyPictureControl.h"

// CPBCSpyDlg 对话框
class CPBCSpyDlg : public CDialogEx
{
// 构造
public:
	CPBCSpyDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PBCSPY_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	void OnMouseMove(UINT nFlags, CPoint point);
	CMyPictureControl g_mypiccontrol;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStartdlgbutton();
};
