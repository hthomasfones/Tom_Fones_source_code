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
/*  Exe file ID  : MadTestApp.exe                                              */
/*                                                                             */
/*  Module  NAME : DisplayIo_Dlg.cpp                                           */
/*                                                                             */
/*  DESCRIPTION  : Implementation of the MFC class DisplayIO_Dlg               */
/*                                                                             */
/*******************************************************************************/


// DisplayIO_Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "MadTestApp.h"
#include "DisplayIO_Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisplayIO_Dlg dialog


CDisplayIO_Dlg::CDisplayIO_Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDisplayIO_Dlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDisplayIO_Dlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDisplayIO_Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDisplayIO_Dlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDisplayIO_Dlg, CDialog)
	//{{AFX_MSG_MAP(CDisplayIO_Dlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisplayIO_Dlg message handlers
