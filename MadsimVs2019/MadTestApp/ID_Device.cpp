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
/*  Module  NAME : ID_Device.cpp                                               */
/*                                                                             */
/*  DESCRIPTION  : Implementation of the MFC class ID_Device                   */
/*                                                                             */
/*******************************************************************************/


// ID_Device.cpp : implementation file
//

#include "stdafx.h"
#include "MadTestApp.h"
#include "ID_Device.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern char gszDeviceName[];
/////////////////////////////////////////////////////////////////////////////
// ID_Device dialog


ID_Device::ID_Device(CWnd* pParent /*=NULL*/)
	: CDialog(ID_Device::IDD, pParent)
{
	//{{AFX_DATA_INIT(ID_Device)
	m_DeviceName = _T("");
	//}}AFX_DATA_INIT
}


void ID_Device::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ID_Device)
	DDX_Text(pDX, IDC_DeviceID, m_DeviceName);
	DDV_MaxChars(pDX, m_DeviceName, 30);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ID_Device, CDialog)
	//{{AFX_MSG_MAP(ID_Device)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ID_Device message handlers

void ID_Device::OnOK() 

{
	CDialog::OnOK();

    strcpy_s(gszDeviceName, 150, m_DeviceName.GetBuffer(1));
        
    return;
}
