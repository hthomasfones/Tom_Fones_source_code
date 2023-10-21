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
/*  Module  NAME : RegDisplay.cpp                                              */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

// RegDisplay.cpp : implementation file
//

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "RegData.h"
#include "RegDisplay.h"
#include "uwm.h"
#include "IoCtl_Cls.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegDisplay dialog


CRegDisplay::CRegDisplay(CWnd* pParent /*=NULL*/)
	: CDialog(CRegDisplay::IDD, pParent)
	, MesgID(0)
{
	//{{AFX_DATA_INIT(CRegDisplay)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CRegDisplay::DoDataExchange(CDataExchange* pDX)

{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegDisplay)
	DDX_Control(pDX, IDC_RD_MESGID,      m_ceMesgID);
	DDX_Control(pDX, IDC_RD_CONTROL,     m_ceControl);
	DDX_Control(pDX, IDC_RD_STATUS,      m_ceStatus);
	DDX_Control(pDX, IDC_RD_INT_ACTV,    m_ceIntEnable);
	DDX_Control(pDX, IDC_RD_INT_ID,      m_ceIntID);
	DDX_Control(pDX, IDC_RD_IN_LEN,      m_cePioCacheReadLen);
	DDX_Control(pDX, IDC_RD_OUT_LEN,     m_cePioCacheWriteLen);
	//DDX_Control(pDX, IDC_RD_DATA1,       m_cData1);
	//DDX_Control(pDX, IDC_RD_DATA2,       m_cData2);
	//DDX_Control(pDX, IDC_RD_DATA3,       m_cData3);
	DDX_Control(pDX, IDC_RD_POWER_STATE, m_cPowerState);

	//DDX_Control(pDX, IDC_RD_IN_DMAHI,    m_ceInDMA_Hi);
	//DDX_Control(pDX, IDC_RD_IN_DMALO,    m_ceInDMA_Lo);
	//DDX_Control(pDX, IDC_RD_OUT_DMAHI,   m_ceOutDMA_Hi);
	//DDX_Control(pDX, IDC_RD_OUT_DMALO,   m_ceOutDMA_Lo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRegDisplay, CDialog)
//{{AFX_MSG_MAP(CRegDisplay)
ON_WM_CLOSE()
ON_WM_DESTROY()
ON_REGISTERED_MESSAGE(UWM_UPDATE_REGS, OnUpdateRegs)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegDisplay message handlers

BOOL CRegDisplay::OnInitDialog()

{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

/****************************************************************************
*   						   CRegDisplay::OnOK
* Result: void
*   	
* Effect: 
*   	Ignores ENTER key
****************************************************************************/

void CRegDisplay::OnOK()

{
	// do nothing
}

/****************************************************************************
*   						 CRegDisplay::OnCancel
* Result: void
*   	
* Effect: 
*   	Ignores ESC key
****************************************************************************/

void CRegDisplay::OnCancel()

{
	// does nothing
}

void CRegDisplay::OnClose()

{
	DestroyWindow(); // closes modeless dialog
}

void CRegDisplay::PostNcDestroy()

{
	CDialog::PostNcDestroy();
}

void CRegDisplay::OnDestroy()

{
	CDialog::OnDestroy();

	GetParent()->SendMessage(UWM_REG_CLOSE);
}

/****************************************************************************
*   					   CRegDisplay::OnUpdateRegs
* Inputs:
*   	WPARAM: ignored
*	LPARAM: (LPARAM)(LPHDW_SIM_REGS)
* Result: LRESULT
*   	0, always
* Effect: 
*   	Updates the register display
****************************************************************************/

LRESULT CRegDisplay::OnUpdateRegs(WPARAM, LPARAM lParam)

{
ULONG ulTemp;
//USHORT uTemp;
PMADREGS   pMadRegs = (PMADREGS)lParam;
CString cs16Bit;
CString csHexAddr;
char szInfoMsg[100] = "";
//UINT uRC;

	//memcpy(szInfoMsg, pRegs, sizeof(MAD_REGS));
    //sprintf_s(szInfoMsg, "Regs = %2X, %2X, %2X, %2X, %2X, %2X %2X",
	//	    pRegs->RegArray[0],pRegs->RegArray[1],pRegs->RegArray[2],
	//		pRegs->RegArray[3],pRegs->RegArray[4],pRegs->RegArray[5],
	//		pRegs->RegArray[6]);
	//uRC = ::MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

    ulTemp = (ULONG)(pMadRegs->MesgID);
    cs16Bit.Format("%d", ulTemp);
 	m_ceMesgID.SetWindowText(cs16Bit);

    cs16Bit.Format("%04X", pMadRegs->Control);
 	m_ceControl.SetWindowText(cs16Bit);

    cs16Bit.Format("%04X", pMadRegs->Status);
	m_ceStatus.SetWindowText(cs16Bit);

    cs16Bit.Format("%04X", pMadRegs->IntEnable);
 	m_ceIntEnable.SetWindowText(cs16Bit);

    cs16Bit.Format("%04X", pMadRegs->IntID);
    m_ceIntID.SetWindowText(cs16Bit);

    cs16Bit.Format("%d", pMadRegs->PioCacheReadLen);
 	m_cePioCacheReadLen.SetWindowText(cs16Bit);

    cs16Bit.Format("%d", pMadRegs->PioCacheWriteLen);
 	m_cePioCacheWriteLen.SetWindowText(cs16Bit);

	//m_cData1.SetWindowText(pRegs->RegArray[MADREG_DATA1]);
	//m_cData2.SetWindowText(pRegs->RegArray[MADREG_DATA2]);
	//m_cData3.SetWindowText(pRegs->RegArray[MADREG_DATA3]);

	m_cPowerState.SetWindowText((BYTE)pMadRegs->PowerState);
 
	return 0;
}
