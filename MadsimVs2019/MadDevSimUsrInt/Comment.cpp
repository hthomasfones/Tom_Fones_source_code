/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2022 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* XYZ Company                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadSimUI.exe                                                */
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : Comment.cpp                                                 */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                 This dialog box supports adding a comment to the Trace win  */
/*                                                                             */
/*******************************************************************************/

// Comment.cpp : implementation file
//

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "Comment.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComment dialog


CComment::CComment(CWnd* pParent /*=NULL*/)
	: CDialog(CComment::IDD, pParent)
{
	//{{AFX_DATA_INIT(CComment)
	m_Comment = _T("");
	//}}AFX_DATA_INIT
}


void CComment::DoDataExchange(CDataExchange* pDX)

{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CComment)
	DDX_Control(pDX, IDC_COMMENT, c_Comment);
	DDX_Control(pDX, IDOK, c_OK);
	DDX_Text(pDX, IDC_COMMENT, m_Comment);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CComment, CDialog)
//{{AFX_MSG_MAP(CComment)
ON_EN_CHANGE(IDC_COMMENT, OnChangeComment)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CComment message handlers

void CComment::OnChangeComment()

{
	UpdateDlgCntls();
}

BOOL CComment::OnInitDialog()

{
	CDialog::OnInitDialog();

	UpdateDlgCntls();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

/****************************************************************************
*   					   CComment::updateControls
* Result: void
*   	
* Effect: 
*   	Updates teh controls
****************************************************************************/

void CComment::UpdateDlgCntls()

{
	c_OK.EnableWindow(c_Comment.GetWindowTextLength() > 0);
}
