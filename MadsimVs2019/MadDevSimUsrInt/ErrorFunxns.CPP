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
/*  Module  NAME : ErrorFunxns.cpp                                             */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
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
#include "ErrorFunxns.h"


/****************************************************************************
*   					CSimulatorDlg::OnUpdateProbabilities
* Inputs:
*   	WPARAM: code indicating which probability to update
*	LPARAM: new probability
* Result: LRESULT
*   	0, always
* Effect: 
*   	Stores the selected probability in its associated variable
****************************************************************************/

LRESULT CSimulatorDlg::OnUpdateProbabilities(WPARAM wParam, LPARAM lParam)

{
	switch (wParam)
		{
			/* which one */
		case PROB_ERR:
				{
				RegistryInt pErr(IDS_PROB_ERR);
				m_ulProbGenlErr = (DWORD)lParam;
				pErr.value = (int)lParam;
				pErr.store();
				}
			break;

		case PROB_LOST:
				{
				RegistryInt pLost(IDS_PROB_LOST);
				m_ulProbLost = (DWORD)lParam;
				pLost.value = (int)lParam;
				pLost.store();
				}
			break;

		case PROB_SPURIOUS:
				{
				RegistryInt pSpurious(IDS_PROB_SPURIOUS);
				m_ulProbSpurious = (DWORD)lParam;
				pSpurious.value = (int)lParam;
				pSpurious.store();
				}
			break; 

		case PROB_OVRUND:
				{
				RegistryInt pOvrUnd(IDS_PROB_OVR_UND);
				m_ulProbOvrUnd = (DWORD)lParam;						   
				pOvrUnd.value = (int)lParam;
				pOvrUnd.store();
				}
			break;

		case PROB_DEVBUSY:
				{
				RegistryInt pDevBusy(IDS_PROB_BUSY);
				m_ulProbDevBusy = (DWORD)lParam;
				pDevBusy.value = (int)lParam;
				pDevBusy.store();
				}
			break;

		default:;
		} //* end switch

	return 0;
}



VOID  CSimulatorDlg::InjectAnyErrs(PMADREGS pMadRegs)

{
//ULONG StatusReg = pMadRegs->Status;
 
	if ((pMadRegs->IntEnable & MAD_INT_STATUS_ALERT_BIT) != 0)
	    {    
        if (InjectGenlErr()) 
		    {
	 	    pMadRegs->Status |= MAD_STATUS_GENERAL_ERR_BIT;
			pMadRegs->IntID |= MAD_INT_STATUS_ALERT_BIT;
		    }

        if (InjectDevBusy())
		    {
	        pMadRegs->Status |= MAD_STATUS_DEVICE_BUSY_BIT;
			pMadRegs->IntID |= MAD_INT_STATUS_ALERT_BIT;
		    }
        }

 	return;
}


/****************************************************************************
*   					   CSimulatorDlg::InjectGenlErr
* Result: BOOL
*   	TRUE if an error should be injected
*	FALSE for no error
* Notes: 
*   	If error injection is disabled, this always returns FALSE
*	If error injection is enabled, this will be based on the probability
*	of error set by the configuration panel and stored in ProbErr
****************************************************************************/

BOOL CSimulatorDlg::InjectGenlErr()

{
	return Probability(m_cbGenlErrs, m_ulProbGenlErr);
}



/****************************************************************************
*   					   CSimulatorDlg::InjectOvrUnd
* Result: BOOL
*   	TRUE if an OVR/UND error should be injected
*	FALSE if not
* Notes: 
*   	
****************************************************************************/

BOOL CSimulatorDlg::InjectOvrUnd()

{
	return Probability(m_cbOvrUndrErrs, m_ulProbOvrUnd);
}

/****************************************************************************
*   					   CSimulatorDlg::InjectDevBusy
* Result: BOOL
*   	TRUE if an Device Busy error should be injected
*	FALSE if not
* Notes: 
*   	
****************************************************************************/

BOOL CSimulatorDlg::InjectDevBusy()

{
	return Probability(m_cbDevBusyErrs, m_ulProbDevBusy);
}



/****************************************************************************
*   						   CSimulatorDlg::Probability
* Inputs:
*   	CButton & ctl: Control to be checked
*	DWORD p: Probability value being an error (0..100)
* Result: BOOL
*   	TRUE if
*	FALSE if
****************************************************************************/

BOOL CSimulatorDlg::Probability(CButton& ctl, DWORD nProbVal)

{
DWORD nRand;

	if (ctl.GetCheck() == BST_UNCHECKED)
		return FALSE;

	if (nProbVal == 0)
		return FALSE;

	nRand = rand() % 100;
	if (nProbVal >= nRand)
		return TRUE;

	return FALSE;
}
