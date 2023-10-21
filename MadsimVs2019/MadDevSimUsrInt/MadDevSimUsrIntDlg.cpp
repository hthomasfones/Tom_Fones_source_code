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
/*  DESCRIPTION  : Function definition(s) supporting the main dialog box       */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "Automatic.h"
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
#include "PacketTerm.h"
#include "MadDevSimUIDlg.h"
#include "AutoExecDlg.h"
#include "About.h"
#include "comment.h"
#include "HdwTrace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAD_DATAFILE_SUFFIX  ".dat"


/****************************************************************************
*   							   UWM_PULSE
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: LRESULT
*   	Logically void, 0, always
* Effect: 
*   	Indicates a single polling operation has started
****************************************************************************/

UINT UWM_PULSE = ::RegisterWindowMessage(_T("UWM_PULSE-{71EDE612-AC36-11d1-8369-00AA005C0507}"));



/****************************************************************************
*   							   UWM_POLL
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: LRESULT
*   	Logically void, 0, always
* Effect: 
*   	Starts the polling operation in the main thread
****************************************************************************/

UINT UWM_POLL = ::RegisterWindowMessage(_T("UWM_POLL-{71EDE612-AC36-11d1-8369-00AA005C0507}"));



/****************************************************************************
*   			   UWM_SET_TIMER_OUT
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: LRESULT
*   	Logically void, 0, always
* Effect: 
*   	Starts a Go->Done output timer
****************************************************************************/

UINT UWM_SET_TIMER_OUT = ::RegisterWindowMessage(_T("UWM_SET_TIMER_OUT-{71EDE612-AC36-11d1-8369-00AA005C0507}"));



/****************************************************************************
*   			   UWM_SET_TIMER_IN
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: LRESULT
*   	Logically void, 0, always
* Effect: 
*   	Starts a GO->Done input timer
****************************************************************************/

UINT UWM_SET_TIMER_IN = ::RegisterWindowMessage(_T("UWM_SET_TIMER_IN-{71EDE612-AC36-11d1-8369-00AA005C0507}"));



/****************************************************************************
*   			   UWM_SET_MANUAL_MODE
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: LRESULT
*   	Logically void, 0, always
* Effect: 
*   	Deferred Manual Mode request (handles queueing deadlock problem)
****************************************************************************/

UINT UWM_SET_MANUAL_MODE = ::RegisterWindowMessage(_T("UWM_SET_MANUAL_MODE-{71EDE612-AC36-11d1-8369-00AA005C0507}"));



/****************************************************************************
*   						 UWM_OPEN_FAILED
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: LRESULT
*   	0, always (logically void)
* Effect: 
*   	Handles notification that the open of the device failed
****************************************************************************/

UINT UWM_OPEN_FAILED = ::RegisterWindowMessage(_T("UWM_OPEN_FAILED-{15B50870-B191-11d1-836A-00AA005C0507}"));


/****************************************************************************
*   						 UWM_IMGR_CLOSE
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: LRESULT
*   	0, always (logically void)
* Effect: 
*   	Handles notification that the interrupt/error manager has closed
****************************************************************************/

UINT UWM_IMGR_CLOSE = ::RegisterWindowMessage(_T("UWM_IMGR_CLOSE-{15B50870-B191-11d1-836A-00AA005C0507}"));


/****************************************************************************
*   						 UWM_REG_CLOSE
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: LRESULT
*   	0, always (logically void)
* Effect: 
*   	Handles notification that the register display has closed
****************************************************************************/

UINT UWM_REG_CLOSE = ::RegisterWindowMessage(_T("UWM_REG_CLOSE-{15B50870-B191-11d1-836A-00AA005C0507}"));


/****************************************************************************
*   						 UWM_UPDATE_REGS
* Inputs:
*   	WPARAM: ignored
*   LPARAM: (LPARAM)(HDW_SIM_REGS)
* Result: LRESULT
*   	0, always (logically void)
* Effect: 
*   	Handles notification that the register display has closed
****************************************************************************/

UINT UWM_UPDATE_REGS = ::RegisterWindowMessage(_T("UWM_UPDATE_REGS-{15B50870-B191-11d1-836A-00AA005C0507}"));


/****************************************************************************
*   							  UWM_GO_HACK_IN
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	LRESULT 0
* Effect: 
*   	The "Go" output bit has been set; initiate the action sequence required
*   to complete the GO action
****************************************************************************/

UINT UWM_GO_HACK_IN = ::RegisterWindowMessage(_T("UWM_GO_HACK_IN-{71EDE610-AC36-11d1-8369-00AA005C0507}"));


/****************************************************************************
*   							  UWM_GO_HACK_OUT
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	LRESULT 0
* Effect: 
*   	The "Go" output bit has been set; initiate the action sequence required
*   to complete the GO action
****************************************************************************/

UINT UWM_GO_HACK_OUT = ::RegisterWindowMessage(_T("UWM_GO_HACK_OUT-{71EDE610-AC36-11d1-8369-00AA005C0507}"));

/****************************************************************************
*   			 UWM_GEN_RANDOM_PKTS
* Inputs:
*   	WPARAM: code which indicates what probability variable is being updated
*   LPARAM: value which is to be set
* Result: void
*   	LRESULT 0
* Notes: 
*   	Sent from the InterruptMgt modeless dialog to its parent
****************************************************************************/

//UINT UWM_GEN_RANDOM_PKTS = ::RegisterWindowMessage(_T("UWM_GEN_RANDOM_PKTS-{71EDE610-AC36-11d1-8369-00AA005C0507}"));


/****************************************************************************
*   			 UWM_SET_PROB
* Inputs:
*   	WPARAM: code which indicates what probability variable is being updated
*   LPARAM: value which is to be set
* Result: void
*   	LRESULT 0
* Notes: 
*   	Sent from the InterruptMgt modeless dialog to its parent
****************************************************************************/

UINT UWM_SET_PROB = ::RegisterWindowMessage(_T("UWM_SET_PROB-{71EDE610-AC36-11d1-8369-00AA005C0507}"));

// The Messages below are nominally obsolete (wait for next set of tests)

/****************************************************************************
*   							  UWM_GO_OUT_SET
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	LRESULT 0
* Effect: 
*   	The "Go" output bit has been set; initiate the action sequence required
*   to complete the GO action
****************************************************************************/

UINT UWM_GO_OUT_SET = ::RegisterWindowMessage(_T("UWM_GO_OUT_SET-{71EDE610-AC36-11d1-8369-00AA005C0507}"));


/****************************************************************************
*   							  UWM_GO_IN_SET
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	LRESULT 0
* Effect: 
*   	The "Go" output bit has been set; initiate the action sequence required
*   to complete the GO action
****************************************************************************/

UINT UWM_GO_IN_SET = ::RegisterWindowMessage(_T("UWM_GO_IN_SET-{71EDE610-AC36-11d1-8369-00AA005C0507}"));


/****************************************************************************
*   							  UWM_IACK_OUT_SET
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	LRESULT 0
* Effect: 
*   	The "IACK" output bit has been set; initiate the action sequence
*   	required to complete the IACK action
****************************************************************************/

UINT UWM_IACK_OUT_SET = ::RegisterWindowMessage(_T("UWM_IACK_OUT_SET-{71EDE610-AC36-11d1-8369-00AA005C0507}"));


/****************************************************************************
*   							  UWM_IACK_IN_SET
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	LRESULT 0
* Effect: 
*   	The "IACK" output bit has been set; initiate the action sequence
*   	required to complete the IACK action
****************************************************************************/

UINT UWM_IACK_IN_SET = ::RegisterWindowMessage(_T("UWM_IACK_IN_SET-{71EDE610-AC36-11d1-8369-00AA005C0507}"));

/****************************************************************************
*   							  UWM_RST_OUT_SET
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	LRESULT 0
* Effect: 
*   	The "RST" output bit has been set; initiate the action sequence
*   	required to complete the RST action
****************************************************************************/

UINT UWM_RST_OUT_SET = ::RegisterWindowMessage(_T("UWM_RST_OUT_SET-{71EDE610-AC36-11d1-8369-00AA005C0507}"));


/****************************************************************************
*   							  UWM_RST_IN_SET
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	LRESULT 0
* Effect: 
*   	The "RST" output bit has been set; initiate the action sequence
*   	required to complete the RST action
****************************************************************************/

UINT UWM_RST_IN_SET = ::RegisterWindowMessage(_T("UWM_RST_IN_SET-{71EDE610-AC36-11d1-8369-00AA005C0507}"));

/****************************************************************************
*   						  UWM_UPDATE_CONTROLS
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	0
* Effect: 
*   	Notifies the main thread to update the control status
****************************************************************************/

UINT UWM_UPDATE_CONTROLS = ::RegisterWindowMessage(_T("UWM_UPDATE_CONTROLS-{71EDE611-AC36-11d1-8369-00AA005C0507}"));

UINT UWM_IE_IN_SET = ::RegisterWindowMessage(_T("UWM_IE_IN_SET-{71EDE612-AC36-11d1-8369-00AA005C0507}"));

UINT UWM_IE_OUT_SET = ::RegisterWindowMessage(_T("UWM_IE_OUT_SET-{71EDE612-AC36-11d1-8369-00AA005C0507}"));


/****************************************************************************
*   	  UWM_Mesgs added to implement Auto-Input & Output
* Inputs:
*   	WPARAM: ignored
*   LPARAM: ignored
* Result: void
*   	0
* Effect: 
*   	
****************************************************************************/

UINT UWM_AUTO_IN_INIT    = ::RegisterWindowMessage(_T("UWM_IN_INIT-{71EDE611-AC36-11d1-8369-00AA005C0507}"));

UINT UWM_AUTO_IN_GENERATE = ::RegisterWindowMessage(_T("UWM_GENERATE-{71EDE611-AC36-11d1-8369-00AA005C0507}"));

UINT UWM_AUTO_IN_SEND     = ::RegisterWindowMessage(_T("UWM_AUTO_IN_SEND-{71EDE611-AC36-11d1-8369-00AA005C0507}"));

UINT UWM_AUTO_IN_GO       = ::RegisterWindowMessage(_T("UWM_AUTO_IN_GOXFER-{71EDE611-AC36-11d1-8369-00AA005C0507}"));

//UINT UWM_AUTO_IN_PKTEND   = ::RegisterWindowMessage(_T("UWM_AUTO_IN_PKTEND-{71EDE611-AC36-11d1-8369-00AA005C0507}"));

UINT UWM_AUTO_OUT_INIT    = ::RegisterWindowMessage(_T("UWM_OUT_INIT-{71EDE611-AC36-11d1-8369-00AA005C0507}"));

UINT UWM_AUTO_OUT_GETSTATE= ::RegisterWindowMessage(_T("UWM_OUT_GETSTATE-{71EDE611-AC36-11d1-8369-00AA005C0507}"));

UINT UWM_AUTO_OUT_GO      = ::RegisterWindowMessage(_T("UWM_AUTO_OUT_GOXFER-{71EDE611-AC36-11d1-8369-00AA005C0507}"));

UINT UWM_AUTO_INPUT       = ::RegisterWindowMessage(_T("UWM_AUTO_INPUT-{EB919054-0C04-4298-9C9A-2D044627CAC0}"));

UINT UWM_AUTO_OUTPUT      = ::RegisterWindowMessage(_T("UWM_AUTO_INPUT-{3B0D4B8D-E984-4a79-8F51-FACABB56101D}"));

/////////////////////////////////////////////////////////////////////////////
// CSimulatorDlg dialog

/****************************************************************************
*   					 CSimulatorDlg::CSimulatorDlg
* Inputs:
*   	CWnd * parent:
* Effect: 
*   	Constructor
****************************************************************************/

CSimulatorDlg::CSimulatorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSimulatorDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSimulatorDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pPollThread = NULL;
	m_bPolling = FALSE;
	m_ulPollIntrvl = MAD_AUTO_POLL_INTERVAL; //50; // 50ms unless otherwise specified
	m_pIntMngr = NULL;
	m_pRegDisp = NULL;
	m_bErr = FALSE;
	m_bTrcDetail = FALSE;
	m_bManual = TRUE;
	//m_bSnglStep = FALSE;
	m_ulGoDoneMin = 100;  // no particular reason, sounds like good default
	m_ulGoDoneVariance = 250; // no particular reason, sounds like good default
	m_eIntErrStyle = eNoIntErrs;
	m_ulProbSpurious = 10; //static 0;
	m_ulProbGenlErr = 10; //static 0
	m_ulProbLost = 0;
	m_ulProbOvrUnd = 0;
	m_ulProbDevBusy = 10; //static 0
	m_nMaxPaktLen = 9999;
	m_ucTermChar = 0x00;
	m_bDisplayOutput = TRUE;
	m_bDisplayTrace = TRUE;
	//m_bBinaryData = FALSE;
	//m_bAutoInput = FALSE;
	//m_bAutoOutput = FALSE;
	m_bAutoDuplex = FALSE;
	m_bAutoSave = FALSE; 
	m_bGenRandomPkts = TRUE; 

	m_eRunMode = eManual;

//#ifdef BLOCK_MODE
	m_hOutput = -1;
#if 0 //#else
    m_pFile    = NULL;
#endif
	
	return;
}



void CSimulatorDlg::DoDataExchange(CDataExchange* pDX)

{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSimulatorDlg)

//* Run mode controls
//*	
	DDX_Control(pDX, IDC_ExeManual, m_cbManual);
	DDX_Control(pDX, IDC_AutoInput, m_cbAutoInput);
	DDX_Control(pDX, IDC_AutoOutput, m_cbAutoOutput);
	DDX_Control(pDX, IDC_AutoDuplex, m_cbAutoDuplex);
	//DDX_Control(pDX, IDC_BURSTINPUT, m_cbBurstInput);
	//DDX_Control(pDX, IDC_SINGLESTEP, m_cbSingleStep);
	DDX_Control(pDX, IDC_GO_HACK_IN, m_csbGoHackIn);
	DDX_Control(pDX, IDC_GO_HACK_OUT, m_csbGoHackOut);

//* Error Injection
//*
	DDX_Control(pDX, IDC_XTRA_INTS, m_cbXtraInts);
	DDX_Control(pDX, IDC_GENL_ERRORS, m_cbGenlErrs);
	DDX_Control(pDX, IDC_OVRUND_ERRS, m_cbOvrUndrErrs);
	DDX_Control(pDX, IDC_DEVBUSY_ERRS, m_cbDevBusyErrs);
	DDX_Control(pDX, IDC_ERROR, m_cbError);
	
	DDX_Control(pDX, IDC_INT, m_cbInt); //errupt);
	DDX_Control(pDX, IDC_MesgID, m_ceMesgID);

//* Device Register Bits
//*
	DDX_Control(pDX, IDC_CNTL_15, m_cbCNTL_15);
	DDX_Control(pDX, IDC_CNTL_14, m_cbCNTL_14);
	DDX_Control(pDX, IDC_CNTL_13, m_cbCNTL_13);
	DDX_Control(pDX, IDC_CNTL_12, m_cbCNTL_12);
	DDX_Control(pDX, IDC_CNTL_11, m_cbCNTL_11);
	DDX_Control(pDX, IDC_CNTL_10, m_cbCNTL_10);
	DDX_Control(pDX, IDC_CNTL_09, m_cbCNTL_9);
	DDX_Control(pDX, IDC_CNTL_08, m_cbCNTL_8);
	DDX_Control(pDX, IDC_CNTL_07, m_cbCNTL_7);
	DDX_Control(pDX, IDC_CNTL_06, m_cbCNTL_6);
	DDX_Control(pDX, IDC_CNTL_05, m_cbCNTL_5);
	DDX_Control(pDX, IDC_CNTL_04, m_cbCNTL_4);
	DDX_Control(pDX, IDC_CNTL_03, m_cbCNTL_3);
	DDX_Control(pDX, IDC_CNTL_02, m_cbCNTL_2);
	DDX_Control(pDX, IDC_CNTL_01, m_cbCNTL_1);
	DDX_Control(pDX, IDC_CNTL_00, m_cbCNTL_0);

	DDX_Control(pDX, IDC_STAT_15, m_cbSTAT_15);
	DDX_Control(pDX, IDC_STAT_14, m_cbSTAT_14);
	DDX_Control(pDX, IDC_STAT_13, m_cbSTAT_13);
	DDX_Control(pDX, IDC_STAT_12, m_cbSTAT_12);
	DDX_Control(pDX, IDC_STAT_11, m_cbSTAT_11);
	DDX_Control(pDX, IDC_STAT_10, m_cbSTAT_10);
	DDX_Control(pDX, IDC_STAT_09, m_cbSTAT_9);
	DDX_Control(pDX, IDC_STAT_08, m_cbSTAT_8);
	DDX_Control(pDX, IDC_STAT_07, m_cbSTAT_7);
	DDX_Control(pDX, IDC_STAT_06, m_cbSTAT_6);
	DDX_Control(pDX, IDC_STAT_05, m_cbSTAT_5);
	DDX_Control(pDX, IDC_STAT_04, m_cbSTAT_4);
	DDX_Control(pDX, IDC_STAT_03, m_cbSTAT_3);
	DDX_Control(pDX, IDC_STAT_02, m_cbSTAT_2);
	DDX_Control(pDX, IDC_STAT_01, m_cbSTAT_1);
	DDX_Control(pDX, IDC_STAT_00, m_cbSTAT_0);

	DDX_Control(pDX, IDC_INTEN_15, m_cbINTEN_15);
	DDX_Control(pDX, IDC_INTEN_14, m_cbINTEN_14);
	DDX_Control(pDX, IDC_INTEN_13, m_cbINTEN_13);
	DDX_Control(pDX, IDC_INTEN_12, m_cbINTEN_12);
	DDX_Control(pDX, IDC_INTEN_11, m_cbINTEN_11);
	DDX_Control(pDX, IDC_INTEN_10, m_cbINTEN_10);
	DDX_Control(pDX, IDC_INTEN_09, m_cbINTEN_9);
	DDX_Control(pDX, IDC_INTEN_08, m_cbINTEN_8);
	DDX_Control(pDX, IDC_INTEN_07, m_cbINTEN_7);
	DDX_Control(pDX, IDC_INTEN_06, m_cbINTEN_6);
	DDX_Control(pDX, IDC_INTEN_05, m_cbINTEN_5);
	DDX_Control(pDX, IDC_INTEN_04, m_cbINTEN_4);
	DDX_Control(pDX, IDC_INTEN_03, m_cbINTEN_3);
	DDX_Control(pDX, IDC_INTEN_02, m_cbINTEN_2);
	DDX_Control(pDX, IDC_INTEN_01, m_cbINTEN_1);
	DDX_Control(pDX, IDC_INTEN_00, m_cbINTEN_0);

	DDX_Control(pDX, IDC_INTID_15, m_cbINTID_15);
	DDX_Control(pDX, IDC_INTID_14, m_cbINTID_14);
	DDX_Control(pDX, IDC_INTID_13, m_cbINTID_13);
	DDX_Control(pDX, IDC_INTID_12, m_cbINTID_12);
	DDX_Control(pDX, IDC_INTID_11, m_cbINTID_11);
	DDX_Control(pDX, IDC_INTID_10, m_cbINTID_10);
	DDX_Control(pDX, IDC_INTID_09, m_cbINTID_9);
	DDX_Control(pDX, IDC_INTID_08, m_cbINTID_8);
	DDX_Control(pDX, IDC_INTID_07, m_cbINTID_7);
	DDX_Control(pDX, IDC_INTID_06, m_cbINTID_6);
	DDX_Control(pDX, IDC_INTID_05, m_cbINTID_5);
	DDX_Control(pDX, IDC_INTID_04, m_cbINTID_4);
	DDX_Control(pDX, IDC_INTID_03, m_cbINTID_3);
	DDX_Control(pDX, IDC_INTID_02, m_cbINTID_2);
	DDX_Control(pDX, IDC_INTID_01, m_cbINTID_1);
	DDX_Control(pDX, IDC_INTID_00, m_cbINTID_0);

//* Device Input/Ouput windows
//*
	DDX_Control(pDX, IDC_GENERATE, m_cbGenerate);

#if 0 //#ifndef BLOCK_MODE	
	DDX_Control(pDX, IDC_TRANSFER, m_ceTransfer);
	DDX_Control(pDX, IDC_SEND, m_cbSend);  //* Char-mode device
#endif

	DDX_Control(pDX, IDC_DATA, m_cbxData);
	DDX_Control(pDX, IDC_Data2Device, m_lbOutPackets);


//*** Get State button(s)
//*
	DDX_Control(pDX, IDC_GETSTATE, m_cbGetState); 

	DDX_Control(pDX, IDC_TRACE, m_wndTrace); //* Trace window

	//DDX_Control(pDX, ID_FILE_AUTOSAVE, m_bAutoSave);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSimulatorDlg, CDialog)
//{{AFX_MSG_MAP(CSimulatorDlg)
ON_WM_SYSCOMMAND()
ON_WM_DESTROY()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_WM_CLOSE()
ON_WM_SIZE() 
ON_WM_GETMINMAXINFO()
//ON_WM_TIMER()
ON_WM_INITMENUPOPUP()
//
ON_COMMAND(ID_EDIT_CLEAR, OnClearTrace)
ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
ON_COMMAND(ID_FILE_OPENABSTRACTDEVICE, OnOpenDevice)

ON_COMMAND(ID_FILE_EXIT, OnFileExit)
ON_COMMAND(ID_FILE_SAVETRACE_SAVE, OnTraceSave)
ON_COMMAND(ID_FILE_SAVETRACE_SAVEAS, OnTraceSaveAs)
ON_COMMAND(ID_FILE_SAVEDEVICEOUTPUT, OnOutputSave)
ON_COMMAND(ID_FILE_AUTOSAVE, OnOutputAutoSave)
ON_COMMAND(ID_FILE_SAVE_DEVICEDATA,  OnFileSaveDeviceData)
ON_COMMAND(ID_FILE_RETRIEVE_DEVICEDATA, OnFileRetrieveDeviceData)
ON_COMMAND(ID_FILE_MAPDEVICEREGISTERS, OnFileMapDeviceRegs)
ON_COMMAND(ID_HELP_INDEX, OnHelpIndex)
ON_COMMAND(IDM_INTERRUPT_MGT, OnIntErrMgt)
//ON_COMMAND(IDM_SETUP_IRQ, OnSetupIrq)
//ON_COMMAND(ID_SETUP_PACKETTERMINATION, OnSetupPacketTermination)
//ON_COMMAND(IDM_HDWSIM_TRACE, OnHdwsimTrace)
ON_COMMAND(IDM_REG_DISP, OnRegDisp)
ON_COMMAND(ID_VIEW_ABSTRACTDEVICEDRIVERTRACE, OnHdwsimTrace)
//ON_COMMAND(ID_VIEW_DETAILEDINTERNALTRACE, OnDebug)
ON_COMMAND(ID_VIEW_OUTPUTDISPLAY, OnViewOutputDisplay)
//ON_COMMAND(ID_VIEW_BINARYDATA, OnSetupBinaryData)
ON_COMMAND(ID_VIEW_REGISTERS, OnRegDisp)
#if 0 //#ifndef BLOCK_MODE
    ON_COMMAND(IDC_CLEAR_INPUT, OnClearInput)
#endif
//ON_COMMAND(ID_WINDOW_CLEARGENERATE, OnClearGenerate)
//ON_COMMAND(IDC_CLEAR_OUTPUT, OnClearOutputWindow)
//ON_COMMAND(IDM_DEBUG, OnDebug)
//ON_COMMAND(ID_RESET_INPUTSTATE, OnResetInputState)
//ON_COMMAND(ID_RESET_OUTPUTSTATE, OnResetOutputState)
ON_COMMAND(ID_RESET_IOwPower, OnResetDevice)
ON_COMMAND(ID_RESET_PLAYDEAD, OnResetPlayDead)
ON_COMMAND(ID_RESET_Init_MAD_Drivr, OnResetInitDriver)


//*** Control button messges
//*
//* Run Mode Controls
//*
ON_BN_CLICKED(IDC_ExeManual, OnManual)
//ON_BN_CLICKED(IDC_SemiAuto, OnBurstInput)
//ON_BN_CLICKED(IDC_AutoInput, OnAutoInput)
//ON_BN_CLICKED(IDC_AutoOutput, OnAutoOutput)
ON_BN_CLICKED(IDC_AutoDuplex, OnAutoDuplex)
//ON_BN_CLICKED(IDC_SINGLESTEP, OnSinglestep)
//ON_BN_CLICKED(IDC_GO_HACK_IN, OnGoHackIn)
//ON_BN_CLICKED(IDC_GO_HACK_OUT, OnGoHackOut)

ON_BN_CLICKED(IDC_INT, OnInt) //* Issue Interrupt

//* Error Injection
//*
ON_BN_CLICKED(IDC_XTRA_INTS, OnXtraInts)
ON_BN_CLICKED(IDC_GENL_ERRORS, OnRandErrs)
ON_BN_CLICKED(IDC_OVRUND_ERRS, OnOvrUndErrs)
ON_BN_CLICKED(IDC_DEVBUSY_ERRS, OnDevBusyErrs)
//ON_BN_CLICKED(IDC_ERROR, OnError)

//* Device Register Controls
//*
//* Status
ON_BN_CLICKED(IDC_STAT_15, OnStatus_15)
ON_BN_CLICKED(IDC_STAT_14, OnStatus_14)
ON_BN_CLICKED(IDC_STAT_13, OnStatus_13)
ON_BN_CLICKED(IDC_STAT_12, OnStatus_12)
ON_BN_CLICKED(IDC_STAT_11, OnStatus_11)
ON_BN_CLICKED(IDC_STAT_10, OnStatus_10)
ON_BN_CLICKED(IDC_STAT_09, OnStatus_9)
ON_BN_CLICKED(IDC_STAT_08, OnStatus_8)
ON_BN_CLICKED(IDC_STAT_07, OnStatus_7)
ON_BN_CLICKED(IDC_STAT_06, OnStatus_6)
ON_BN_CLICKED(IDC_STAT_05, OnStatus_5)
ON_BN_CLICKED(IDC_STAT_04, OnStatus_4)
ON_BN_CLICKED(IDC_STAT_03, OnStatus_3)
ON_BN_CLICKED(IDC_STAT_02, OnStatus_2)
ON_BN_CLICKED(IDC_STAT_01, OnStatus_1)
ON_BN_CLICKED(IDC_STAT_00, OnStatus_0)

//* Int ID
ON_BN_CLICKED(IDC_INTID_15, OnIntID_15)
ON_BN_CLICKED(IDC_INTID_14, OnIntID_14)
ON_BN_CLICKED(IDC_INTID_13, OnIntID_13)
ON_BN_CLICKED(IDC_INTID_12, OnIntID_12)
ON_BN_CLICKED(IDC_INTID_11, OnIntID_11)
ON_BN_CLICKED(IDC_INTID_10, OnIntID_10)
ON_BN_CLICKED(IDC_INTID_09, OnIntID_9)
ON_BN_CLICKED(IDC_INTID_08, OnIntID_8)
ON_BN_CLICKED(IDC_INTID_07, OnIntID_7)
ON_BN_CLICKED(IDC_INTID_06, OnIntID_6)
ON_BN_CLICKED(IDC_INTID_05, OnIntID_5)
ON_BN_CLICKED(IDC_INTID_04, OnIntID_4)
ON_BN_CLICKED(IDC_INTID_03, OnIntID_3)
ON_BN_CLICKED(IDC_INTID_02, OnIntID_2)
ON_BN_CLICKED(IDC_INTID_01, OnIntID_1)
ON_BN_CLICKED(IDC_INTID_00, OnIntID_0)


//* Data Xfer Control buttons
//*
//ON_BN_CLICKED(IDC_GENERATE, OnGenerate) //* Generate an input packet

#if 0 //#ifndef BLOCK_MODE
	ON_BN_CLICKED(IDC_SEND, OnSend)     //* Transfer 1 byte
#endif

//* Get State Buttons
//*
ON_BN_CLICKED(IDC_GETIOSTATE, OnGetIOState)
//ON_BN_CLICKED(IDC_GetInpState, OnGetInputState)
//ON_BN_CLICKED(IDC_GetOutState, OnGetOutputState)

ON_BN_CLICKED(IDC_COMMENT, OnComment) //* Insert Trace Comment


//ON_CBN_SELENDOK(IDC_DATA, OnSelendokData)
//ON_CBN_EDITCHANGE(IDC_DATA, OnEditchangeData)
//
ON_REGISTERED_MESSAGE(UWM_PULSE, OnPulse)
ON_REGISTERED_MESSAGE(UWM_POLL, OnPoll)
ON_REGISTERED_MESSAGE(UWM_IMGR_CLOSE, OnImgrClose)
ON_REGISTERED_MESSAGE(UWM_REG_CLOSE, OnRegClose)
ON_REGISTERED_MESSAGE(UWM_OPEN_FAILED, OnOpenFailed)
ON_REGISTERED_MESSAGE(UWM_SET_TIMER_OUT, OnSetTimerOut)
ON_REGISTERED_MESSAGE(UWM_SET_TIMER_IN, OnSetTimerIn)
//ON_REGISTERED_MESSAGE(UWM_GO_HACK_IN, OnExecuteGoHackIn)
//ON_REGISTERED_MESSAGE(UWM_GO_HACK_OUT, OnExecuteGoHackOut)
ON_REGISTERED_MESSAGE(UWM_SET_MANUAL_MODE, OnSetManualMode)
//ON_REGISTERED_MESSAGE(UWM_GEN_RANDOM_PKTS, OnGenRandomPkts)
ON_REGISTERED_MESSAGE(UWM_SET_PROB, OnUpdateProbabilities)
//ON_REGISTERED_MESSAGE(UWM_AUTO_IN_INIT, InitDeviceIn)
//ON_REGISTERED_MESSAGE(UWM_AUTO_IN_GENERATE, OnGenerate)
//ON_REGISTERED_MESSAGE(UWM_AUTO_INPUT, OnAutoInput)
//ON_REGISTERED_MESSAGE(UWM_AUTO_OUTPUT, OnAutoOutput)

#if 0 //#ifndef BLOCK_MODE
	ON_REGISTERED_MESSAGE(UWM_AUTO_IN_SEND, OnSend)
#endif
//ON_REGISTERED_MESSAGE(UWM_AUTO_IN_GO, OnGoHackIn)
//ON_REGISTERED_MESSAGE(UWM_AUTO_IN_PKTEND, OnAutoInPktEnd)
//ON_REGISTERED_MESSAGE(UWM_AUTO_OUT_INIT, InitDeviceOut)
//ON_REGISTERED_MESSAGE(UWM_AUTO_OUT_GETSTATE, GetOutputState)
//ON_REGISTERED_MESSAGE(UWM_AUTO_OUT_GO, OnGoHackOut)
//}}AFX_MSG_MAP

// Nominally obsolete
//ON_REGISTERED_MESSAGE(UWM_RST_IN_SET, OnRstInSet)
//ON_REGISTERED_MESSAGE(UWM_RST_OUT_SET, OnRstOutSet)
//ON_REGISTERED_MESSAGE(UWM_GO_OUT_SET, OnGoOutSet)
//ON_REGISTERED_MESSAGE(UWM_GO_IN_SET, OnGoInSet)
//ON_REGISTERED_MESSAGE(UWM_IACK_IN_SET, OnIACKInSet)
//ON_REGISTERED_MESSAGE(UWM_IACK_OUT_SET, OnIACKOutSet)
ON_REGISTERED_MESSAGE(UWM_UPDATE_CONTROLS, OnUpdateDlgCntls)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSimulatorDlg message handlers



/****************************************************************************
*   				   CSimulatorDlg::LogControlChange
* Inputs:
*   	BYTE o: Old command register
*   BYTE n: New command register
*   LPCTSTR reason: Reason string, may be NULL
* Result: void
*   	
* Effect: 
*   	Logs any bits that changed
****************************************************************************/

void CSimulatorDlg::LogControlChange(USHORT uOld, USHORT uNew, LPCTSTR reason)


{
	LogChange(uOld, uNew, MADMASK_BIT15, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT14, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT13, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT12, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT11, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT10, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT9, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT8, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT7, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT6, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT5, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT4, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT3, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT2, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT1, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT0, TRACE_TYPE_BIT_X, reason);

    return;
}


/****************************************************************************
*   				   CSimulatorDlg::LogStatusChange
* Inputs:
*   	BYTE o: Old status
*   BYTE n: New status
*   LPCTSTR reason: Commentary text, defaults to NULL
* Result: void
*   	
* Effect: 
*   	Logs changes in the input status line
****************************************************************************/

void CSimulatorDlg::LogStatusChange(USHORT uOld, USHORT uNew, LPCTSTR reason)

{
	LogChange(uOld, uNew, MADMASK_BIT15, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT14, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT13, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT12, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT11, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT10, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT9, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT8, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT7, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT6, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT5, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT4, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT3, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT2, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT1, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT0, TRACE_TYPE_BIT_X, reason);

	return;
}


/****************************************************************************
*   				   CSimulatorDlg::LogIntEnableChange
* Inputs:
*   	BYTE o: Old command register
*   BYTE n: New command register
*   LPCTSTR reason: Reason string, may be NULL
* Result: void
*   	
* Effect: 
*   	Logs any bits that changed
****************************************************************************/

void CSimulatorDlg::LogIntEnableChange(USHORT uOld, USHORT uNew, LPCTSTR reason)


{
	LogChange(uOld, uNew, MADMASK_BIT15, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT14, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT13, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT12, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT11, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT10, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT9, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT8, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT7, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT6, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT5, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT4, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT3, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT2, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT1, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT0, TRACE_TYPE_BIT_X, reason);

    return;
}


/****************************************************************************
*   				   CSimulatorDlg::LogIntIdChange
* Inputs:
*   	BYTE o: Old command register
*   BYTE n: New command register
*   LPCTSTR reason: Reason string, may be NULL
* Result: void
*   	
* Effect: 
*   	Logs any bits that changed
****************************************************************************/

void CSimulatorDlg::LogIntIdChange(USHORT uOld, USHORT uNew, LPCTSTR reason)


{
	LogChange(uOld, uNew, MADMASK_BIT15, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT14, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT13, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT12, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT11, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT10, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT9, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT8, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT7, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT6, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT5, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT4, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT3, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT2, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT1, TRACE_TYPE_BIT_X, reason);
	LogChange(uOld, uNew, MADMASK_BIT0, TRACE_TYPE_BIT_X, reason);

    return;
}


/****************************************************************************
*   					   CSimulatorDlg::LogChange
* Inputs:
*   	BYTE o: Old value
*   BYTE n: New value
*   BYTE ucStatus: Status flag
*   UINT type: Type report
*   LPCTSTR reason: commentary text, or NULL
* Result: void
*   	
* Effect: 
*   	Logs
****************************************************************************/

void CSimulatorDlg::LogChange(USHORT uOld, USHORT uNew, USHORT uStatus, UINT type,
	LPCTSTR reason)

{
USHORT uDelta = (USHORT)((uOld ^ uNew) & uStatus);

	if (uDelta != 0) 		/* log err change */
		{
		TraceItem* item = new TraceItem(type, (uNew & uStatus) != 0, reason);
		m_wndTrace.AddString(item);
		} 

    return;
}


/****************************************************************************
*   				CSimulatorDlg::GenerateAndLogInterrupt
* Inputs:
*   LPCTSTR annotation = NULL: Notation to add, or NULL if none
* Result: void
*   	
* Effect: 
*   	Logs an interrupt event and generates the interrupt.
*   Sets the IE bit
* Notes:
*   Before calling this, the following conditions must be true:
*   	The IE bit must already be set
*   	The DONE bit must already be set
*   	The INT bit must already be set
****************************************************************************/


void CSimulatorDlg::GenerateAndLogInterrupt(LPCTSTR annotation)

{
//static ULONG MesgID = 1;
TraceItem* item = new TraceItem(TRACE_TYPE_INT, annotation);

	m_wndTrace.AddString(item);
    LogInterrupt(annotation);
	//m_IoCtlObj.GenerateInterrupt();

	return;
}

void CSimulatorDlg::LogInterrupt(LPCTSTR annotation)

{
TraceItem* item = new TraceItem(TRACE_TYPE_INT, annotation);

	m_wndTrace.AddString(item);

	return;
}


/****************************************************************************
*   					   CSimulatorDlg::OnComment
* Result: void
*   	
* Effect: 
*   	Adds a commment to the trace box
****************************************************************************/

void CSimulatorDlg::OnComment()

{
	CComment dlg;
	switch (dlg.DoModal())
		{
			/* DoModal */
		case IDOK:
			break;
		case IDCANCEL:
			return;
		} /* DoModal */

	TraceItem* e = new TraceItem(TRACE_TYPE_COMMENT, dlg.m_Comment);
	m_wndTrace.AddString(e);
}


void CSimulatorDlg::OnFileMapDeviceRegs()

{
	//::MessageBox(NULL, "OnFileMapDeviceRegs", NULL, MB_OK);
	UINT bRC = m_IoCtlObj.MapWholeDevice(&m_pDeviceRegs, &m_pPioRead, &m_pPioWrite, &m_pDeviceData);

	return;
}


void CSimulatorDlg::OnFileRetrieveDeviceData()
{
	FileAccessHelperFunxn(FALSE); //Read

	return;
}


void CSimulatorDlg::OnFileSaveDeviceData()
{
	FileAccessHelperFunxn(TRUE); //Write
    
	return;
}


void CSimulatorDlg::FileAccessHelperFunxn(BOOL bWrite)
{
UINT uRC;
char szDeviceName[50] = MAD_DEVICE_NAME_PREFIX;
char szInfoMsg[100] = "Retrieve device data from: ";
char szObjectName[60] = ""; //"\\\\?\\";
char szUnit[5] = "";
errno_t errno;
BOOL bRC;
size_t IoLen     = MAD_DEFAULT_DATA_EXTENT;
ULONG FileAccess = GENERIC_READ;
ULONG FileShare  = FILE_SHARE_READ;
ULONG CreateDisp = OPEN_EXISTING;
ULONG  nCount;
HANDLE hDevice;
DWORD  dwGLE;

    if (m_pDeviceData == NULL)
	    {
		sprintf_s(szInfoMsg, 100,
			      "No mapping for device data. Unable to save/retrieve device data to/from disk");
		uRC = ::MessageBox(NULL, szInfoMsg, NULL, MB_ICONEXCLAMATION);
		return;
	    }

    if (bWrite) 
	    {
		strcpy_s(szInfoMsg, 100, "Save device data to: "); 
	    FileAccess = GENERIC_WRITE;
		FileShare  = FILE_SHARE_WRITE;
		CreateDisp = CREATE_ALWAYS;
	    }

    errno = _itoa_s(m_nSerialNo, szUnit, 5, 10);
	strcat_s(szDeviceName, 50, szUnit);
    strcat_s(szDeviceName, 50, MAD_DATAFILE_SUFFIX);
	strcat_s(szInfoMsg, 100, szDeviceName);
    uRC = ::MessageBox(NULL, szInfoMsg, NULL, (MB_ICONQUESTION|MB_OKCANCEL));
	if (uRC == IDCANCEL)
		return;

	strcat_s(szObjectName, 60, szDeviceName);  
	hDevice = CreateFile(szObjectName, FileAccess, FileShare,
						  NULL, CreateDisp, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
		{
		sprintf_s(szInfoMsg, 100, "Target device open failed! %s\n", szObjectName); 
		uRC = ::MessageBox(NULL, szInfoMsg, NULL, MB_ICONSTOP);
        return;
	    }
	
	if (!bWrite) 
	    bRC = ReadFile(hDevice, m_pDeviceData, (DWORD)IoLen, &nCount, NULL);
	else
        bRC = WriteFile(hDevice, m_pDeviceData, (DWORD)IoLen, &nCount, NULL);
	CloseHandle(hDevice);
	if (bRC)
	    {
        sprintf_s(szInfoMsg, 100, "%d bytes I/O completed", nCount);
		uRC = ::MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);
	    }
	else
    	{
        dwGLE = GetLastError(); 
        sprintf_s(szInfoMsg, 100, "%d bytes I/O failed! GLE=%d ", (ULONG)IoLen, dwGLE);
       	uRC = ::MessageBox(NULL, szInfoMsg, NULL, MB_ICONEXCLAMATION);
        }

	return;
}
