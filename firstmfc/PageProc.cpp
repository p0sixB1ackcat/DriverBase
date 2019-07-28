// PageProc.cpp : implementation file
//

#include "stdafx.h"
#include "firstmfc.h"
#include "PageProc.h"


// CPageProc dialog

IMPLEMENT_DYNAMIC(CPageProc, CDialog)

CPageProc::CPageProc(CWnd* pParent /*=NULL*/)
	: CDialog(CPageProc::IDD, pParent)
{

}

CPageProc::~CPageProc()
{
}

void CPageProc::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPageProc, CDialog)
END_MESSAGE_MAP()


// CPageProc message handlers
