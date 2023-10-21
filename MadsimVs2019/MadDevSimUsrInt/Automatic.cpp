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
/*  Module  NAME : Automatic.cpp                                               */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s) supporting Automatic mode            */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <share.h>
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
//#include "buzzphrase.h"
#include "uwm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TIMER_SET_DONE_IN  1
#define TIMER_SET_DONE_OUT 2


/****************************************************************************
*   					   CSimulatorDlg::OnAutoInput
* Result: void
*   	
* Effect: 
*   	1) Releases the automatic polling thread.
*       2) Updates the display controls
****************************************************************************./

void CSimulatorDlg::OnAutoInput()

{
TraceItem* item;

	if (!m_bManual)  // already in some automatic mode
		{
		if (!m_bAutoInput)
			m_cbAutoInput.SetCheck(FALSE);

		return;
		}

	SetRunMode(eAutoInput); 
	m_eAutoInState = eInitIn;
	m_nPktsSent    = 0;
	m_ucCommandIn  = 0x00; //* Re-initialize the remembered state

	item = new TraceItem(TRACE_TYPE_ANNOTATION, TRUE,
	         		   	 _T(" Run Mode: Automatic Input"));
	m_wndTrace.AddString(item);

	//OpenRegisterTransaction("OnAutoInput");
	UpdateDlgCntls(); 
	//CloseRegisterTransaction("OnAutoInput");

	ResetDevice();
    
//#ifdef DMA_MDL_SG_DRVR
	//m_ulSGL_DX = 0;
    //GeneratePacket();
//#endif 

	::SetEvent(m_hFreeRun);

	return;
}


/****************************************************************************
*   					   CSimulatorDlg::OnAutoOutput
* Result: void
*   	
* Effect: 
*   	1) Releases the automatic polling thread.
*       2) Updates the display controls
****************************************************************************./

void CSimulatorDlg::OnAutoOutput()

{
static ULONG ulOpenFlags = (_O_CREAT      |
	        				_O_BINARY     |
			        		_O_TRUNC      |
							_O_SEQUENTIAL |
							_O_APPEND     | _O_WRONLY);
TraceItem* item;
errno_t errno;

	if (!m_bManual)  // already in some automatic mode
		{
		if (!m_bAutoOutput)
			m_cbAutoOutput.SetCheck(FALSE);

		return;
		}

	SetRunMode(eAutoOutput); 
	m_nPktsRecvd    = 0;

#if 0 //ndef BLOCK_MODE    
	memset(m_szOutPacket, 0x00, 250);
	m_nPktCount = 0; 
#endif
	
	m_ucCommandOut = 0x00; //* re-initialize the remembered state

	item = new TraceItem(TRACE_TYPE_ANNOTATION, TRUE,
	            		 _T("Run Mode: Automatic Output"));
	m_wndTrace.AddString(item);

	//OpenRegisterTransaction("OnAutoInput");
	UpdateDlgCntls(); 
	//CloseRegisterTransaction("OnAutoInput");

	ResetDevice();

    if (m_bAutoSave)
//#ifdef BLOCK_MODE
		errno  = _sopen_s(&m_hOutput, m_szOutFile, ulOpenFlags, _SH_DENYNO, _S_IWRITE);
#if 0 //#else
		m_pFile = fopen(m_szOutFile, "wb");
#endif

	::SetEvent(m_hFreeRun);

	return;
}
/* */


/****************************************************************************
*   					   CSimulatorDlg::OnAutoDuplex
* Result: void
*   	
* Effect: 
*   	1) Releases the automatic polling thread.
*       2) Updates the display controls
****************************************************************************/

void CSimulatorDlg::OnAutoDuplex()

{
static ULONG ulOpenFlags = (_O_CREAT      |
	        				_O_BINARY     |
			        		_O_TRUNC      |
							_O_SEQUENTIAL |
							_O_APPEND     | _O_WRONLY);
TraceItem* item;

	if (!m_bManual)  // already in some automatic mode
		{
		if (!m_bAutoDuplex)
			m_cbAutoDuplex.SetCheck(FALSE);

		return;
		}

	SetRunMode(eAutoDuplex); 
	m_eAutoInState  = eInitIn;
	m_nPktsSent     = 0;
	m_nPktsRecvd    = 0;

	item = new TraceItem(TRACE_TYPE_ANNOTATION, TRUE,
	            		 _T("Run Mode: Automatic Full Dux"));
	m_wndTrace.AddString(item);

	UpdateDlgCntls(); 
	ResetDevice();
    
    if (m_bAutoSave)
//#ifdef BLOCK_MODE
		errno  = _sopen_s(&m_hOutput, m_szOutFile, ulOpenFlags, _SH_DENYNO, _S_IWRITE);
#if 0 //#else
		m_pFile = fopen(m_szOutFile, "wb");
#endif

	::SetEvent(m_hFreeRun);

	return;
}


/****************************************************************************
*   						 CSimulatorDlg::OnPoll
* Inputs:
*   	WPARAM: ignored
*	LPARAM: ignored
* Result: LRESULT
*   	0, always
* Effect: 
*   	Polls for input and output state changes
* Notes:
*	Since this is done as an atomic operation in the context of the
*	main thread, it can be properly serialized with WM_TIMER messages
****************************************************************************/

LRESULT CSimulatorDlg::OnPoll(WPARAM, LPARAM)

{
static int count = 0;
BOOL bResult;

	count++;
	if (m_bTrcDetail)
		{
		/* trace it */
		TraceItem* e = new TraceItem(TRACE_TYPE_COMMENT, _T("=> OnPoll"));
		m_wndTrace.AddString(e);
		if (count > 1)
			{
			/* add error */
			e = new TraceItem(TRACE_TYPE_ERROR, _T("!!! count wrong"));
			m_wndTrace.AddString(e);
			} /* add error */
		} /* trace it */
	else
		{
		/* watch it */
		ASSERT(count < 2);
		} /* watch it */


	count--;

	bResult = Probability(m_cbXtraInts, m_ulProbSpurious);
    if (bResult)
		{
		LogInterrupt(_T("by Extraneous Interrupt"));
		((PMADREGS)m_pDeviceRegs)->MesgID = 1;
		}
	else
	    {
    	if ((m_eRunMode == eAutoInput)    ||
	    	(m_eRunMode == eAutoDuplex))
		    {
		    PollInput();
		    }

		if ((m_eRunMode == eAutoOutput)    ||
		    (m_eRunMode == eAutoDuplex))
		    {
		    PollOutput();
		    }
	    }

	if (m_bTrcDetail)
		{
		/* trace it */
		TraceItem* e = new TraceItem(TRACE_TYPE_COMMENT, _T("<= OnPoll"));
		m_wndTrace.AddString(e);
		} /* trace it */

	::SetEvent(m_hPause); // release polling thread

	return 0;
}


/****************************************************************************
*   					   CSimulatorDlg::PollInput
* Result: void
*   	
* Effect: 
*   	If the GO bit is set, initiate the actions that will implement
*	the transfer of a byte to the simulated device's input
****************************************************************************/

void CSimulatorDlg::PollInput()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
ULONG IntEnable =  pMadRegs->IntEnable;  
ULONG IntEnableWork = IntEnable;
ULONG ControlReg = pMadRegs->Control;   

	IntEnableWork &= MAD_INT_INPUT_MASK;
	if (IntEnableWork == 0)
		return;

    if (IntEnableWork & MAD_INT_DMA_INPUT_BIT)  
		{
        if (ControlReg & MAD_CONTROL_DMA_GO_BIT)
		    {
			InjectAnyErrs(pMadRegs);
            pMadRegs->IntID   |= MAD_INT_DMA_INPUT_BIT;
			pMadRegs->Control &= ~MAD_CONTROL_DMA_GO_BIT;
			LogInterrupt(_T("DMA input interrupt"));
			pMadRegs->MesgID = 1;
		    }
		} 

	if (IntEnableWork & MAD_INT_BUFRD_INPUT_BIT)  
		{
        if ((ControlReg & MAD_CONTROL_BUFRD_GO_BIT)    || 
			(ControlReg & MAD_CONTROL_CACHE_XFER_BIT))
		    {
			InjectAnyErrs(pMadRegs);
            pMadRegs->IntID   |= MAD_INT_BUFRD_INPUT_BIT;
			if (ControlReg & MAD_CONTROL_BUFRD_GO_BIT) 
			    {
				pMadRegs->Control &= ~MAD_CONTROL_BUFRD_GO_BIT;
				LogInterrupt(_T("Buffered input interrupt"));
			    }
			else
			    {
                pMadRegs->Control &= ~MAD_CONTROL_CACHE_XFER_BIT;
				LogInterrupt(_T("Cache xfer input interrupt"));
			    }

		    pMadRegs->MesgID = 1;
		    }
    	} 

	if (IntEnableWork & MAD_INT_ALIGN_INPUT_BIT)  
		{
        if (ControlReg & MAD_CONTROL_CACHE_XFER_BIT)
		    {
			InjectAnyErrs(pMadRegs);
            pMadRegs->IntID   |= MAD_INT_ALIGN_INPUT_BIT;
			pMadRegs->Control &= ~MAD_CONTROL_CACHE_XFER_BIT;
            LogInterrupt(_T("Cache align input interrupt"));
			pMadRegs->MesgID = 1;
    		}
	    }
			
	return;
}

/****************************************************************************
*   					   CSimulatorDlg::PollOutput
* Result: void
*   	
* Effect: 
*   	If the GO bit is set for the output side, accept the byte from the
*	device
****************************************************************************/

void CSimulatorDlg::PollOutput()

{
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
ULONG    IntEnable = pMadRegs->IntEnable;  
ULONG    IntEnableWork = IntEnable;
ULONG    ControlReg = pMadRegs->Control;   

	IntEnableWork &= MAD_INT_OUTPUT_MASK;
	if (IntEnableWork == 0)
		return;

    if (IntEnableWork & MAD_INT_DMA_OUTPUT_BIT)  
		{
        if (ControlReg & MAD_CONTROL_DMA_GO_BIT)
		    {
			InjectAnyErrs(pMadRegs);
            pMadRegs->IntID   |= MAD_INT_DMA_OUTPUT_BIT;
			pMadRegs->Control &= ~MAD_CONTROL_DMA_GO_BIT;
			LogInterrupt(_T("DMA output interrupt"));
			pMadRegs->MesgID = 1;
		    }
		} 

	if (IntEnableWork & MAD_INT_BUFRD_OUTPUT_BIT)  
		{
        if ((ControlReg & MAD_CONTROL_BUFRD_GO_BIT)    || 
			(ControlReg & MAD_CONTROL_CACHE_XFER_BIT))
		    {
			InjectAnyErrs(pMadRegs);
            pMadRegs->IntID   |= MAD_INT_BUFRD_OUTPUT_BIT;
			if (ControlReg & MAD_CONTROL_BUFRD_GO_BIT) 
			    {
				pMadRegs->Control &= ~MAD_CONTROL_BUFRD_GO_BIT;
				LogInterrupt(_T("Buffered output interrupt"));
			    }
			else
			    {
                pMadRegs->Control &= ~MAD_CONTROL_CACHE_XFER_BIT;
				LogInterrupt(_T("Cache xfer output interrupt"));
			    }

			pMadRegs->MesgID = 1;
		    }
    	} 

	if (IntEnableWork & MAD_INT_ALIGN_OUTPUT_BIT)  
		{
        if (ControlReg & MAD_CONTROL_CACHE_XFER_BIT)
		    {
			InjectAnyErrs(pMadRegs);
            pMadRegs->IntID   |= MAD_INT_ALIGN_OUTPUT_BIT;
			pMadRegs->Control &= ~MAD_CONTROL_CACHE_XFER_BIT;
            LogInterrupt(_T("Cache align output interrupt"));
			pMadRegs->MesgID = 1;
		    }
		} 

	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnPulse
* Inputs:
*   	WPARAM: ignored
*	LPARAM: ignored
* Result: LRESULT
*   	0, always
* Effect: 
*   	Indicates that a pulse has occurred in the polling operation
****************************************************************************/

LRESULT CSimulatorDlg::OnPulse(WPARAM, LPARAM)

{
	m_csbGoHackIn.Pulse();
	m_csbGoHackOut.Pulse();

	return 0;
}


/****************************************************************************
*   						CSimulatorDlg::OnTimer
* Inputs:
*   	UINT nIDEvent: Event id
* Result: void
*   	
* Effect: 
*   	Handles the timer
****************************************************************************/

void CSimulatorDlg::OnTimer(UINT nIDEvent)

{
//BOOL bRC;

	KillTimer(nIDEvent); // take only one such event
	switch (nIDEvent) 	 /* event type */
		{
		case TIMER_SET_DONE_IN:
				{
				/* done in */
				if (m_bTrcDetail)
					{
					/* note it */
					TraceItem* e = new TraceItem(TRACE_TYPE_ANNOTATION,
									        	 _T("=> Timer: In Done"));
					m_wndTrace.AddString(e);
					} /* note it */

				//OpenRegisterTransaction(_T("Timer Done In"));
				//bRC = SendNext(TRUE);
				//CloseRegisterTransaction(_T("Timer Done In"));
				m_csbGoHackIn.SetMode(TRUE); // indicate we are done waiting
				if (m_bTrcDetail)
					{
					/* note it */
					TraceItem* e = new TraceItem(TRACE_TYPE_ANNOTATION,
									        	 _T("<= Timer: In Done"));
					m_wndTrace.AddString(e);
					} /* note it */
				} /* done in */
			break;

		case TIMER_SET_DONE_OUT:
				{
				/* done out */
				if (m_bTrcDetail)
					{
					/* note it */
					TraceItem* e = new TraceItem(TRACE_TYPE_ANNOTATION,
									   	         _T("=> Timer: Out Done"));
					m_wndTrace.AddString(e);
					} /* note it */

				//OpenRegisterTransaction(_T("Timer Done Out"));
				//ReceiveNext();
				//CloseRegisterTransaction(_T("Timer Done Out"));
				m_csbGoHackOut.SetMode(TRUE);
				if (m_bTrcDetail)
					{
					/* note it */
					TraceItem* e = new TraceItem(TRACE_TYPE_ANNOTATION,
									   	         _T("<= Timer: Out Done"));
					m_wndTrace.AddString(e);
					} /* note it */
				} /* done out */
			break;
		} /* event type */

	CDialog::OnTimer(nIDEvent);

	return;
}

/****************************************************************************
*   					  CSimulatorDlg::GoDoneDelay
* Result: DWORD
*   	Random interval
*	At least GoDoneMinimum, and with as much as GoDoneVariance added
****************************************************************************/

DWORD CSimulatorDlg::GoDoneDelay()

{
DWORD interval = m_ulGoDoneMin;

	if (m_ulGoDoneVariance > 0)
		interval += rand() % m_ulGoDoneVariance;

	return interval;
}


/****************************************************************************
*			 CSimulatorDlg::OnSetTimerOut
* Inputs:
*   	WPARAM: ignored
*	LPARAM: ignored
* Result: LRESULT
*   	0, always
* Effect: 
*   	Sets the timer
* Notes:
*	This is done via a PostMessage so it will not take place until
*	the current transaction has completed
****************************************************************************/

LRESULT CSimulatorDlg::OnSetTimerOut(WPARAM, LPARAM)

{
DWORD interval = GoDoneDelay();

	if (m_bTrcDetail)
		{
		/* debug */
		CString s;
		s.Format(_T("Setting output timer %dms"), interval);
		TraceItem* e = new TraceItem(TRACE_TYPE_COMMENT, s);
		m_wndTrace.AddString(e);
		} /* debug */

	SetTimer(TIMER_SET_DONE_OUT, interval, NULL);

	return 0;
}


/****************************************************************************
*			 CSimulatorDlg::OnSetTimerIn
* Inputs:
*   	WPARAM: ignored
*	LPARAM: ignored
* Result: LRESULT
*   	0, always
* Effect: 
*   	Sets the timer
* Notes:
*	This is done via a PostMessage so it will not take place until
*	the current transaction has completed
****************************************************************************/

LRESULT CSimulatorDlg::OnSetTimerIn(WPARAM, LPARAM)

{
DWORD interval = GoDoneDelay();

	if (m_bTrcDetail)
		{
		/* debug */
		CString s;
		s.Format(_T("Setting input timer %dms"), interval);
		TraceItem* e = new TraceItem(TRACE_TYPE_COMMENT, s);
		m_wndTrace.AddString(e);
		} /* debug */

	SetTimer(TIMER_SET_DONE_IN, interval, NULL);

	return 0;
}


//******************
//*
void CSimulatorDlg::SetRunMode(etRunMode eRunMode)

{
	switch (eRunMode)
		{
		case eManual:
			m_bManual     = TRUE;
			//m_bAutoInput  = FALSE;
			//m_bAutoOutput = FALSE;
			m_bAutoDuplex= FALSE;
			m_cbManual.SetCheck(TRUE);
			//m_cbAutoInput.SetCheck(FALSE);
			//m_cbAutoOutput.SetCheck(FALSE);
			m_cbAutoDuplex.SetCheck(FALSE);
			break;

		/*
		case eAutoInput:
			m_bAutoInput = TRUE;
			m_bManual = FALSE;
			m_cbAutoInput.SetCheck(TRUE);
			m_cbManual.SetCheck(FALSE);
			break;

		case eAutoOutput:
			m_bAutoOutput = TRUE;
			m_bManual = FALSE;
			m_cbAutoOutput.SetCheck(TRUE);
			m_cbManual.SetCheck(FALSE);
			break; */

		case eAutoDuplex:
			m_bAutoDuplex = TRUE;
			m_bManual = FALSE;
			m_cbAutoDuplex.SetCheck(TRUE);
			m_cbManual.SetCheck(FALSE);
			break;

		default:;
		}; //* end switch

	m_eRunMode = eRunMode;

	return;
}

//*** Update a status register with a new error flag
//*** Cycle through the various status bits each time we're here
//*** skipping zero & one
//* 
void Set_Random_Err(PUSHORT puNewStat)

{
static short int nShiftVal = 3;
USHORT uTemp = 0x0001;

    uTemp = (USHORT)(uTemp << nShiftVal);
	*puNewStat = (USHORT)(*puNewStat | uTemp);

    nShiftVal++;
	nShiftVal = (short int)(nShiftVal % 16);
	if (nShiftVal == 0)
		nShiftVal = 3;

	return;
}

