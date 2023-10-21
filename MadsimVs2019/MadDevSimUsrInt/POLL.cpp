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
/*  Module  NAME : Poll.cpp                                                    */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "Automatic.h"
#include "MadDevSimUsrInt.h"
#include <sys\timeb.h>
#include "TraceWnd.h"
#include "RegVars.h"
#include "SpinnerButton.h"
#include "IoCtl_Cls.h"
#include "RegData.h"
#include "RegDisplay.h"
#include "NumericEdit.h"
#include "ErrorMngt.h"
#include "MadDevSimUIDlg.h"
#include "uwm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
*   						CSimulatorDlg::watcher
* Inputs:
*   	LPVOID me: (LPVOID)(CSimulatorDlg *)
* Result: UINT
*   	0, always
* Effect: 
*   	Top-level (static) thread function
****************************************************************************/

UINT CSimulatorDlg::Watcher(LPVOID me)

{
	((CSimulatorDlg *) me)->Poll();

	return 0;
}

/****************************************************************************
*   					  CSimulatorDlg::StartPollingCycle
* Result: void
*   	
* Effect: 
*   	Sets the interlock to start polling
****************************************************************************/

void CSimulatorDlg::StartPollingCycle()

{
	//OpenRegisterTransaction(_T("PollingCycle"));

	return;
}

/****************************************************************************
*			CSimulatorDlg::EndPollingCycle
* Result: void
*   	
* Effect: 
*   	Releases the spin lock
****************************************************************************/

void CSimulatorDlg::EndPollingCycle(BOOL bOutput)

{
	//CloseRegisterTransaction(_T("PollingCycle"), bOutput);

	return;
}



/****************************************************************************
*   						  CSimulatorDlg::Poll
* Result: void
*   	
* Effect: 
*   	Polls status
* Notes:
*	This is running as a separate thread, and thus CWnd-derived
*	objects have no meaning
****************************************************************************/

void CSimulatorDlg::Poll()

{
//TraceItem* pTrcItem;

	m_bPolling = TRUE;
	while (m_bPolling)
		{
		/* polling */
		WaitForSingleObject(m_hFreeRun, INFINITE); // don't freerun unless released
		::ResetEvent(m_hStop);

		//if (!m_bSnglStep)
			Sleep(m_ulPollIntrvl);

		if (!m_bPolling)
			break; // in case application was shut down while sleeping

		 //* Increment the Spinner button position each pulse
		::PostMessage(m_hMainWnd, UWM_PULSE, 0, 0); 

		::ResetEvent(m_hPause);

		::PostMessage(m_hMainWnd, UWM_POLL, 0, 0);
		::WaitForSingleObject(m_hPause, INFINITE);

		::SetEvent(m_hStop); // user may now stop

		switch (m_eRunMode)
			{
			case eAutoInput:
				PollAutoInput();
				break; 

			case eAutoOutput:
				PollAutoOutput();
				break;

			case eAutoDuplex:
				PollAutoInput();
				PollAutoOutput();
				break;

			case eManual:   //* BUG !*!
    		default:;   	//* BUG !*!   
			} //* end switch Run Mode

		} //* end while polling 

	return;
}

//* Run Auto-input fully automatically
//* Simulate a perfect operator with registered messages in sequence
//* as in a finite state machine
//*
void CSimulatorDlg::PollAutoInput()

{
//static ULONG nKonst = 99999;
static ULONG nCycleLmt = 40;
static ULONG nPulseCnt = 0;
static ULONG nXferInit = 0;
static ULONG nDlayInit = 0;
static ULONG nDlayLmt  = 25;
TraceItem* pTrcItem;
char szTrcText[100]; 
//UCHAR ucStatus;

	nPulseCnt++;

	//if (debug)
	if ((nPulseCnt % 100) == 0) //* Show a heartbeat in the Trace window
		{
		sprintf_s(szTrcText, 100, "Auto-Input: pulse count = %d", nPulseCnt);
		pTrcItem = new TraceItem(TRACE_TYPE_COMMENT, TRUE, _T(szTrcText));
		m_wndTrace.AddString(pTrcItem);
		}

    if ((nPulseCnt % 10) != 0) //Fall through to FSM activity every tenth time
		return;

//#ifdef BLOCK_MODE //* Skip the rest of this stuff
	::PostMessage(m_hMainWnd, UWM_AUTO_INPUT, 0, 0);
	return;
//#endif

//* Implement a finite state machine:
//* 1) Generate a new string,   2) Load the Xfer window (Send)
//* 3) Start the device input,  4) Wait for packet completion
//* 5) Issue an EOP indication, 6) Wait between packets
//*
	switch (m_eAutoInState)
		{
		case eInitIn:
			sprintf_s(szTrcText, 100, "Auto-Input: Initialize - pulse = %d", nPulseCnt);
    	    pTrcItem = new TraceItem(TRACE_TYPE_COMMENT, TRUE, _T(szTrcText));
			m_wndTrace.AddString(pTrcItem);

			m_eAutoInState = eGenerate;
			break;

		case eGenerate:
			sprintf_s(szTrcText, 100, "Auto-Input: Generate - pulse = %d", nPulseCnt);
		    pTrcItem = new TraceItem(TRACE_TYPE_COMMENT, TRUE, _T(szTrcText));
			m_wndTrace.AddString(pTrcItem);

			//* Simulate clicking the <Generate> button
			::PostMessage(m_hMainWnd, UWM_AUTO_IN_GENERATE, 0, 0);
			m_eAutoInState = eLoadSendWin;
			break;

		case eLoadSendWin:
			sprintf_s(szTrcText, 100, "Auto-Input: Send - pulse = %d", nPulseCnt);
		    pTrcItem = new TraceItem(TRACE_TYPE_COMMENT, TRUE, _T(szTrcText));
			m_wndTrace.AddString(pTrcItem);

			//* Simulate clicking the <Load> button
			::PostMessage(m_hMainWnd, UWM_AUTO_IN_SEND, 0, 0);
			m_eAutoInState = eDataXfer;
			break;

		case eDataXfer:
			sprintf_s(szTrcText, 100, "Auto-Input: Init Xfer - pulse = %d", nPulseCnt);
		    pTrcItem = new TraceItem(TRACE_TYPE_COMMENT, TRUE, _T(szTrcText));
			m_wndTrace.AddString(pTrcItem);

			//* Simulate clicking the <GoInHack> Spinner button
			::PostMessage(m_hMainWnd, /*UWM_AUTO_IN_GO*/UWM_GO_HACK_IN, 0, 0);
			nXferInit = nPulseCnt; //* Initialize the wait
			m_eAutoInState = eWait4Xfer;
			break;

		case eWait4Xfer:
			//* Wait a pre-determined limit 4 Xfer 2 complete 
			if (nPulseCnt >= (nXferInit + nCycleLmt)) //* Assume Xfer completed
				{
				m_nPktsSent++;
				sprintf_s(szTrcText, 100, "Auto-Input: packets sent = %d, - pulse = %d",
						m_nPktsSent, nPulseCnt);
				pTrcItem = new TraceItem(TRACE_TYPE_COMMENT, TRUE,
							   	         _T(szTrcText));
				m_wndTrace.AddString(pTrcItem);

				m_eAutoInState = ePacketEnd; //* allow exiting Auto-Input
				}
			break;

		case ePacketEnd:
			sprintf_s(szTrcText, 100, "Auto-Input: Delay ... pulse = %d", nPulseCnt);
			pTrcItem = new TraceItem(TRACE_TYPE_COMMENT, TRUE, _T(szTrcText));
			m_wndTrace.AddString(pTrcItem);

			nDlayInit = nPulseCnt;  	 //* Set up inter-packet delay ...
			m_eAutoInState = eDelayIn; 
			break;

		case eDelayIn:
			//* Wait 100 polls to allow user to exit Auto-Input 
			if (nPulseCnt >= (nDlayInit + nDlayLmt)) //*End inter-packet delay
				{
				sprintf_s(szTrcText, 100,
					    "Auto-Input: ... End Delay - pulse = %d", nPulseCnt);
    			pTrcItem = new TraceItem(TRACE_TYPE_COMMENT, TRUE,
					                     _T(szTrcText));
	    		m_wndTrace.AddString(pTrcItem);

				nPulseCnt = 0;
				m_eAutoInState = eInitIn; //* Let's do it all over again
				}
			break;

		default:
			m_eAutoInState = eInitIn; //* Security
		} //* end switch - finite state machine

	return;
}



//*
void CSimulatorDlg::PollAutoOutput()

{
static ULONG nKonst = 9999;
static ULONG nPulseCnt = 0;
static BOOL bInitXfer  = FALSE;
static BOOL bInitDelay = FALSE;
TraceItem* pTrcItem;
char szTrcText[100]; 

	nPulseCnt++;

	if ((nPulseCnt % 100) == 0) //* Show a heartbeat in the Trace window
		{
		sprintf_s(szTrcText, 100, "Auto-Output: pulse count = %d", nPulseCnt);
		pTrcItem = new TraceItem(TRACE_TYPE_ANNOTATION, TRUE, _T(szTrcText));
		m_wndTrace.AddString(pTrcItem);
		}

	if ((nPulseCnt % 10) != 0) 
        return;

//#ifdef BLOCK_MODE //* Skip the rest of this stuff
	::PostMessage(m_hMainWnd, UWM_AUTO_OUTPUT, 0, 0);
	return;
//#endif

	return;
}







