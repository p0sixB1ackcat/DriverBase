#pragma once


// CPageNet dialog

class CPageNet : public CDialog
{
	DECLARE_DYNAMIC(CPageNet)

public:
	CPageNet(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPageNet();

// Dialog Data
	enum { IDD = IDD_DIALOG_PAGE_NET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
