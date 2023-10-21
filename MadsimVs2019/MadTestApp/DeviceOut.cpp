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
/*  Module  NAME : DeviceOut.cpp                                               */
/*                                                                             */
/*  DESCRIPTION  : Implementation of the MFC class DeviceOut                   */
/*                                                                             */
/*******************************************************************************/

// DeviceOut.cpp : implementation file
//

#include "stdafx.h"
#include "MadTestApp.h"
#include "DeviceOut.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HANDLE ghDevice;
extern BOOL   gbBinary;


/////////////////////////////////////////////////////////////////////////////
// CDeviceOut dialog


CDeviceOut::CDeviceOut(CWnd* pParent /*=NULL*/)
	: CDialog(CDeviceOut::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDeviceOut)
	m_CSwritepacket = _T("");
	//}}AFX_DATA_INIT
}


void CDeviceOut::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeviceOut)
	DDX_Text(pDX, IDC_Output, m_CSwritepacket);
	DDV_MaxChars(pDX, m_CSwritepacket, 250);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeviceOut, CDialog)
	//{{AFX_MSG_MAP(CDeviceOut)
	ON_EN_CHANGE(IDC_Output, OnChangeOutput)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//* overriden DoModal Function
//*
UINT CDeviceOut::DoModal(char* pChar)

{
LONG nLen;
LONG nDM;
//int rc;
CString csInfoMsg;

    nDM = (LONG)CDialog::DoModal(); //Invoke Base member funxn 1st
    if (nDM == IDOK)
        {
        nLen = m_CSwritepacket.GetLength();
        if (nLen > 0)
            strcpy_s(pChar, 100, m_CSwritepacket.GetBuffer(1));
        }

    return nDM;
}    

/////////////////////////////////////////////////////////////////////////////
// CDeviceOut message handlers

void CDeviceOut::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CDeviceOut::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void CDeviceOut::OnChangeOutput() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
}
