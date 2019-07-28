#pragma once


// CPageFile dialog

class CPageFile : public CDialog
{
	DECLARE_DYNAMIC(CPageFile)

public:
	CPageFile(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPageFile();

// Dialog Data
	enum { IDD = IDD_DIALOG_PAGE_FILE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
