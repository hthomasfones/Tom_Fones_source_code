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
/*  Module  NAME : RegData.cpp                                                 */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

// RegData.cpp : implementation file
//

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "RegData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegData

CRegData::CRegData()

{
}

CRegData::~CRegData()

{
}


BEGIN_MESSAGE_MAP(CRegData, CEdit)
//{{AFX_MSG_MAP(CRegData)
ON_WM_CHAR()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegData message handlers


/****************************************************************************
*   						CRegData::SetWindowText
* Inputs:
*   	BYTE value: Value to set
* Result: void
*   	
* Effect: 
*   	Converts the byte to hex and stores it
****************************************************************************/

void CRegData::SetWindowText(BYTE value)

{
CString s;
CString t;

    s.Format(_T("%02X"), value);
    CEdit::GetWindowText(t);
    if (s != t) // avoid flicker!
	    CEdit::SetWindowText(s);

    return;
}
