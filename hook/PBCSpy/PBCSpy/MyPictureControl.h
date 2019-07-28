#pragma once


// CMyPictureControl 对话框

class CMyPictureControl : public CDialogEx
{
	DECLARE_DYNAMIC(CMyPictureControl)

public:
	CMyPictureControl(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CMyPictureControl();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PBCSPY_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
