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
/*  Module  NAME : AutoExecDlg.cpp                                             */
/*                                                                             */
/*  DESCRIPTION  : Function definitions for the AutoExecDlg class              */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "AutoExecDlg.h"
#include "help.h"
#include "regvars.h"
//#include "uwm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSetupDlg dialog


CSetupDlg::CSetupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSetupDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSetupDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSetupDlg::DoDataExchange(CDataExchange* pDX)

{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSetupDlg)
	DDX_Control(pDX, IDC_SPINGODONEVAR, c_SpinGoDoneVariance);
	DDX_Control(pDX, IDC_SPINGODONEMIN, c_SpinGoDoneMin);
	DDX_Control(pDX, IDC_GODONE_VAR, c_GoDoneVariance);
	DDX_Control(pDX, IDC_GODONE_MIN, c_GoDoneMin);
	DDX_Control(pDX, IDC_SPINPOLLINTERVAL, c_SpinPollingInterval);
//	DDX_Control(pDX, IDC_POLLINTERVAL, c_PollingInterval);
	DDX_Text(pDX, IDC_POLLINTERVAL, m_nPollIntrvl);
	DDV_MinMaxLong(pDX, m_nPollIntrvl, 10, 2000);
	DDX_Check(pDX, IDC_RandomPkts, m_bGenRndmPkts);
	DDX_Control(pDX, IDOK, c_OK);
	DDX_Control(pDX, IDC_IRQ, c_IRQ);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetupDlg, CDialog)
//{{AFX_MSG_MAP(CSetupDlg)
//ON_CBN_SELENDOK(IDC_IRQ, OnSelendokIrq)
//ON_BN_CLICKED(IDC_RandomPkts, OnRandomPkts)
//ON_BN_CLICKED(IDC_SETUP_HELP, OnHelp)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//* Overriding DoModal member function to pass parameters
//*
UINT CSetupDlg::DoModal(PAUTOEXECUTE_PARMS pAutoExecParms) 

{
UINT nDM;

    m_bGenRndmPkts = (int)pAutoExecParms->bGenRandomPkts;
    m_nPollIntrvl  = pAutoExecParms->nPollInt; 

	nDM = (UINT)CDialog::DoModal(); //Invoke Base member funxn 1st
	if (nDM == IDCANCEL)
		return IDCANCEL;

    pAutoExecParms->nPollInt       = m_nPollIntrvl; 
    pAutoExecParms->bGenRandomPkts = m_bGenRndmPkts; 

	return IDOK;
}


/////////////////////////////////////////////////////////////////////////////
// CSetupDlg message handlers

/***********
void CSetupDlg::OnSelendokIrq()

{
	UpdateSetupControls();
}
*************/

//void CSetupDlg::OnRandomPkts()

//{
//	GetParent()->SendMessage(UWM_GEN_RANDOM_PKTS, m_cbRandomPkts.GetCheck(), 0L);

//	return;
//}

/********
void CSetupDlg::OnOK()

{
	RegistryInt interval(IDS_POLLING_INTERVAL);
	interval.value = c_SpinPollingInterval.GetPos();
	interval.store();

	RegistryInt minimum(IDS_GODONEMIN);
	minimum.value = c_SpinGoDoneMin.GetPos();
	minimum.store();

	RegistryInt variance(IDS_GODONEVAR);
	variance.value = c_SpinGoDoneVariance.GetPos();
	variance.store();

	int n = c_IRQ.GetCurSel();
	if (n != CB_ERR)
		{
		//* success  
		RegistryInt regIRQ(IDS_IRQ);
		regIRQ.value = c_IRQ.GetItemData(n);
		regIRQ.store();
		} //* success  

	CDialog::OnOK();
}
*******************/

/****************************************************************************
*   					   CSetupDlg::UpdateSetupControls
* Result: void
*   	
* Effect: 
*   	Enables/disables controls
****************************************************************************/

void CSetupDlg::UpdateSetupControls()

{
}

/****************************************************************************
*   						CSetupDlg::OnInitDialog
* Result: BOOL
*   	TRUE, always
* Effect: 
*   	Initializes controls
****************************************************************************

BOOL CSetupDlg::OnInitDialog()

{
	CDialog::OnInitDialog();

//    m_cbRandomPkts.SetCheck((int)m_nGenRndmPkts);

	RegistryInt interval(IDS_POLLING_INTERVAL);
	interval.load(m_nPollIntrvl); // default to 50 ms
	c_SpinPollingInterval.SetRange(10, 10000); // 0.010 to 10.000 seconds
	c_SpinPollingInterval.SetPos(interval.value);

//*** Used for Polled device simulation
//*
/**************
	RegistryInt minimum(IDS_GODONEMIN);
	minimum.load(500);
	c_SpinGoDoneMin.SetRange(0, 10000);
	c_SpinGoDoneMin.SetPos(minimum.value);

	RegistryInt variance(IDS_GODONEVAR);
	variance.load(500);
	c_SpinGoDoneVariance.SetRange(0, 10000);
	c_SpinGoDoneVariance.SetPos(variance.value);

//******************** /

	//UpdateSetupControls();

	return TRUE;  // return TRUE unless you set the focus to a control
}
*********************************/


void CSetupDlg::OnHelp()

{
//	WinHelp(HID_SETUP_DIALOG, HELP_CONTEXT);
}
