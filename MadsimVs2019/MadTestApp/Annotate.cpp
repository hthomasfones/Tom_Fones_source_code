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
/*  Module  NAME : Annotate.cpp                                                */
/*                                                                             */
/*  DESCRIPTION  : Implementation of the MFC class Annotate                    */
/*                                                                             */
/*******************************************************************************/


// Annotate.cpp : implementation file
//

#include "stdafx.h"
#include "MadTestApp.h"
#include "Annotate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAnnotate dialog


CAnnotate::CAnnotate(CWnd* pParent /*=NULL*/)
	: CDialog(CAnnotate::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAnnotate)
	m_csAnnotateText = _T("");
	//}}AFX_DATA_INIT
}


void CAnnotate::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAnnotate)
	DDX_Text(pDX, IDC_Annotation, m_csAnnotateText);
	DDV_MaxChars(pDX, m_csAnnotateText, 250);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAnnotate, CDialog)
	//{{AFX_MSG_MAP(CAnnotate)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAnnotate message handlers

int CAnnotate::DoModal(PCHAR pChar) 

{
UINT uRC;

	uRC = (UINT)CDialog::DoModal();
	if (uRC == IDCANCEL)
		return IDCANCEL;

	strcpy_s(pChar, 100, m_csAnnotateText.GetBuffer(1));

    return uRC;
}
