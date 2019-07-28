// PageNet.cpp : implementation file
//

#include "stdafx.h"
#include "firstmfc.h"
#include "PageNet.h"


// CPageNet dialog

IMPLEMENT_DYNAMIC(CPageNet, CDialog)

CPageNet::CPageNet(CWnd* pParent /*=NULL*/)
	: CDialog(CPageNet::IDD, pParent)
{

}

CPageNet::~CPageNet()
{
}

void CPageNet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPageNet, CDialog)
END_MESSAGE_MAP()


// CPageNet message handlers
