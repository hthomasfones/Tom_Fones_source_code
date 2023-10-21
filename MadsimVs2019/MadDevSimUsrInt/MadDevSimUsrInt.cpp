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
/*  Module  NAME : MadDevSimUIDlg.cpp                                          */
/*                                                                             */
/*  DESCRIPTION  : Defines the class behaviors for the application.            */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
//#include "PacketTerm.h"
#include "Automatic.h"
#include "MadDevSimUsrInt.h"
#include <sys\timeb.h>
#include "TraceWnd.h"
#include "RegVars.h"
#include "SpinnerButton.h"
#include "IoCtl_Cls.h" //Includes "MadDefinition.h" "MadBusUsrIntBufrs.h" 
#include "RegData.h"
#include "RegDisplay.h"
#include "NumericEdit.h"
#include "ErrorMngt.h"
#include "MadDevSimUIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSimulatorApp

BEGIN_MESSAGE_MAP(CSimulatorApp, CWinApp)
//{{AFX_MSG_MAP(CSimulatorApp)
ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
//}}AFX_MSG_MAP
ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSimulatorApp construction

CSimulatorApp::CSimulatorApp()

{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSimulatorApp object

CSimulatorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSimulatorApp initialization


#pragma warning(suppress: 6262)
BOOL CSimulatorApp::InitInstance()

{
CSimulatorDlg  dlg;
int rc;

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

//#ifdef _AFXDLL
//    Enable3dControls();			// Call this when using MFC in a shared DLL
//#else
//    Enable3dControlsStatic();	// Call this when linking to MFC statically
//#endif

#pragma warning(suppress: 28159)
    rc = ::WinExec(MadTestAppCmd, SW_SHOW);

    m_pMainWnd = &dlg;
    int nResponse = (int)dlg.DoModal();
    if (nResponse == IDOK)
        {
        }
    else 
		if (nResponse == IDCANCEL)
            {
            }

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}

/****************************************************************************
*   					 CSimulatorApp::OnContextHelp
* Result: void
*   	
* Effect: 
*   	Switches to arrow-question cursor to support context help
****************************************************************************/

void CSimulatorApp::OnContextHelp()

{
    CWinApp::OnContextHelp();
}

