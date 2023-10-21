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
/*  Module  NAME : Manual.cppp                                                 */
/*                                                                             */
/*  DESCRIPTION  : Function definitions for the simulator dialog box           */
/*                 message handlers                                            */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include <io.h>
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
#include "uwm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
*   						CSimulatorDlg::OnManual
* Result: void
*   	
* Effect: 
*	Queues up a request to terminate freerun mode
* Notes:
*	This posts a message so it falls behind the next UWM_POLL and thus
*	prevents a deadlock situation
****************************************************************************/

void CSimulatorDlg::OnManual()

{
	if (m_bManual) // already in manual mode
		return; 

	//Issue a timeout error interrupt after stopping automatic mode
    PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
    pMadRegs->IntID  |= MAD_INT_STATUS_ALERT_BIT;
 	pMadRegs->Status = MAD_STATUS_TIMEOUT_ERROR_BIT;
 	LogInterrupt(_T("Timeout interrupt"));
	pMadRegs->MesgID = 1;

	SetRunMode(eManual); 
	PostMessage(UWM_SET_MANUAL_MODE);

    if (m_bAutoSave)
//#ifdef BLOCK_MODE
		_close((int)m_hOutput);
#if 0 //#else
		if (m_pFile != NULL) 
			{
			fflush(m_pFile);
		    fclose(m_pFile);
			m_pFile = NULL;
			}
#endif

	return;
}

/****************************************************************************
*   					  CSimulatorDlg::SetOnManualMode
* Inputs:
*   	WPARAM: ignored
*	LPARAM: ignored
* Result: LRESULT
*   	0, always
* Effect: 
*   	Forces the free-running polling thread to stop after it completes its
*	current cycle
****************************************************************************/

LRESULT CSimulatorDlg::OnSetManualMode(WPARAM single, LPARAM)

{
DWORD interval;

	// We do a WaitForSingleObject here so we don't try to stop in the
	// middle of a cycle

	// The polling loop looks like this
	//
	//	WaitForSingleObject(freerun);
	//  	ResetEvent(stop) 	// block out stoppability
	//   *	ResetEvent(pause);	// wait for reply
	//   *	PostMessage		// will SetEvent(pause) when done
	//   *	WaitForSingleObject(pause); //
	//   	SetEventStop
	//
	::SetEvent(m_hPause);     // release pause that is queued up
	::ResetEvent(m_hFreeRun); // shut down freerun loop

	if (m_ulPollIntrvl < 1000)
		interval = 10 * m_ulPollIntrvl;
	else
		interval = 2 * m_ulPollIntrvl;

	switch (::WaitForSingleObject(m_hStop, interval))
		{
			/* wait */
		case WAIT_TIMEOUT:
			if (m_bTrcDetail)
				{
				CString s;
#pragma warning(suppress: 6031)
				s.LoadString(IDS_STOP_TIMEOUT);
				TraceItem* e = new TraceItem(TRACE_TYPE_ERROR, s);
				m_wndTrace.AddString(e);
				}

			PostMessage(UWM_SET_MANUAL_MODE); // resync state
			return 0; // don't go any further

		case WAIT_OBJECT_0:
			break;  // proceed with kill
		default:
				{
				CString s;	
#pragma warning(suppress: 6031)
				s.LoadString(IDS_STOP_ERROR);
				TraceItem* e = new TraceItem(TRACE_TYPE_ERROR, s);
				m_wndTrace.AddString(e);
				}
			return 0;
		} //* end sw - wait

	TraceItem* item = new TraceItem(TRACE_TYPE_ANNOTATION,
						  	TRUE,
						  	single ? _T("End Single Step") : _T("Manual Mode"));
	m_wndTrace.AddString(item);

	//OpenRegisterTransaction(_T("OnManual"));
	UpdateDlgCntls();
	//CloseRegisterTransaction(_T("OnManual"));

	m_bManual      = TRUE;
	//m_bAutoInput   = FALSE;
	//m_bAutoOutput  = FALSE;
	m_bAutoDuplex = FALSE;

	return 0;
}

void CSimulatorDlg::OnBusyOut()

{
	ASSERT(m_bManual);
	//OpenRegisterTransaction(_T("OnBusyOut"));

	// Nothing to be done here

	//CloseRegisterTransaction(_T("OnBusyOut"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_15
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_15()

{
PMADREGS pMadRegs   = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus; //    = (USHORT)pMadRegs->Status;;
USHORT   uNewStatus; // = uStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_15.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewStatus &= ~MADMASK_BIT15; //* Clear the bit in the register
	else 
        uNewStatus |= MADMASK_BIT15; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}



/****************************************************************************
*   						CSimulatorDlg::OnStatus_14
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_14()

{
PMADREGS pMadRegs   = (PMADREGS)m_pDeviceRegs;
USHORT uStatus;
USHORT uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_14.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewStatus &= ~MADMASK_BIT14; //* Clear the bit in the register
	else 
        uNewStatus |= MADMASK_BIT14; //* Set the bit in the register

	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_13
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_13()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_13.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewStatus &= ~MADMASK_BIT13; //* Clear the bit in the register
	else 
        uNewStatus |= MADMASK_BIT13; //* Set the bit in the register

	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}



/****************************************************************************
*   						CSimulatorDlg::OnStatus_12
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_12()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_12.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewStatus &= ~MADMASK_BIT12; //* Clear the bit in the register
	else 
        uNewStatus |= MADMASK_BIT12; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_11
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_11()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT uStatus;
USHORT uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_11.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewStatus &= ~MADMASK_BIT11; //* Clear the bit in the register
	else 
        uNewStatus |= MADMASK_BIT11; //* Set the bit in the register

    UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_10
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_10()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_10.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewStatus &= ~MADMASK_BIT10; //* Clear the bit in the register
	else 
        uNewStatus |= MADMASK_BIT10; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_9
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_9()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_9.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewStatus &= ~MADMASK_BIT9; //* Clear the bit in the register
	else 
        uNewStatus |= MADMASK_BIT9; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_8
* Result: void
*   	
* Effect: This button is used as the input interrupt indicator
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_8()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_8.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewStatus &= ~MADMASK_BIT8; //* Clear the bit in the register
	else 
		uNewStatus |= MADMASK_BIT8; //* Set the bit in the register

    UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_7
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_7()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_7.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewStatus |= MADMASK_BIT7; //* Set the bit in the register
	else 
	    uNewStatus &= ~MADMASK_BIT7; //* Reset the bit in the register

	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_6
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_6()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_6.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewStatus |= MADMASK_BIT6; //* Set the bit in the register
	else 
	    uNewStatus &= ~MADMASK_BIT6; //* Reset the bit in the register

	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_5
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_5()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_5.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewStatus |= MADMASK_BIT5; //* Set the bit in the register
	else 
	    uNewStatus &= ~MADMASK_BIT5; //* Reset the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_4
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_4()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_4.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewStatus |= MADMASK_BIT4; //* Set the bit in the register
	else 
	    uNewStatus &= ~MADMASK_BIT4; //* Reset the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_3
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_3()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_3.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewStatus |= MADMASK_BIT3; //* Set the bit in the register
	else 
	    uNewStatus &= ~MADMASK_BIT3; //* Reset the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_2
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_2()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_2.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewStatus |= MADMASK_BIT2; //* Set the bit in the register
	else 
	    uNewStatus &= ~MADMASK_BIT2; //* Reset the bit in the register

	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_1
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_1()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_1.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewStatus |= MADMASK_BIT1; //* Set the bit in the register
	else 
	    uNewStatus &= ~MADMASK_BIT1; //* Reset the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnStatus_0
* Result: void
*   	
* Effect:  This button is used as the output interrupt indicator
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnStatus_0()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uStatus;
USHORT   uNewStatus;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uStatus    = (USHORT)pMadRegs->Status;   
	uNewStatus = uStatus;

	bSet = m_cbSTAT_0.GetCheck() == BST_CHECKED;

	if (!bSet)
		 uNewStatus &= ~MADMASK_BIT0; 
	else 
		uNewStatus |= MADMASK_BIT0; //* Set the Reg Bit

 	UpdateDlgCntls();
	LogStatusChange(uStatus, uNewStatus, _T("Manual"));

	return;
}



/****************************************************************************
*   						CSimulatorDlg::OnIntID_15
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_15()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_15.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewIntID &= ~MADMASK_BIT15; //* Clear the bit in the register
	else 
        uNewIntID |= MADMASK_BIT15; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_14
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_14()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_14.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewIntID &= ~MADMASK_BIT14; //* Clear the bit in the register
	else 
        uNewIntID |= MADMASK_BIT14; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_13
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_13()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
//UCHAR ucNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_13.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewIntID &= ~MADMASK_BIT13; //* Clear the bit in the register
	else 
        uNewIntID |= MADMASK_BIT13; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}



/****************************************************************************
*   						CSimulatorDlg::OnIntID_12
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_12()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
//UCHAR ucNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_12.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewIntID &= ~MADMASK_BIT12; //* Clear the bit in the register
	else 
        uNewIntID |= MADMASK_BIT12; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_11
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_11()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_11.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewIntID &= ~MADMASK_BIT11; //* Clear the bit in the register
	else 
        uNewIntID |= MADMASK_BIT11; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_10
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_10()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL     bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_10.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewIntID &= ~MADMASK_BIT10; //* Clear the bit in the register
	else 
        uNewIntID |= MADMASK_BIT10; //* Set the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

    return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_9
* Result: void
*   	
* Effect: Undefined 
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_9()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL     bSet;

	ASSERT(m_bManual);
	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_9.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewIntID &= ~MADMASK_BIT9; //* Clear the bit in the register
	else 
        uNewIntID |= MADMASK_BIT9; //* Set the bit in the register

    UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


//#ifdef BLOCK_MODE //*********************************************************

/****************************************************************************
*   						CSimulatorDlg::OnIntID_8
* Result: void
*   	
* Effect: This button is used as the input interrupt indicator
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_8()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
SHORT nLen;
BOOL bSet;
CString csInput;
char szInput[USRINT_BLOK_SIZE];

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_8.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewIntID &= ~MADMASK_BIT8; //* Clear the bit in the register
	else 
		{
		uNewIntID |= MADMASK_BIT8; //* Set the bit in the register
    	m_cbxData.GetWindowText(csInput);
		strcpy_s(szInput, USRINT_BLOK_SIZE,csInput.GetBuffer(1));
		//m_IoCtlObj.SetInput(m_pPioRead, (PUCHAR)szInput, USRINT_BLOK_SIZE);
        nLen = (short int)strlen(szInput);
		//m_IoCtlObj.SetReg16(MADREG_PIO_READ_LEN, nLen);
		}

    //ucNewIntID = (UCHAR)(uNewIntID >> 8);
	//m_IoCtlObj.SetReg16(MADREG_INT_ID, uNewIntID);

	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	//CloseRegisterTransaction(_T("OnIntID_8"));

	return;
}

#if 0 

/****************************************************************************
*   						CSimulatorDlg::OnIntID_8
* Result: void
*   	
* Effect: This button is used as the input interrupt indicator
*   	 
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_8()

{
USHORT uIntID;
USHORT uNewIntID;
//UCHAR ucNewIntID;
//USHORT uLen;
BOOL bSet;

	ASSERT(m_bManual);

	//OpenRegisterTransaction(_T("OnIntID_8"));

	//uIntID = m_IoCtlObj.GetReg16(MADREG_INT_ID); 
	uNewIntID = uIntID;

	bSet = m_cbINTID_8.GetCheck() == BST_CHECKED;
	if (!bSet) 
		uNewIntID &= ~MADMASK_BIT8; //* Clear the bit in the register
	else 
		{
		uNewIntID |= MADMASK_BIT8; //* Set the bit in the register

		}

    //ucNewIntID = (UCHAR)(uNewIntID >> 8);
	//m_IoCtlObj.SetReg16(MADREG_INT_ID, uNewIntID);

	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	//CloseRegisterTransaction(_T("OnIntID_8"));

	return;
}
#endif //*********************************************************

/****************************************************************************
*   						CSimulatorDlg::OnIntID_7
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_7()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_7.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewIntID |= MADMASK_BIT7; //* Set the bit in the register
	else 
	    uNewIntID &= ~MADMASK_BIT7; //* Reset the bit in the register

    UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_6
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_6()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);
	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_6.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewIntID |= MADMASK_BIT6; //* Set the bit in the register
	else 
	    uNewIntID &= ~MADMASK_BIT6; //* Reset the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_5
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_5()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_5.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewIntID |= MADMASK_BIT5; //* Set the bit in the register
	else 
	    uNewIntID &= ~MADMASK_BIT5; //* Reset the bit in the register

	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_4
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_4()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);
	
	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_4.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewIntID |= MADMASK_BIT4; //* Set the bit in the register
	else 
	    uNewIntID &= ~MADMASK_BIT4; //* Reset the bit in the register

	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

    return;
}

/****************************************************************************
*   						CSimulatorDlg::OnIntID_3
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_3()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_3.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewIntID |= MADMASK_BIT3; //* Set the bit in the register
	else 
	    uNewIntID &= ~MADMASK_BIT3; //* Reset the bit in the register

    UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_2
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_2()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT uIntID;
USHORT uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_2.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewIntID |= MADMASK_BIT2; //* Set the bit in the register
	else 
	    uNewIntID &= ~MADMASK_BIT2; //* Reset the bit in the register

	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_1
* Result: void
*   	
* Effect:  Undefined 
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_1()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_1.GetCheck() == BST_CHECKED;
	if (bSet)
		uNewIntID |= MADMASK_BIT1; //* Set the bit in the register
	else 
	    uNewIntID &= ~MADMASK_BIT1; //* Reset the bit in the register

 	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnIntID_0
* Result: void
*   	
* Effect:  This button is used as the output interrupt indicator
*   	 
*  
*	 
****************************************************************************/

void CSimulatorDlg::OnIntID_0()

{
static unsigned char szOutput[USRINT_BLOK_SIZE+1];
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
USHORT   uIntID;
USHORT   uNewIntID;
BOOL bSet, bRC = FALSE;
//char szInfoMsg[200];
int nOutLen;
//PUCHAR pucBufr;

	ASSERT(m_bManual);

	if (pMadRegs == NULL)
		return;

	uIntID     = (USHORT)pMadRegs->IntID;   
	uNewIntID  = uIntID;

	bSet = m_cbINTID_0.GetCheck() == BST_CHECKED;
	if (!bSet)
		 uNewIntID &= ~MADMASK_BIT0; 
	else 
		{  
		uNewIntID |= MADMASK_BIT0; //* Set the Reg Bit
		//bRC = m_IoCtlObj.GetOutput(szOutput, m_pPioWrite, USRINT_BLOK_SIZE); //* Retrieve output to the device
		if (bRC)
			{
            nOutLen = (int)strlen((PCHAR)szOutput);
			if (nOutLen > 160)
				szOutput[160] = 0x00;
			//sprintf_s(szInfoMsg, "Len = %d, Packet = %s", nOutLen, pucBufr);
			//MessageBox(szInfoMsg, NULL, MB_ICONINFORMATION);

			if (m_bDisplayOutput)
    			m_lbOutPackets.AddString((PCHAR)szOutput); //* Display the packet
			}
		}

	UpdateDlgCntls();
	LogStatusChange(uIntID, uNewIntID, _T("Manual"));

	return;
}




/****************************************************************************
*   						 CSimulatorDlg::OnInt
* Result: void
*   	
* Effect: 
*   	Forces an interrupt to be generated explicitly
*	Note that this generates a spurious interrupt to the driver
*	and the INT bit, DONE bit, etc. are NOT set
* Notes:
*	The simulator must be in Manual mode for this to be enabled
*	At least one of the input IE or output IE flags must be set
*   Design change:  Let's let this button Generate an interrupt
*   unconditionally 
****************************************************************************/

void CSimulatorDlg::OnInt()

{
BOOL bINT = TRUE;
USHORT MesgIdRegDlg = 0;
USHORT StatRegDlg;
USHORT IntIdRegDlg;
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
char     szInfoMsg[100] = "OnInt invoked!";

	ASSERT(m_bManual);

	StatRegDlg = Get_Status_DlgCntls();
	pMadRegs->Status = StatRegDlg;
	IntIdRegDlg = Get_IntID_DlgCntls();
	pMadRegs->IntID = IntIdRegDlg;
	
	//Update MesgID to the device last because it is the interrupt indicator/trigger
	//
    MesgIdRegDlg = m_ceMesgID.GetWindowInt(); //GetDlgMesgID();
	pMadRegs->MesgID = MesgIdRegDlg;

	return;
}


/****************************************************************************
*   					   CSimulatorDlg::OnGetIOState
* Result: void
*   	
* Effect: 
*   Reads the state of all the device registers.  Essentially, a manual poll
****************************************************************************/

void CSimulatorDlg::OnGetIOState()

{
    ASSERT(m_bManual); // must be in manual mode

    if (!m_bManual)
	   return;

    GetDeviceState();

    return;
}


/****************************************************************************
*   					   CSimulatorDlg::OnGetInputState
* Result: void
*   	
* Effect: 
*   	Reads the state of all the device input registers.  
****************************************************************************.

void CSimulatorDlg::OnGetInputState()

{
    ASSERT(m_bManual); // must be in manual mode
    if (!m_bManual)
	  return;

    GetInputState();

   return;
}
**************/


/****************************************************************************
*   					   CSimulatorDlg::OnGetOutputState
* Result: void
*   	
* Effect: 
*   	Reads the state of all the device input registers.  
****************************************************************************./

void CSimulatorDlg::OnGetOutputState()

{
    ASSERT(m_bManual); // must be in manual mode
    if (!m_bManual)
	   return;

    //GetOutputState();

    return;
}
*************/
