// PageFile.cpp : implementation file
//

#include "stdafx.h"
#include "firstmfc.h"
#include "PageFile.h"


// CPageFile dialog

IMPLEMENT_DYNAMIC(CPageFile, CDialog)

CPageFile::CPageFile(CWnd* pParent /*=NULL*/)
	: CDialog(CPageFile::IDD, pParent)
{

}

CPageFile::~CPageFile()
{
}

void CPageFile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPageFile, CDialog)
END_MESSAGE_MAP()


// CPageFile message handlers
