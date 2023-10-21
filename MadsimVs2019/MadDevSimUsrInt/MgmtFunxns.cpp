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
/*  Module  NAME : MgmtFunxns.cpp                                              */
/*                                                                             */
/*  DESCRIPTION  : Functions associated solely with window management or       */
/*                  process management                                         */     
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "Automatic.h"
#include "MadDevSimUsrInt.h"
#include <sys\timeb.h>
#include "TraceWnd.h"
#include "Regvars.h"
#include "NumericEdit.h"
#include "ErrorMngt.h"
#include "ErrMngtParms.h"
#include "SpinnerButton.h"
#include "IoCtl_Cls.h"
#include "RegData.h"
#include "RegDisplay.h"
#include "MadDevSimUIDlg.h"
//#include "buzzphrase.h"
#include "AutoExecDlg.h"
#include "About.h"
#include "uwm.h"
#include "format.h"
#include "..\Includes\GenDrivrTestVuParms.h" //* Defines for the Driver Test App.
#include "..\Includes\MadBusIoctls.h" 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CSimulatorDlg::OnInitDialog()

{
RECT WinRect;
LONG nHite, nWidth;
//LONG nSerialNo;
char szSerNum[7] = "0000\0";
char szWinTitl[100] = "MAD Simulation User Interface: Unit ";
BOOL bOpen = FALSE;
BOOL bRC   = FALSE;

	CDialog::OnInitDialog();

	//* Position this Device Simulator User Interface window next to to the
	//* just launched DriverTestApp.exe window - in the upper left corner
	//*
	CWnd::GetWindowRect(&WinRect);
	nHite = WinRect.bottom - WinRect.top;
	nWidth = WinRect.right - WinRect.left;
	CWnd::MoveWindow(CLIENTWIDTH + 10, //* GenDrivrTestVuParms.h
	0, nWidth, nHite, TRUE); //*Position,repaint,No resize

	time_t t;
	time(&t);
	srand((UINT)t);

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strAboutMenu;
#pragma warning(suppress: 6031)
	strAboutMenu.LoadString(IDS_ABOUTBOX);
	if (!strAboutMenu.IsEmpty())
		{
#ifdef _MAC
		// On the Macintosh, the "About..." menu item is already there.  We
		//  just need to rename it and attach it to the about command.
		pSysMenu->ModifyMenu(0, MF_BYPOSITION, IDM_ABOUTBOX, strAboutMenu);
#else
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
#endif
		}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//bRC = MessageBox(szWinTitl, szWinTitl, MB_ICONINFORMATION);
	// Open the simulator device
	bOpen = m_IoCtlObj.Open(&m_nSerialNo);
	if (!bOpen)
		{
		/* failed */
		PostMessage(UWM_OPEN_FAILED, 0, GetLastError()); // tell that it failed
		m_nSerialNo = 0;
		} /* failed */
 
    sprintf_s(m_szOutFile, 50, "DToutput%d.tst", m_nSerialNo);

	_itoa_s(m_nSerialNo, szSerNum, 5, 10);
	strcat_s(szWinTitl, 100, szSerNum);
    CWnd::SetWindowText(szWinTitl);

//*** in char-mode activate the character-advance windows
//*
#if 0 //#ifndef BLOCK_MODE 
    m_ceTransfer.EnableWindow(TRUE);
	m_cbSend.EnableWindow(TRUE);
#endif

	//if (bOpen)
        

	// Create an event for the manual/freerun state
	m_hFreeRun = ::CreateEvent(NULL,  // ignore security
	TRUE,  // manual-reset
	FALSE, // initially reset
	NULL); // Anonymous

//* ignore security, manual-reset, initially set, Anonymous
	m_hStop = ::CreateEvent(NULL, TRUE, TRUE, NULL);	

// ignore security,  manual-reset, initially reset, Anonymous
	m_hPause = ::CreateEvent(NULL, TRUE, FALSE, NULL);		

	m_cbManual.SetCheck(BST_CHECKED); // indicate manual run

	m_bManual = TRUE;
	//m_bSnglStep = FALSE;

	// Note that the freerun event *must* be created before the 
	// thread is started

	m_hMainWnd = m_hWnd; // for cross-thread sendMessage calls

	TimeStamp ts(TRUE);
	m_wndTrace.setTimeBase(ts.getMS());

	m_pPollThread = AfxBeginThread(Watcher, (LPVOID)this);

	if (m_pPollThread != NULL)
		{
		/* started */
		TraceItem* item = new TraceItem(TRACE_TYPE_THREAD,
							  	_T("Status Register Watcher started"));
		m_wndTrace.AddString(item);
		} /* started */

	RegistryInt regIRQ(IDS_IRQ);

	regIRQ.load(-1);
	m_ulIRQ = regIRQ.value;

	RegistryInt interval(IDS_POLLING_INTERVAL);
	interval.load(50);
	m_ulPollIntrvl = interval.value;

	RegistryInt minimum(IDS_GODONEMIN);
	minimum.load(m_ulGoDoneMin); // use current value as default
	m_ulGoDoneMin = minimum.value;

	RegistryInt variance(IDS_GODONEVAR);
	variance.load(m_ulGoDoneVariance); // use current value as default
	m_ulGoDoneVariance = variance.value;

	RegistryInt pErr(IDS_PROB_ERR);
	pErr.load(m_ulProbGenlErr);
	m_ulProbGenlErr = pErr.value;

	RegistryInt pOvrUnd(IDS_PROB_OVR_UND);
	pOvrUnd.load(m_ulProbOvrUnd);
	m_ulProbOvrUnd = pOvrUnd.value;

	RegistryInt pLost(IDS_PROB_LOST);
	pLost.load(m_ulProbLost);
	m_ulProbLost = pLost.value;

	RegistryInt pSpurious(IDS_PROB_SPURIOUS);
	pSpurious.load(m_ulProbSpurious);
	m_ulProbSpurious = pSpurious.value;

	UpdateDlgCntls();

	if (bOpen)
	    {
		bRC = m_IoCtlObj.MapWholeDevice(&m_pDeviceRegs, &m_pPioRead, &m_pPioWrite, &m_pDeviceData);
		if (bRC)
		    GetDeviceState();
	    }

	return TRUE;  // return TRUE  unless you set the focus to a control
}


/****************************************************************************
*   					  CSimulatorDlg::OnSysCommand
* Inputs:
*   	UINT nID: ID of command
*	LPARAM lParam: passed on to superclass
* Result: void
*   	
* Effect: 
*   	Handles system command messages (for About box etc.)
****************************************************************************/

void CSimulatorDlg::OnSysCommand(UINT nID, LPARAM lParam)

{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
		{
		doAbout();
		}
	else
		{
		CDialog::OnSysCommand(nID, lParam);
		}
}

/****************************************************************************
*   					   CSimulatorDlg::OnDestroy
* Result: void
*   	
* Effect: 
*   	Closes the open simulator device, closes down help
****************************************************************************/

void CSimulatorDlg::OnDestroy()

{
	m_IoCtlObj.Close(m_pDeviceRegs);

	WinHelp(0L, HELP_QUIT);

	CDialog::OnDestroy();
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSimulatorDlg::OnPaint()

{
	if (IsIconic())
		{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
		}
	else
		{
		CDialog::OnPaint();
		}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSimulatorDlg::OnQueryDragIcon()

{
	return (HCURSOR) m_hIcon;
}

void CSimulatorDlg::OnCancel()

{
	Shutdown();

	CDialog::OnCancel();
}


void CSimulatorDlg::OnOK()

{
	Shutdown();

	CDialog::OnOK();
}


#if 0 //#ifndef BLOCK_MODE
/****************************************************************************
*   					  CSimulatorDlg::SetTransfer
* Inputs:
*   	CString & s: string that will be the new input data string
* Result: void
*   	
* Effect: 
*   	Transfers the string to the input buffer and the display
****************************************************************************/

void CSimulatorDlg::SetTransfer(CString& s)

{
	TraceItem* item = new TraceItem(TRACE_TYPE_SEND, s);
	m_wndTrace.AddString(item);

	m_ceTransfer.SetWindowText(s);
	m_csInput = s;

	return;
}

//#endif //* Char-mode (BLOCK_MODE not defined ******************************

//#if 0 //#ifndef BLOCK_MODE //******************************************************

/****************************************************************************
*   						 CSimulatorDlg::OnSend
* Result: void
*   	
* Effect: 
*   	Initiates the transfer of the currently set string
* Notes:
*	In manual mode, this initiates the transfer of the next character of
*	the string, with automatic EOP handling
*	In freerun mode, simply initiates the process of sending
****************************************************************************/

void CSimulatorDlg::OnSend()

{
CString s;
BOOL bRC;

	m_cbxData.GetWindowText(s);
	int n = m_cbxData.FindStringExact(-1, s);
	if (n == CB_ERR)
		{
		/* add it */
		n = m_cbxData.AddString(s);
		m_cbxData.SetCurSel(n);
		} /* add it */
	else
		{
		/* already exists */
		m_cbxData.SetCurSel(n);
		} /* already exists */


	if (m_cbManual.GetCheck())
		{
		/* manual */
		// If this is the first time we've used the Send, we transfer the
		// data from the combo box to the input buffer.  
		// We determine this by checking to see if the input buffer is
		// empty
		if (m_ceTransfer.GetWindowTextLength() == 0)
			{
			/* first time */
			SetTransfer(s);
			} /* first time */
		else
			{
			//OpenRegisterTransaction(_T("OnSend"));
			bRC = SendNext(TRUE);
			//CloseRegisterTransaction(_T("OnSend"));
			}
		} /* manual */
	else
		{
		/* free run */
		SetTransfer(s);
		} /* free run */

	m_bErr = FALSE; // clear aborted-packet error flag

	UpdateDlgCntls(FALSE);

	return;
}
#endif //* char-mode (BLOCK_MODE not defined) ******************************


void CSimulatorDlg::OnSelendokData()

{
	UpdateDlgCntls(FALSE);
}

void CSimulatorDlg::OnClose()

{
	if (!m_bManual)
		OnManual(); // force into manual mode so shutdown works

	CDialog::OnClose();
}

void CSimulatorDlg::OnSize(UINT nType, int cx, int cy)

{
	CDialog::OnSize(nType, cx, cy);

	// Now resize the list box

	if (m_wndTrace.m_hWnd == NULL)
		return; // startup transient

	CRect lb;
	m_wndTrace.GetWindowRect(&lb);

	CRect dlg;
	GetWindowRect(&dlg);

	int border = lb.left - dlg.left;

	lb.right = dlg.right - border; 
	lb.bottom = dlg.bottom - border;

	if (lb.bottom <= lb.top)
		return; // negative size

	ScreenToClient(&lb);
	m_wndTrace.SetWindowPos(NULL, 0, 0, lb.Width(), lb.Height(),
				            (SWP_NOMOVE | SWP_NOZORDER));
}


//* Process the message to Generate a random buzzphrase
/*
void CSimulatorDlg::OnGenerate()

{
	GeneratePacket();

	return;
}


//* Generate a random buzzphrase and store it in the generate  window
//* 1st let's set it to all CAPs
//*
void CSimulatorDlg::GeneratePacket()

{
register int j;
BOOL bRandom = m_bGenRandomPkts | m_bManual;
CString csPhrase;
short int nLen;
TCHAR tChar;

    BuzzPhrase(csPhrase, bRandom);
    nLen = (short int)csPhrase.GetLength();

    for (j = 0; j < nLen; j++)
		{
		tChar = (TCHAR)csPhrase.GetAt(j);
		csPhrase.SetAt(j, (char)toupper(tChar)); 
		}

	m_cbxData.SetWindowText(csPhrase);	
	UpdateDlgCntls(FALSE);

	return;
}
/* */

void CSimulatorDlg::OnEditchangeData()

{
	UpdateDlgCntls(FALSE);
}


/****************************************************************************
*   						CSimulatorDlg::shutdown
* Result: void
*   	
* Effect: 
*   	Shuts down the polling thread
****************************************************************************/

void CSimulatorDlg::Shutdown()

{
	if (m_pPollThread != NULL)
		{
		/* shut down polling */
		// Note that because the pollthread object is destroyed when
		// the thread terminates, we have to save the actual handle
		// so we can wait on it (we could inhibit the object destruction
		// but choose to do it this way instead)
		HANDLE thread = m_pPollThread->m_hThread;
		m_bPolling = FALSE;
		::SetEvent(m_hFreeRun); // if the state was manual, force it to run
		::SetEvent(m_hPause);   // if it was paused, let it proceed
		WaitForSingleObject(thread, INFINITE);

		CloseHandle(m_hFreeRun);
		} /* shut down polling */

    return;
}


/****************************************************************************
*   					   CSimulatorDlg::GetDeviceState
* Result: void
*   	
* Effect: 
*   	Reads the state of all the device input registers.  
****************************************************************************/

void CSimulatorDlg::GetDeviceState()

{
USHORT uOldCmd;
USHORT uNewCmd;
USHORT uOldStat;
USHORT uNewStat;
USHORT uOldIntA;
USHORT uNewIntA;
USHORT uOldIntId;
USHORT uNewIntId;
USHORT uOldMesgID;
USHORT uNewMesgID;
PMADREGS pMadRegs = (PMADREGS)m_pDeviceRegs;
char szInfoMsg[100];

	sprintf_s(szInfoMsg, 100, "pMadRegs=%p", pMadRegs);
	//::MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

	if ((pMadRegs == NULL) || (pMadRegs == (PVOID)-1))
	    {
	    sprintf_s(szInfoMsg, 100, "pMadRegs=%p; devices not mapped!\nDevice state not available", pMadRegs);
		::MessageBox(NULL, szInfoMsg, NULL, MB_ICONEXCLAMATION);
		return;
	    }

	//----------------------------------------------------------------
	// MESG ID REGISTER
	//----------------------------------------------------------------
    uOldMesgID = m_ceMesgID.GetWindowInt(); //GetDlgMesgID();
	uNewMesgID = (USHORT)pMadRegs->MesgID; 
	m_ceMesgID.SetWindowText(uNewMesgID);
	LogChange(uOldMesgID, uNewMesgID, 0, TRACE_TYPE_BIT_X, _T("Get MesgID Reg"));

	//----------------------------------------------------------------
	// CONTOL REGISTER
	//----------------------------------------------------------------
	uOldCmd = Get_Control_DlgCntls();
    uNewCmd = (USHORT)pMadRegs->Control; //m_IoCtlObj.GetReg16(MADREG_CNTL); 
    ControlReg_2_DlgCntls(uNewCmd);
	LogControlChange(uOldCmd, uNewCmd, _T("Get Control Reg"));
	//::MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

	//----------------------------------------------------------------
	// STATUS REGISTER
	//----------------------------------------------------------------
	uOldStat = Get_Status_DlgCntls();
    uNewStat = (USHORT)pMadRegs->Status; //m_IoCtlObj.GetReg16(MADREG_STAT); 
    StatusReg_2_DlgCntls(uNewStat);
	LogStatusChange(uOldStat, uNewStat, _T("Get Status Reg"));

	//----------------------------------------------------------------
	// INT ACTV REGISTER
	//----------------------------------------------------------------
	uOldIntA = Get_IntEnable_DlgCntls();
    uNewIntA = (USHORT)pMadRegs->IntEnable; //m_IoCtlObj.GetReg16(MADREG_INT_ACTV); 
    IntEnableReg_2_DlgCntls(uNewIntA);
	LogIntEnableChange(uOldIntA, uNewIntA, _T("Get Int-Active Reg"));

	//----------------------------------------------------------------
	// INT ID REGISTER
	//----------------------------------------------------------------
	uOldIntId = Get_IntID_DlgCntls();
    uNewIntId = (USHORT)pMadRegs->IntID; //m_IoCtlObj.GetReg16(MADREG_INT_ID1);  
    IntIdReg_2_DlgCntls(uNewIntId);
	LogIntIdChange(uOldIntId, uNewIntId, _T("Get Int-ID Reg"));
	
	return;
}


//* Reset registers & data on the Input Side
//*
void CSimulatorDlg::ResetDevice()

{
//static BYTE ucZero = 0x00;
PMADBUS_MAP_WHOLE_DEVICE pMapWholeDevice = (PMADBUS_MAP_WHOLE_DEVICE)m_IoCtlObj.GetUsrIntDataPntr(); 
PMADREGS              pMadRegs           = (PMADREGS)m_pDeviceRegs; 
char szInfoMsg[100];

    ClearGenerate();

    ClearOutputWindow();

	sprintf_s(szInfoMsg, 100, "pMapWholeDevice=%p, pMadRegs=%p", pMapWholeDevice, pMadRegs);
	//::MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

    memset(pMadRegs, 0x00, sizeof(MADREGS));

//* Update the dialog box
//*
	ControlReg_2_DlgCntls(0x0000);
    StatusReg_2_DlgCntls(0x0000);
	IntEnableReg_2_DlgCntls(0x0000);
    IntIdReg_2_DlgCntls(0x0000);
 
    return;
}


//*** Set all device memory to high (FF) 
//*
void CSimulatorDlg::ResetPlayDead()

{
//static BYTE ucFF = 0xFF;
PMADBUS_MAP_WHOLE_DEVICE pMapWholeDevice = (PMADBUS_MAP_WHOLE_DEVICE)m_IoCtlObj.GetUsrIntDataPntr(); 
PMADREGS                 pMadRegs        = (PMADREGS)m_pDeviceRegs; 
char szInfoMsg[150];

	sprintf_s(szInfoMsg, 150, "pMapWholeDevice=%p, pMadRegs=%p, pMapWholeDevice->pDeviceRegs=%p",
		      pMapWholeDevice, pMadRegs, pMapWholeDevice->pDeviceRegs);
	//::MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

//* Update the dialog box
//*
	ControlReg_2_DlgCntls(0xFFFF);
    StatusReg_2_DlgCntls(0xFFFF);
	IntEnableReg_2_DlgCntls(0xFFFF);
    IntIdReg_2_DlgCntls(0xFFFF);

	//Causes an interrupt with all the device regs "bits-up"
 	memset(pMadRegs, 0xFF, sizeof(MADREGS));

    return;
}


/****************************************************************************
*   					CSimulatorDlg::OnUpdateDlgCntls
* Inputs:
*   	WPARAM: ignored
*	LPARAM: ignored
* Result: LRESULT
*   	void, 0 always
* Effect: 
*   	Updates the controls
****************************************************************************/

LRESULT CSimulatorDlg::OnUpdateDlgCntls(WPARAM, LPARAM)

{
	UpdateDlgCntls();

	return 0;
}


/****************************************************************************
*   				  CSimulatorDlg::InCmdReg_2_DlgCntls
* Inputs:
*   	BYTE command: Command register as read from device
* Result: void
*   	
* Effect: 
*   	Sets the state of the controls from the command byte
* Notes:
*	Optimizes update to reduce flicker
****************************************************************************/

void CSimulatorDlg::ControlReg_2_DlgCntls(USHORT ControlReg)

{
	Set_DlgCntl(m_cbCNTL_15, ControlReg, MADMASK_BIT15);
	Set_DlgCntl(m_cbCNTL_14, ControlReg, MADMASK_BIT14);
	Set_DlgCntl(m_cbCNTL_13, ControlReg, MADMASK_BIT13);
	Set_DlgCntl(m_cbCNTL_12, ControlReg, MADMASK_BIT12);
	Set_DlgCntl(m_cbCNTL_11, ControlReg, MADMASK_BIT11);
	Set_DlgCntl(m_cbCNTL_10, ControlReg, MADMASK_BIT10);
	Set_DlgCntl(m_cbCNTL_9,  ControlReg, MADMASK_BIT9);
	Set_DlgCntl(m_cbCNTL_8,  ControlReg, MADMASK_BIT8);
	Set_DlgCntl(m_cbCNTL_7,  ControlReg, MADMASK_BIT7);
	Set_DlgCntl(m_cbCNTL_6,  ControlReg, MADMASK_BIT6);
	Set_DlgCntl(m_cbCNTL_5,  ControlReg, MADMASK_BIT5);
	Set_DlgCntl(m_cbCNTL_4,  ControlReg, MADMASK_BIT4);
	Set_DlgCntl(m_cbCNTL_3,  ControlReg, MADMASK_BIT3);
	Set_DlgCntl(m_cbCNTL_2,  ControlReg, MADMASK_BIT2);
	Set_DlgCntl(m_cbCNTL_1,  ControlReg, MADMASK_BIT1);
	Set_DlgCntl(m_cbCNTL_0,  ControlReg, MADMASK_BIT0);

	UpdateDlgCntls(FALSE); // handle any induced dependencies

	return;
}


/****************************************************************************
*   				   CSimulatorDlg::StatusReg_2_DlgCntls
* Inputs:
*   	BYTE status: Status byte
* Result: void
*   	
* Effect: 
*   	Sets the state of the controls to the input status flags
* Notes:
*	Uses optimized update to reduce flicker
****************************************************************************/

void CSimulatorDlg::StatusReg_2_DlgCntls(USHORT uStatReg)

{
	Set_DlgCntl(m_cbSTAT_15, uStatReg, MADMASK_BIT15);
	Set_DlgCntl(m_cbSTAT_14, uStatReg, MADMASK_BIT14);
	Set_DlgCntl(m_cbSTAT_13, uStatReg, MADMASK_BIT13);
	Set_DlgCntl(m_cbSTAT_12, uStatReg, MADMASK_BIT12);
	Set_DlgCntl(m_cbSTAT_11, uStatReg, MADMASK_BIT11);
	Set_DlgCntl(m_cbSTAT_10, uStatReg, MADMASK_BIT10);
	Set_DlgCntl(m_cbSTAT_9,  uStatReg, MADMASK_BIT9);
	Set_DlgCntl(m_cbSTAT_8,  uStatReg, MADMASK_BIT8);
	Set_DlgCntl(m_cbSTAT_7,  uStatReg, MADMASK_BIT7);
	Set_DlgCntl(m_cbSTAT_6,  uStatReg, MADMASK_BIT6);
	Set_DlgCntl(m_cbSTAT_5,  uStatReg, MADMASK_BIT5);
	Set_DlgCntl(m_cbSTAT_4,  uStatReg, MADMASK_BIT4);
	Set_DlgCntl(m_cbSTAT_3,  uStatReg, MADMASK_BIT3);
	Set_DlgCntl(m_cbSTAT_2,  uStatReg, MADMASK_BIT2);
	Set_DlgCntl(m_cbSTAT_1,  uStatReg, MADMASK_BIT1);
	Set_DlgCntl(m_cbSTAT_0,  uStatReg, MADMASK_BIT0);

	UpdateDlgCntls(FALSE); // handle any induced dependencies

	return;
}


/****************************************************************************
*   				   CSimulatorDlg::IntEnableReg_2_DlgCntls
* Inputs:
*   	BYTE status: Status byte
* Result: void
*   	
* Effect: 
*   	Sets the state of the controls to the input status flags
* Notes:
*	Uses optimized update to reduce flicker
****************************************************************************/

void CSimulatorDlg::IntEnableReg_2_DlgCntls(USHORT uRegVal)

{
	Set_DlgCntl(m_cbINTEN_15, uRegVal, MADMASK_BIT15);
	Set_DlgCntl(m_cbINTEN_14, uRegVal, MADMASK_BIT14);
	Set_DlgCntl(m_cbINTEN_13, uRegVal, MADMASK_BIT13);
	Set_DlgCntl(m_cbINTEN_12, uRegVal, MADMASK_BIT12);
	Set_DlgCntl(m_cbINTEN_11, uRegVal, MADMASK_BIT11);
	Set_DlgCntl(m_cbINTEN_10, uRegVal, MADMASK_BIT10);
	Set_DlgCntl(m_cbINTEN_9,  uRegVal, MADMASK_BIT9);
	Set_DlgCntl(m_cbINTEN_8,  uRegVal, MADMASK_BIT8);
	Set_DlgCntl(m_cbINTEN_7,  uRegVal, MADMASK_BIT7);
	Set_DlgCntl(m_cbINTEN_6,  uRegVal, MADMASK_BIT6);
	Set_DlgCntl(m_cbINTEN_5,  uRegVal, MADMASK_BIT5);
	Set_DlgCntl(m_cbINTEN_4,  uRegVal, MADMASK_BIT4);
	Set_DlgCntl(m_cbINTEN_3,  uRegVal, MADMASK_BIT3);
	Set_DlgCntl(m_cbINTEN_2,  uRegVal, MADMASK_BIT2);
	Set_DlgCntl(m_cbINTEN_1,  uRegVal, MADMASK_BIT1);
	Set_DlgCntl(m_cbINTEN_0,  uRegVal, MADMASK_BIT0);

	UpdateDlgCntls(FALSE); // handle any induced dependencies

	return;
}


/****************************************************************************
*   				   CSimulatorDlg::IntIdReg_2_DlgCntls
* Inputs:
*   	BYTE status: Status byte
* Result: void
*   	
* Effect: 
*   	Sets the state of the controls to the input status flags
* Notes:
*	Uses optimized update to reduce flicker
****************************************************************************/

void CSimulatorDlg::IntIdReg_2_DlgCntls(USHORT uRegVal)

{
	Set_DlgCntl(m_cbINTID_15, uRegVal, MADMASK_BIT15);
	Set_DlgCntl(m_cbINTID_14, uRegVal, MADMASK_BIT14);
	Set_DlgCntl(m_cbINTID_13, uRegVal, MADMASK_BIT13);
	Set_DlgCntl(m_cbINTID_12, uRegVal, MADMASK_BIT12);
	Set_DlgCntl(m_cbINTID_11, uRegVal, MADMASK_BIT11);
	Set_DlgCntl(m_cbINTID_10, uRegVal, MADMASK_BIT10);
	Set_DlgCntl(m_cbINTID_9,  uRegVal, MADMASK_BIT9);
	Set_DlgCntl(m_cbINTID_8,  uRegVal, MADMASK_BIT8);
	Set_DlgCntl(m_cbINTID_7,  uRegVal, MADMASK_BIT7);
	Set_DlgCntl(m_cbINTID_6,  uRegVal, MADMASK_BIT6);
	Set_DlgCntl(m_cbINTID_5,  uRegVal, MADMASK_BIT5);
	Set_DlgCntl(m_cbINTID_4,  uRegVal, MADMASK_BIT4);
	Set_DlgCntl(m_cbINTID_3,  uRegVal, MADMASK_BIT3);
	Set_DlgCntl(m_cbINTID_2,  uRegVal, MADMASK_BIT2);
	Set_DlgCntl(m_cbINTID_1,  uRegVal, MADMASK_BIT1);
	Set_DlgCntl(m_cbINTID_0,  uRegVal, MADMASK_BIT0);

	UpdateDlgCntls(FALSE); // handle any induced dependencies

	return;
}



/****************************************************************************
*   				  CSimulatorDlg::Get_InCmd_DlgCntls
* Result: BYTE
*   	Command bits as seen in the controls
****************************************************************************/

USHORT CSimulatorDlg::Get_Control_DlgCntls()

{
USHORT uResult = 0;

	if (m_cbCNTL_15.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT15;

	if (m_cbCNTL_14.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT14;

	if (m_cbCNTL_13.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT13;

	if (m_cbCNTL_12.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT12;

	if (m_cbCNTL_11.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT11;

	if (m_cbCNTL_10.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT10;

	if (m_cbCNTL_9.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT9;

	if (m_cbCNTL_8.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT8;

	if (m_cbCNTL_7.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT7;

	if (m_cbCNTL_6.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT6;

	if (m_cbCNTL_5.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT5;

	if (m_cbCNTL_4.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT4;

	if (m_cbCNTL_3.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT3;

	if (m_cbCNTL_2.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT2;

	if (m_cbCNTL_1.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT1;

	if (m_cbCNTL_0.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT0;

	return uResult;
}


/****************************************************************************
*   				   CSimulatorDlg::Get_Status_DlgCntls
* Result: BYTE
*   	A status byte representing the current state in the controls
****************************************************************************/

USHORT CSimulatorDlg::Get_Status_DlgCntls()

{
USHORT uResult = 0;

	if (m_cbSTAT_15.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT15;

	if (m_cbSTAT_14.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT14;

	if (m_cbSTAT_13.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT13;

	if (m_cbSTAT_12.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT12;

	if (m_cbSTAT_11.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT11;

	if (m_cbSTAT_10.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT10;

	if (m_cbSTAT_9.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT9;

	if (m_cbSTAT_8.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT8;

	if (m_cbSTAT_7.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT7;

	if (m_cbSTAT_6.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT6;

	if (m_cbSTAT_5.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT5;

	if (m_cbSTAT_4.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT4;

	if (m_cbSTAT_3.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT3;

	if (m_cbSTAT_2.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT2;

	if (m_cbSTAT_1.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT1;

	if (m_cbSTAT_0.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT0;


	return uResult;
}


/****************************************************************************
*   				   CSimulatorDlg::Get_IntEnable_DlgCntls
* Result: BYTE
*   	A status byte representing the current state in the controls
****************************************************************************/

USHORT CSimulatorDlg::Get_IntEnable_DlgCntls()

{
USHORT uResult = 0;

	if (m_cbINTEN_15.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT15;

	if (m_cbINTEN_14.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT14;

	if (m_cbINTEN_13.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT13;

	if (m_cbINTEN_12.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT12;

	if (m_cbINTEN_11.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT11;

	if (m_cbINTEN_10.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT10;

	if (m_cbINTEN_9.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT9;

	if (m_cbINTEN_8.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT8;

	if (m_cbINTEN_7.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT7;

	if (m_cbINTEN_6.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT6;

	if (m_cbINTEN_5.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT5;

	if (m_cbINTEN_4.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT4;

	if (m_cbINTEN_3.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT3;

	if (m_cbINTEN_2.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT2;

	if (m_cbINTEN_1.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT1;

	if (m_cbINTEN_0.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT0;

	return uResult;
}


/****************************************************************************
*   				   CSimulatorDlg::Get_IntID_DlgCntls
* Result: BYTE
*   	A status byte representing the current state in the controls
****************************************************************************/

USHORT CSimulatorDlg::Get_IntID_DlgCntls()

{
USHORT uResult = 0;

	if (m_cbINTID_15.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT15;

	if (m_cbINTID_14.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT14;

	if (m_cbINTID_13.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT13;

	if (m_cbINTID_12.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT12;

	if (m_cbINTID_11.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT11;

	if (m_cbINTID_10.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT10;

	if (m_cbINTID_9.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT9;

	if (m_cbINTID_8.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT8;

	if (m_cbINTID_7.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT7;

	if (m_cbINTID_6.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT6;

	if (m_cbINTID_5.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT5;

	if (m_cbINTID_4.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT4;

	if (m_cbINTID_3.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT3;

	if (m_cbINTID_2.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT2;

	if (m_cbINTID_1.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT1;

	if (m_cbINTID_0.GetCheck() == BST_CHECKED)
		uResult |= MADMASK_BIT0;

	return uResult;
}


/****************************************************************************
*   					 CSimulatorDlg::LogErrorChange
* Inputs:
*   	CButton & ctl: Control that has changed
* Result: void
*   	
* Effect: 
*   	Logs the error injection control state change
****************************************************************************/

void CSimulatorDlg::LogErrorChange(CButton& ctl)

{
CString s;

	ctl.GetWindowText(s);
	TraceItem* item = new TraceItem(TRACE_TYPE_ANNOTATION, ctl.GetCheck(), s);
	m_wndTrace.AddString(item);

	return;
}


//********
void CSimulatorDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex,
	BOOL bSysMenu)

{
	CDialog::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	CMenu* menu = GetMenu();

	menu->EnableMenuItem(ID_EDIT_CLEAR,
		                 m_wndTrace.GetCount() > 0 ? MF_ENABLED : MF_GRAYED);

	menu->EnableMenuItem(ID_FILE_SAVE,
		  	             m_wndTrace.GetCount() > 0 &&
		  	             m_wndTrace.GetModified() ? MF_ENABLED : MF_GRAYED);

	menu->EnableMenuItem(ID_FILE_SAVE_AS,
		  	             m_wndTrace.GetCount() > 0 ? MF_ENABLED : MF_GRAYED);

	menu->CheckMenuItem(ID_FILE_AUTOSAVE,
		                m_bAutoSave ? MF_CHECKED : MF_UNCHECKED);

	menu->CheckMenuItem(ID_VIEW_INTERNALTRACE,
		  	            m_bDisplayTrace ? MF_CHECKED : MF_UNCHECKED);

	menu->CheckMenuItem(ID_VIEW_DETAILEDINTERNALTRACE,
		  	            m_bTrcDetail ? MF_CHECKED : MF_UNCHECKED);

	menu->CheckMenuItem(ID_VIEW_OUTPUTDISPLAY,
		  	            m_bDisplayOutput ? MF_CHECKED : MF_UNCHECKED);

	//menu->CheckMenuItem(ID_VIEW_BINARYDATA,
	//	              	m_bBinaryData ? MF_CHECKED : MF_UNCHECKED);

	menu->EnableMenuItem(IDM_SETUP_IRQ, m_bManual ? MF_ENABLED:MF_GRAYED);
}

/****************************************************************************
*   					  CSimulatorDlg::OnOpenFailed
* Inputs:
*   	WPARAM: ignored
*	LPARAM lParam: GetLastError()
* Result: LRESULT
*   	0, always
* Effect: 
*   	Notifies user that the device open failed
****************************************************************************/

LRESULT CSimulatorDlg::OnOpenFailed(WPARAM, LPARAM lParam)

{
CString s;

#pragma warning(suppress: 6031)
	s.LoadString(IDS_OPEN_FAILED);
	s += _T("\r\n");
	s += formatError((DWORD)lParam);
	AfxMessageBox(s, MB_ICONERROR | MB_OK);

	return 0;
}

/****************************************************************************
*   						 CSimulatorDlg::DoTraceSave
* Inputs:
*   	BOOL mode: TRUE to Save, FALSE to SaveAs
* Result: void
*   	
* Effect: 
*   	Does a save
****************************************************************************/

void CSimulatorDlg::DoTraceSave(BOOL mode)

{
BOOL prompt = FALSE;
CString csName;

	if (!mode) /* SaveAs */
		prompt = TRUE;
	else/* Save */
		if (m_csSaveFileName.GetLength() == 0) /* no file name set */
		    prompt = TRUE;

	if (prompt) /* already has name */
		csName = m_csSaveFileName;
	else
		{
		/* get file name */
		CString filter;
#pragma warning(suppress: 6031)
		filter.LoadString(IDS_FILTER);
		CFileDialog dlg(FALSE,
			_T(".trc"),
			NULL,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			filter);

		CString s;
#pragma warning(suppress: 6031)
		s.LoadString(mode ? IDS_FILE_SAVE : IDS_FILE_SAVE_AS);
		dlg.m_ofn.lpstrTitle = s;

		switch (dlg.DoModal())
			{
				/* domodal */
			case 0:
				// should never happen
			case IDCANCEL:
				return;
			} /* domodal */

		// We get here, it must be IDOK
		csName = dlg.GetPathName();
		} /* get file name */

	TRY
		{
		CStdioFile f(csName,
		             CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive);

		for (int i = 0; i < m_wndTrace.GetCount(); i++)
			{
			/* write each */
			TraceItem* e = (TraceItem*) m_wndTrace.GetItemDataPtr(i);
			f.WriteString(e->display(m_wndTrace.getTimeBase()));
			} /* write each */

		f.Close();

		m_csSaveFileName = csName;

		m_wndTrace.SetModified(FALSE);
		}
	CATCH(CFileException, e)
		{
		/* error */
		AfxMessageBox(IDS_FILE_OPEN_FAILED, MB_ICONERROR | MB_OK);
		// TODO: Make this report a meaningful error (it's 5am and I want
		// to finish this before I leave for a trip later today!)
		return; // return without doing anything further
		} /* error */
	END_CATCH
}


/****************************************************************************
*   					  CSimulatorDlg::OnRegClose
* Inputs:
*   	WPARAM: ignored
*	LPARAM: ignored
* Result: LRESULT
*   	0, always
* Effect: 
*   	The register display has closed
****************************************************************************/

LRESULT CSimulatorDlg::OnRegClose(WPARAM, LPARAM)

{
	m_pRegDisp = NULL;

	return 0;
}


#if 0 //#ifndef BLOCK_MODE //*******************************************************

/****************************************************************************
*   					  CSimulatorDlg::ClearInput
* Result: void
*   	
* Effect: 
*   	Clears the input buffer.  This will force an ERR condition if there
*	is an attempt to read the rest of the packet before a new one is
*	established
****************************************************************************/

void CSimulatorDlg::ClearInput()

{
    m_csInput = _T("");
    m_ceTransfer.SetWindowText(_T(""));
    m_bErr = TRUE;

    UpdateDlgCntls(FALSE);

    return;
}
#endif //* char-mode (BLOCK_MODE not defined *******************************


/****************************************************************************
*   					  CSimulatorDlg::ClearGenerate
* Result: void
*   	
* Effect: 
*   Clears the window of generated data. This has no other effect

****************************************************************************/

void CSimulatorDlg::ClearGenerate()

{
	m_cbxData.SetWindowText(_T(""));
	UpdateDlgCntls(FALSE);

	return;
}


//*** Clear the device output window
//*
void CSimulatorDlg::ClearOutputWindow()

{
	m_lbOutPackets.ResetContent();

	return;
}


//* Clear the trace window
//*
void CSimulatorDlg::ClearTrace()

{
	m_wndTrace.ResetContent();	
	UpdateDlgCntls(FALSE);

	return;
}


/****************************************************************************
*   					  CSimulatorDlg::Set_DlgCntl
* Inputs:
*   	CButton & ctl: Control to be set
*	BYTE value: Value to be used to set it (as a bit mask)
*	BYTE mask:  Mask used to determine the selected bit
* Result: void
*   	
* Effect: 
*   	Sets the check box.  If the check box is already in the desired
*	state, does not set it (reduces flicker)
****************************************************************************/

void CSimulatorDlg::Set_DlgCntl(CButton& ctl, USHORT uValue, USHORT uMask)

{
BOOL bDesired = (uValue & uMask) != 0;
BOOL bCntlVal = ctl.GetCheck() == BST_CHECKED;

	if (bDesired ^ bCntlVal) //* changed 
		ctl.SetCheck(bDesired ? BST_CHECKED : BST_UNCHECKED);

    return;
}


/****************************************************************************
*   					 CSimulatorDlg::UpdateDlgCntls
* Inputs:
*	BOOL valid: TRUE (default) if register set is valid
*			FALSE if register set is not valid
* Result: void
*   	
* Effect: 
*   	Updates all controls
****************************************************************************/

void CSimulatorDlg::UpdateDlgCntls(BOOL valid /* = TRUE */)

{
BOOL bIsManual = m_cbManual.GetCheck()       == BST_CHECKED;
BOOL bAutoIn   = /*m_cbBurstInput.GetCheck()   == BST_CHECKED  || */ 
                 m_cbAutoInput.GetCheck()    == BST_CHECKED  ||
                 m_cbAutoDuplex.GetCheck()  == BST_CHECKED;  
BOOL bAutoOut  = m_cbAutoOutput.GetCheck()   == BST_CHECKED  ||  
                 m_cbAutoDuplex.GetCheck()  == BST_CHECKED;  
BOOL bValid = valid; //* To please the compiler

#if 0 //#ifndef BLOCK_MODE
    BOOL bEnable; 
#endif

BOOL bAutoRun;
BOOL bDataWinLdd;

#if 0 //#ifndef BLOCK_MODE
    BOOL bXferWinLdd;
#endif

	//m_cbSingleStep.EnableWindow(bIsManual);
	m_csbGoHackIn.ShowWindow(bAutoIn   ? SW_SHOW : SW_HIDE);
	m_csbGoHackOut.ShowWindow(bAutoOut ? SW_SHOW : SW_HIDE);

	//----------------------------------------------------------------------
	// The Send button is enabled if
	//		We are in freerun mode, the data string is nonempty, and
	//		the data buffer is empty
	//		We are in manual mode, and 
	//			the data string is nonempty and the data buffer is empty
	//			the data buffer is nonempty

	bAutoRun = m_eRunMode >= eAutoInput; 
	bDataWinLdd = m_cbxData.GetWindowTextLength() > 0;

//* Error Injection
//*
	m_cbXtraInts.EnableWindow(bIsManual);
	m_cbGenlErrs.EnableWindow(bIsManual);
	m_cbOvrUndrErrs.EnableWindow(bIsManual);
	m_cbDevBusyErrs.EnableWindow(bIsManual);
	m_cbError.EnableWindow(bIsManual);

	//----------------------------------------------------------------------
	// REGISTER BIT CHECK BOX CONTROLS
	//----------------------------------------------------------------------

	//m_cbCNTL_15.EnableWindow(bIsManual);
	//m_cbCNTL_14.EnableWindow(bIsManual);
	//m_cbCNTL_13.EnableWindow(bIsManual);
	//m_cbCNTL_12.EnableWindow(bIsManual);
	//m_cbCNTL_11.EnableWindow(bIsManual);
	//m_cbCNTL_10.EnableWindow(bIsManual);
	//m_cbCNTL_9.EnableWindow(bIsManual);
	//m_cbCNTL_8.EnableWindow(bIsManual);
	//m_cbCNTL_7.EnableWindow(bIsManual);
	//m_cbCNTL_6.EnableWindow(bIsManual);
	//m_cbCNTL_5.EnableWindow(bIsManual);
	//m_cbCNTL_4.EnableWindow(bIsManual);
	//m_cbCNTL_3.EnableWindow(bIsManual);
	//m_cbCNTL_2.EnableWindow(bIsManual);
	//m_cbCNTL_1.EnableWindow(bIsManual);
	//m_cbCNTL_0.EnableWindow(bIsManual);

	m_cbSTAT_15.EnableWindow(bIsManual);
	m_cbSTAT_14.EnableWindow(bIsManual);
	m_cbSTAT_13.EnableWindow(bIsManual);
	m_cbSTAT_12.EnableWindow(bIsManual);
	m_cbSTAT_11.EnableWindow(bIsManual);
	m_cbSTAT_10.EnableWindow(bIsManual);
	m_cbSTAT_9.EnableWindow(bIsManual);
	m_cbSTAT_8.EnableWindow(bIsManual);
	m_cbSTAT_7.EnableWindow(bIsManual);
	m_cbSTAT_6.EnableWindow(bIsManual);
	m_cbSTAT_5.EnableWindow(bIsManual);
	m_cbSTAT_4.EnableWindow(bIsManual);
	m_cbSTAT_3.EnableWindow(bIsManual);
	m_cbSTAT_2.EnableWindow(bIsManual);
	m_cbSTAT_1.EnableWindow(bIsManual);
	m_cbSTAT_0.EnableWindow(bIsManual);

	//m_cbINTEN_15.EnableWindow(bIsManual);
	//m_cbINTEN_14.EnableWindow(bIsManual);
	//m_cbINTEN_13.EnableWindow(bIsManual);
	//m_cbINTEN_12.EnableWindow(bIsManual);
	//m_cbINTEN_11.EnableWindow(bIsManual);
	//m_cbINTEN_10.EnableWindow(bIsManual);
	//m_cbINTEN_9.EnableWindow(bIsManual);
	//m_cbINTEN_8.EnableWindow(bIsManual);
	//m_cbINTEN_7.EnableWindow(bIsManual);
	//m_cbINTEN_6.EnableWindow(bIsManual);
	//m_cbINTEN_5.EnableWindow(bIsManual);
	//m_cbINTEN_4.EnableWindow(bIsManual);
	//m_cbINTEN_3.EnableWindow(bIsManual);
	//m_cbINTEN_2.EnableWindow(bIsManual);
	//m_cbINTEN_1.EnableWindow(bIsManual);
	//m_cbINTEN_0.EnableWindow(bIsManual);

	m_cbINTID_15.EnableWindow(bIsManual);
	m_cbINTID_14.EnableWindow(bIsManual);
	m_cbINTID_13.EnableWindow(bIsManual);
	m_cbINTID_12.EnableWindow(bIsManual);
	m_cbINTID_11.EnableWindow(bIsManual);
	m_cbINTID_10.EnableWindow(bIsManual);
	m_cbINTID_9.EnableWindow(bIsManual);
	m_cbINTID_8.EnableWindow(bIsManual);
	m_cbINTID_7.EnableWindow(bIsManual);
	m_cbINTID_6.EnableWindow(bIsManual);
	m_cbINTID_5.EnableWindow(bIsManual);
	m_cbINTID_4.EnableWindow(bIsManual);
	m_cbINTID_3.EnableWindow(bIsManual);
	m_cbINTID_2.EnableWindow(bIsManual);
	m_cbINTID_1.EnableWindow(bIsManual);
	m_cbINTID_0.EnableWindow(bIsManual);

	m_cbGetState.EnableWindow(bIsManual); 

	return;
}


//#ifdef BLOCK_MODE ///////////////////////////////////////////
//*** Block IO block preparation functions
//*
void CSimulatorDlg::Set_Block_Input(char szData[])

{
static unsigned char szInput[USRINT_BLOK_SIZE];

    memset(szInput, (unsigned char)0x81, USRINT_BLOK_SIZE);
	strcpy_s((char *)szInput, USRINT_BLOK_SIZE, szData);
	//m_IoCtlObj.SetInput(m_pPioRead, szInput, USRINT_BLOK_SIZE);

    return;
}


PUCHAR CSimulatorDlg::Get_Block_Output(void)

{
static unsigned char szOutput[USRINT_BLOK_SIZE];
BOOL bRC = TRUE;
PUCHAR puChar = (unsigned char *)&szOutput[0];

    //memset(szOutput, (unsigned char)0x81, MAD_BLOK_SIZE);
	//bRC = m_IoCtlObj.GetOutput(szOutput, m_pPioWrite, USRINT_BLOK_SIZE);
	if (!bRC)
		return NULL;

	return puChar;
}
//#endif //* BLOCK_MODE ////////////////////////////////////////
