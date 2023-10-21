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
/*                                                                             */
/*  Module  NAME : HdwTrace.cpp                                                */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

// HdwTrace.cpp : implementation file
//

#include "stdafx.h"
#include <winioctl.h>
#include "resource.h"
#include "IoCtl_Cls.h"
#include "MadDevSimUsrInt.h"
#include "HdwTrace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHdwTrace dialog


CHdwTrace::CHdwTrace(CWnd* pParent /*=NULL*/)
	: CDialog(CHdwTrace::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHdwTrace)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CHdwTrace::DoDataExchange(CDataExchange* pDX)

{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHdwTrace)
	DDX_Control(pDX, IDC_TRC_IOCTLS, c_IoctlTrace);
	DDX_Control(pDX, IDC_TRC_REGS,   c_RegTrace);
	DDX_Control(pDX, IDC_TRC_INTS,   c_IntTrace);
	DDX_Control(pDX, IDOK, c_OK);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHdwTrace, CDialog)
//{{AFX_MSG_MAP(CHdwTrace)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//***
//*
BOOL CHdwTrace::OnInitDialog()

{
	CDialog::OnInitDialog();

	if (m_pIoCtlObj == NULL)
		{
		/* whoops */
		c_IoctlTrace.EnableWindow(FALSE);
		c_RegTrace.EnableWindow(FALSE);
		c_OK.EnableWindow(FALSE);
		} /* whoops */
	else
		{
		/* ok */
		DWORD ulMask = 0; // m_pIoCtlObj->GetTrace();
		c_IoctlTrace.SetCheck((ulMask & DEBUGMASK_TRACE_IOCTLS) ?
					 	       BST_CHECKED : BST_UNCHECKED);
		c_RegTrace.SetCheck((ulMask & DEBUGMASK_TRACE_REGISTERS) ?
				   	         BST_CHECKED : BST_UNCHECKED);
		c_IntTrace.SetCheck((ulMask & DEBUGMASK_TRACE_INTS) ?
				   	         BST_CHECKED : BST_UNCHECKED);
		} /* ok */

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
// CHdwTrace message handlers
//* Fetch the existing Debug Flags
//* Update by ORing on values SET or ANDing off values RESET by the user.
//* 
void CHdwTrace::OnOK()

{
DWORD ulMask = 0; //m_pIoCtlObj->GetTrace();

#if 0
	ulMask &= ~(DEBUGMASK_TRACE_REGISTERS |
		        DEBUGMASK_TRACE_IOCTLS    | DEBUGMASK_TRACE_INTS);

	if (c_IoctlTrace.GetCheck() == BST_CHECKED)
		ulMask = ulMask | DEBUGMASK_TRACE_IOCTLS;
	else
		ulMask = ulMask & ~DEBUGMASK_TRACE_IOCTLS;

	if (c_RegTrace.GetCheck() == BST_CHECKED)
		ulMask = ulMask | DEBUGMASK_TRACE_REGISTERS;
	else
		ulMask = ulMask & ~DEBUGMASK_TRACE_REGISTERS;

	if (c_IntTrace.GetCheck() == BST_CHECKED)
		ulMask = ulMask | DEBUGMASK_TRACE_INTS;
	else
		ulMask = ulMask & ~DEBUGMASK_TRACE_INTS;

	m_pIoCtlObj->SetTrace(ulMask);
#endif

	CDialog::OnOK();

	return;
}

