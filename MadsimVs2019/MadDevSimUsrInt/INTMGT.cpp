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
/*  Module  NAME : IntMgt.cpp                                                  */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                 functions associated solely with interupt/error management  */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "Automatic.h"
#include "MadDevSimUsrInt.h"
#include <sys\timeb.h>
#include "TraceWnd.h"
#include "Regvars.h"
#include "NumericEdit.h"
#include "ErrMngtParms.h"
#include "ErrorMngt.h"
#include "SpinnerButton.h"
#include "IoCtl_Cls.h"
#include "RegData.h"
#include "RegDisplay.h"
#include "MadDevSimUIDlg.h"
#include "AutoExecDlg.h"
#include "About.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



void CSimulatorDlg::OnXtraInts()

{
	LogErrorChange(m_cbXtraInts);
	
	return;
}



void CSimulatorDlg::OnRandErrs()

{
	LogErrorChange(m_cbGenlErrs);
	
	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnOvrUnd
* Result: void
*   	
* Effect: 
*   	Enables overrun (input) and underrun (output) errors to be injected
*	automatically during free running
****************************************************************************/
void CSimulatorDlg::OnOvrUndErrs()

{
	LogErrorChange(m_cbOvrUndrErrs);

	return;
}


void CSimulatorDlg::OnDevBusyErrs()

{
	LogErrorChange(m_cbDevBusyErrs);

	return;
}


/****************************************************************************
*   					  CSimulatorDlg::OnImgrClose
* Inputs:
*   	WPARAM: ignored
*	LPARAM: ignored
* Result: LRESULT
*   	0, always
* Effect: 
*   	The imgr has closed
****************************************************************************/

LRESULT CSimulatorDlg::OnImgrClose(WPARAM, LPARAM)

{
	m_pIntMngr = NULL;

	return 0;
}

