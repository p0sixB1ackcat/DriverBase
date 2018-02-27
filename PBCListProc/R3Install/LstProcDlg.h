#if !defined(AFX_LSTPROCDLG_H__C93440F3_BA05_4FBB_94E8_6F84DE58AF18__INCLUDED_)
#define AFX_LSTPROCDLG_H__C93440F3_BA05_4FBB_94E8_6F84DE58AF18__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LstProcDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLstProcDlg dialog

class CLstProcDlg : public CDialog
{
// Construction
public:
	CLstProcDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLstProcDlg)
	enum { IDD = IDD_ListProc_DIALOG };
	CListCtrl	m_procListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLstProcDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLstProcDlg)
	afx_msg void OnProc();
	afx_msg void OnColumnclickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillprocess();
	afx_msg void OnBaidu();
	afx_msg void OnGoogle();
	afx_msg void OnRclickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnForceKillproc();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	static int CALLBACK ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
private:
	static int m_columnClicked;
	static int m_nClicked;
	LONG m_dwPID;
	CString m_strSearch;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSTPROCDLG_H__C93440F3_BA05_4FBB_94E8_6F84DE58AF18__INCLUDED_)
