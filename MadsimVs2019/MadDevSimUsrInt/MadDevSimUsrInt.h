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
/*  Exe file ID  : MadBus.sys, MadDevice.sys, MadSimUI.exe, MadTestApp.exe,    */
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : MadDevSimUrsInt.h                                           */
/*                                                                             */
/*  DESCRIPTION  : Defintion of the MadDevSimUsrInt class                      */
/*                                                                             */
/*******************************************************************************/

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CSimulatorApp:
// See Simulator.cpp for the implementation of this class
//

class CSimulatorApp: public CWinApp
    {
    public:
    													CSimulatorApp();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSimulatorApp)
    public:
    virtual BOOL	InitInstance();
    //}}AFX_VIRTUAL

    // Implementation

    //{{AFX_MSG(CSimulatorApp)
    afx_msg void	OnContextHelp();
    //}}AFX_MSG
    													DECLARE_MESSAGE_MAP()
    };


/////////////////////////////////////////////////////////////////////////////
