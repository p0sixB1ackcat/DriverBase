#pragma once


// CPageProc dialog

class CPageProc : public CDialog
{
	DECLARE_DYNAMIC(CPageProc)

public:
	CPageProc(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPageProc();

// Dialog Data
	enum { IDD = IDD_DIALOG_PAGE_PROC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
