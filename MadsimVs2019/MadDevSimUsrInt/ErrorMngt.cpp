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
/*  Module  NAME : ErrorMngt.cpp                                               */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

// ErrorMngt.cpp : implementation file
//

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "regvars.h"
#include "NumericEdit.h"
#include "ErrMngtParms.h"
#include "ErrorMngt.h"
#include "help.h"
#include "uwm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInterruptMgt dialog


CInterruptMgt::CInterruptMgt(CWnd* pParent /*=NULL*/)
	: CDialog(CInterruptMgt::IDD, pParent)
{
	m_bInit = FALSE;
	//{{AFX_DATA_INIT(CInterruptMgt)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CInterruptMgt::DoDataExchange(CDataExchange* pDX)

{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInterruptMgt)
	DDX_Control(pDX, IDC_IntErrNone, c_None);
	DDX_Control(pDX, IDC_SPURIOUS, c_Spurious);
	DDX_Control(pDX, IDC_LOSE, c_Lose);
	DDX_Control(pDX, IDC_BOTH, c_Both);

	DDX_Control(pDX, IDC_PERR_CAPTION, c_c_ProbErr);
	DDX_Control(pDX, IDC_PLOSE, c_ProbLose);
	DDX_Control(pDX, IDC_SPINLOSE, c_SpinLose);
	DDX_Control(pDX, IDC_PSPURIOUS, c_ProbSpurious);
	DDX_Control(pDX, IDC_SPINSPURIOUS, c_SpinSpurious);

	DDX_Control(pDX, IDC_PERR, c_ProbErr);
	DDX_Control(pDX, IDC_SPINPERR, c_SpinErr);
	DDX_Control(pDX, IDC_POVR, c_ProbOvr);
	DDX_Control(pDX, IDC_SPINPOVR, c_SpinOvr);
	DDX_Control(pDX, IDC_PROB_DEVBUSY, c_ProbDevBusy);
	DDX_Control(pDX, IDC_SPIN_PROBTIMEOUT, c_SpinTimeout);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInterruptMgt, CDialog)
//{{AFX_MSG_MAP(CInterruptMgt)
//ON_BN_CLICKED(IDCANCEL, OnCloseDialog)
ON_BN_CLICKED(IDC_IntErrNone, OnNoIntErrs)
ON_BN_CLICKED(IDC_LOSE, OnLose)
ON_BN_CLICKED(IDC_SPURIOUS, OnSpurious)
ON_BN_CLICKED(IDC_BOTH, OnBoth)
ON_EN_CHANGE(IDC_PLOSE, OnChangePlose)
ON_EN_CHANGE(IDC_PSPURIOUS, OnChangePspurious)

ON_EN_CHANGE(IDC_PERR, OnChangePerr)
ON_EN_CHANGE(IDC_POVR, OnChangePovr)
ON_EN_CHANGE(IDC_PROB_DEVBUSY, OnChangePDevBusy)
//ON_BN_CLICKED(IDC_HELP_COMMAND, OnHelp)
//ON_WM_DESTROY()
//ON_BN_CLICKED(IDC_CLOSE, OnCloseDialog)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//* Overriding DoModal member function to pass parameters
//*
UINT CInterruptMgt::DoModal(etIntErrStyle* peIntErrStyle)

{
UINT nDM;

    m_eIntErrStyle = *peIntErrStyle;

	nDM = (UINT)CDialog::DoModal(); //Invoke Base member funxn 1st
	if (nDM == IDCANCEL)
		return IDCANCEL;

   *peIntErrStyle =  m_eIntErrStyle;

	return IDOK;
}

/////////////////////////////////////////////////////////////////////////////
// CInterruptMgt message handlers


/****************************************************************************
*   					 CInterruptMgt::updateControls
* Result: void
*   	
* Effect: 
*   	Updates all the controls
****************************************************************************/

void CInterruptMgt::UpdateControls()

{
	BOOL enable = c_Spurious.GetCheck() == BST_CHECKED ||
		c_Both.GetCheck() == BST_CHECKED;
	BOOL caption = enable;

	c_SpinSpurious.EnableWindow(enable);
	c_ProbSpurious.EnableWindow(enable);

	enable = c_Lose.GetCheck() == BST_CHECKED ||
		c_Both.GetCheck() == BST_CHECKED;

	caption |= enable;

	c_SpinLose.EnableWindow(enable);
	c_ProbLose.EnableWindow(enable);
	c_SpinTimeout.EnableWindow(enable);

	caption |= c_Both.GetCheck() == BST_CHECKED;

	c_c_ProbErr.EnableWindow(caption);

	return;
}


/****************************************************************************
*   					  CInterruptMgt::OnInitDialog
* Result: BOOL
*   	
* Effect: 
*   	Initializes all the controls
****************************************************************************/

BOOL CInterruptMgt::OnInitDialog()

{
	m_bInit = FALSE;

	CDialog::OnInitDialog();

    switch (m_eIntErrStyle)
		{
		case eNoIntErrs:
			c_None.SetCheck(BST_CHECKED);
            break;
		case eLoseInts:
	       	c_Lose.SetCheck(BST_CHECKED);
            break;
		case eXtraInts:
			c_Spurious.SetCheck(BST_CHECKED);
            break;
		case eBothErrs:
	       	c_Both.SetCheck(BST_CHECKED);
            break;
        default:
			c_None.SetCheck(BST_CHECKED);
 		} //* end sw

	c_SpinSpurious.SetRange(0, 100);
	c_SpinOvr.SetRange(0, 100);
	c_SpinErr.SetRange(0, 100);
	c_SpinLose.SetRange(0, 100);
	c_SpinTimeout.SetRange(0, 100);

	RegistryInt pSpurious(IDS_PROB_SPURIOUS);
	pSpurious.load(DefltProb);
	c_SpinSpurious.SetPos(pSpurious.value);

	RegistryInt pOvrUnd(IDS_PROB_OVR_UND);
	pOvrUnd.load(DefltProb);
	c_SpinOvr.SetPos(pOvrUnd.value);

	RegistryInt pErr(IDS_PROB_ERR);
	pErr.load(DefltProb);
	c_SpinErr.SetPos(pErr.value);

	RegistryInt pLost(IDS_PROB_LOST);
	pLost.load(DefltProb);
	c_SpinLose.SetPos(pLost.value);

	RegistryInt pTimeOut(IDS_PROB_TIMEOUT);
	pTimeOut.load(DefltProb);
	c_SpinTimeout.SetPos(pTimeOut.value);

	UpdateControls();
	m_bInit = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
}				  // EXCEPTION: OCX Property Pages should return FALSE


//*
void CInterruptMgt::OnNoIntErrs()

{
    m_eIntErrStyle = eNoIntErrs;
	UpdateControls();	

	return;
}


//*
void CInterruptMgt::OnLose()

{
    m_eIntErrStyle = eLoseInts;
	UpdateControls();

	return;
}

//*
void CInterruptMgt::OnSpurious()

{
    m_eIntErrStyle = eXtraInts;
	UpdateControls();	

	return;
}



//*
void CInterruptMgt::OnBoth()

{
    m_eIntErrStyle = eBothErrs;
	UpdateControls();

    return;
}


//*
void CInterruptMgt::OnChangePlose()

{
	if (!m_bInit)
		return;

	if (c_ProbLose.GetWindowInt() > 100)
		c_ProbLose.SetWindowText(100);

	GetParent()->SendMessage(UWM_SET_PROB,
				 	PROB_LOST,
				 	c_ProbLose.GetWindowInt());

	return;
}


void CInterruptMgt::OnChangePspurious()

{
	if (!m_bInit)
		return;

	if (c_ProbSpurious.GetWindowInt() > 100)
		c_ProbSpurious.SetWindowText(100);

	GetParent()->SendMessage(UWM_SET_PROB,
				 	PROB_SPURIOUS,
				 	c_ProbSpurious.GetWindowInt());

	return;
}



void CInterruptMgt::OnChangePerr()

{
	if (!m_bInit)
		return;

	if (c_ProbErr.GetWindowInt() > 100)
		c_ProbErr.SetWindowText(100);	

	GetParent()->SendMessage(UWM_SET_PROB, PROB_ERR, c_ProbErr.GetWindowInt());

	return;
}


void CInterruptMgt::OnChangePovr()

{
	if (!m_bInit)
		return;

	if (c_ProbOvr.GetWindowInt() > 100)
		c_ProbOvr.SetWindowText(100);

	GetParent()->SendMessage(UWM_SET_PROB, PROB_OVRUND, c_ProbOvr.GetWindowInt());

	return;
}


void CInterruptMgt::OnChangePDevBusy()

{
WORD wParm;

	if (!m_bInit)
		return;

	wParm = (WORD)c_ProbDevBusy.GetWindowInt();
	if (wParm > 100)
		{
		wParm = 100; //* % 0 - 100
		c_ProbDevBusy.SetWindowText(100);
		}

	GetParent()->SendMessage(UWM_SET_PROB, PROB_DEVBUSY, wParm);

	return;
}

/****************************************************************************
*   						 CInterruptMgt::OnHelp
* Result: void
*   	
* Effect: 
*   	Invokes help
****************************************************************************/

void CInterruptMgt::OnHelp()

{
	WinHelp(HID_INTERRUPT_MGT, HELP_CONTEXT);
}

/****************************************************************************
*   					   CInterruptMgt::OnDestroy
* Result: void
*   	
* Effect: 
*   	Notifies the parent that the window is shutting down
****************************************************************************/

void CInterruptMgt::OnDestroy()

{
//	GetParent()->SendMessage(UWM_IMGR_CLOSE);	

	CDialog::OnDestroy();
}


//*
void CInterruptMgt::OnCloseDialog()

{
	DestroyWindow();	

	return;
}


//****
void CInterruptMgt::PostNcDestroy()

{
//	delete this; //* Now lays an egg after we Override DoModal above

	CDialog::PostNcDestroy();

	return;
}
