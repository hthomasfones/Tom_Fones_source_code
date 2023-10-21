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
/*  Module  NAME : DlgMenu.cpp                                                 */
/*                                                                             */
/*  DESCRIPTION  : Function definitions for the DlgMenu message handlers       */
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
//#include "PacketTerm.h"
#include "MadDevSimUIDlg.h"
//#include "buzzphrase.h"
#include "AutoExecDlg.h"
#include "About.h"
#include "comment.h"
#include "HdwTrace.h"
#include "uwm.h"
#include "DlgMenu.h"


//*** File Menu Message Handlers   //////////////////////////////////////////
//*
/****************************************************************************
*   					  CSimulatorDlg::OnOpenDevice
* Result: void
*   	
* Effect: 
*   	Opens the Model-Abstract-Demo Device 
*	
****************************************************************************/

void CSimulatorDlg::OnOpenDevice()

{
int rc;
//LONG nSerialNo;
UINT MB_Icon = MB_ICONINFORMATION;
char szInfoMsg[100] = "Model-Abstract-Demo Device Opened. "; 

	if (m_IoCtlObj.m_bOpen)
		return;

    strcpy_s(m_szOutFile, 50, "DToutput0.tst");
	
	//* Open the simulator device
	//*
	if (m_IoCtlObj.Open(&m_nSerialNo))
		{ 
		m_szOutFile[8] = (char)(m_szOutFile[8] + (CHAR)m_nSerialNo);
		strcat_s(szInfoMsg, 100, m_szOutFile);
		}
	else
		{
		/* failed */
		sprintf_s(szInfoMsg, 100,
				  "Device open/initialize failure of the Model-Abstract-Demo Device\n. Serial#=%d", m_nSerialNo);
		MB_Icon = MB_ICONEXCLAMATION;
		} 

	rc = MessageBox(szInfoMsg, NULL, MB_Icon);

	return;
}

//********
void CSimulatorDlg::OnOutputSave()

{
static char szTemp[100];
static char szNuline[] = "\n";
register int i;
BOOL prompt = FALSE;
int nLen;
CString cs_Name = m_szOutFile;

	TRY
		{
		CStdioFile f(cs_Name,
			CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive);

		for (i = 0; i < m_lbOutPackets.GetCount(); i++)
			{
			nLen = m_lbOutPackets.SetSel(i, TRUE);
			nLen = m_lbOutPackets.GetText(i, szTemp);
			f.WriteString(szTemp);
			f.WriteString(szNuline);
			nLen = m_lbOutPackets.SetSel(i, FALSE); //* Reset Highlight
			} 

		f.Close();
		}

	CATCH (CFileException, pVoid)
		{ /* error */
		AfxMessageBox(IDS_FILE_OPEN_FAILED, MB_ICONERROR | MB_OK);
		// TODO: Make this report a meaningful error (it's 5am and I want
		// to finish this before I leave for a trip later today!)
		return; // return without doing anything further
		} /* error */
	END_CATCH

	sprintf_s(szTemp, 100, "Output data saved in file: %s \n", m_szOutFile);
	nLen = MessageBox(szTemp, NULL, MB_ICONINFORMATION);

	return;
}


//********
void CSimulatorDlg::OnOutputAutoSave()

{
UINT uRC;
char szInfoMsg[100];
char szPrefx[] = "DToutput";
char szSufx[] = ".tst";
CString cs_Name = m_szOutFile;

    if (!m_bAutoSave)
		{ 
		sprintf_s(szInfoMsg, 100,
		          "Device output will be saved in file: %s", m_szOutFile); 
        uRC = MessageBox(szInfoMsg, NULL, (MB_YESNO|MB_ICONQUESTION));
        if (uRC == IDNO)
			return;
		}

	m_bAutoSave = !m_bAutoSave;	

	return;
}


/****************************************************************************
*   					   CSimulatorDlg::OnTraceSave
* Result: void
*   	
* Effect: 
*   	Saves the contents of the transcript
****************************************************************************/

void CSimulatorDlg::OnTraceSave()

{
	DoTraceSave(TRUE);
	m_wndTrace.SetModified(FALSE);

	return;
}


/****************************************************************************
*   					  CSimulatorDlg::OnTraceSaveAs
* Result: void
*   	
* Effect: 
*   	Saves the contents of the trace buffer
****************************************************************************/

void CSimulatorDlg::OnTraceSaveAs()

{
	DoTraceSave(FALSE);
	m_wndTrace.SetModified(FALSE);

	return;
}


/****************************************************************************
*   					   CSimulatorDlg::OnFileExit
* Result: void
*   	
* Effect: 
*   	Exits the program
****************************************************************************/

void CSimulatorDlg::OnFileExit()

{
	OnOK();
}


//*** Parameter Menu Message Handlers //////////////////////////////////
//*
/****************************************************************************
*   					 CSimulatorDlg::OnInterruptMgt
* Result: void
*   	
* Effect: Here is the new way.
*         Every menu item click is a new day. 
*   	  Int state to be remembered in the parent dialog box 
*	
****************************************************************************/

void CSimulatorDlg::OnIntErrMgt()

{
CInterruptMgt IntErrMngt;
etIntErrStyle eIntErrStyle = m_eIntErrStyle;
UINT nDM;

	nDM = IntErrMngt.DoModal(&eIntErrStyle);
	if (nDM == IDCANCEL)
		return;

    m_eIntErrStyle = eIntErrStyle;

	return;
}


//*** View Menu Message Handlers ////////////////////////////////////////////
//*
/****************************************************************************
*   						CSimulatorDlg::OnViewOutputDisplay
* Result: void
*   	
* Effect: 
*   	Toggles the display output flag
****************************************************************************/

void CSimulatorDlg::OnViewOutputDisplay()

{
	m_bDisplayOutput = !m_bDisplayOutput;
	
	return;
}


/****************************************************************************
*   						CSimulatorDlg::OnViewTraceDisplay
* Result: void
*   	
* Effect: 
*   	Toggles the display output flag
****************************************************************************/

void CSimulatorDlg::OnViewTraceDisplay()

{
	m_bDisplayTrace = !m_bDisplayTrace;
	
	return;
}


//* Create the register Display dialog box
//*
void CSimulatorDlg::OnRegDisp()

{
//BOOL bRC;
//MAD_CNTL Mad_Cntl;

	if (m_pRegDisp != NULL)
		 m_pRegDisp->SetFocus();
	else
		{ /* create it */
		m_pRegDisp = new CRegDisplay;
		m_pRegDisp->Create(IDD_REGISTERS, this);
		} 
	
	if (!m_pRegDisp == NULL)
		{
       	//bRC = m_IoCtlObj.QueryDevice(&Mad_Cntl);
        m_pRegDisp->SendMessage(UWM_UPDATE_REGS, 0, (LPARAM)m_pDeviceRegs);
		}

	return;
}



//******
void CSimulatorDlg::OnHdwsimTrace()

{
CHdwTrace dlg;

	dlg.m_pIoCtlObj = &m_IoCtlObj;

	dlg.DoModal();
}

//*** Window Menu Message Handlers ////////////////////////////////////////////
//*


//* Clear the trace window
//*
void CSimulatorDlg::OnClearTrace()

{
    ClearTrace();	

    return;
}

//*** Reset Menu Message Handlers ////////////////////////////////////////////
//*
//* Reset everything in sight: Registers & data on the input & output side
//* as well as the trace window
//*
void CSimulatorDlg::OnResetDevice()

{
static UCHAR ucZero = 0x00;

	if (!m_bManual)
		return;

    ResetDevice();

	return;
}


//* Play Dead - Set all of Device memory to 0xFF (and exit)
//*
void CSimulatorDlg::OnResetPlayDead()

{
int rc;
char szInfoMsg[200];

    if (!m_bManual)
	    return;

    ResetPlayDead();

	strcpy_s(szInfoMsg, 200,
		     "The play dead function should cause the bus & device drivers to collaborate on a graceful unplug.\n\n");
	strcat_s(szInfoMsg, 200,
		     "We will exit here without issuing an unplug once you have read this ... \n");
	rc = MessageBox(szInfoMsg, NULL, MB_ICONINFORMATION);

	ExitProcess(0);
    return;
}

//* Issue an Initialize IOCTL to the Abstract Demo Device Simulation Driver
//*
void CSimulatorDlg::OnResetInitDriver()

{
BOOL bResult;
int rc;
//MAD_CNTL Mad_Cntl;

	bResult = m_IoCtlObj.InitDriver();
	if (!bResult)
		rc = MessageBox(NULL, NULL, MB_ICONEXCLAMATION);

	if (m_pRegDisp != NULL) //* If the Register Display window is active
		{
       	//bResult = m_IoCtlObj.QueryDevice(&Mad_Cntl);
       	//m_pRegDisp->SendMessage(UWM_UPDATE_REGS, 0, (LPARAM)&Mad_Cntl);
		}

	return;
}


//*** About Message Handler ////////////////////////////////////////////
//*
/****************************************************************************
*   					  CSimulatorDlg::OnHelpIndex
* Result: void
*   	
* Effect: 
*   	Calls Help system
****************************************************************************/

void CSimulatorDlg::OnHelpIndex()

{
	WinHelp(0, HELP_CONTENTS);
}



/****************************************************************************
*   					   CSimulatorDlg::OnAppAbout
* Result: void
*   	
* Effect: 
*   	Presents the ABOUT box
****************************************************************************/

void CSimulatorDlg::OnAppAbout()

{
	doAbout();

	return;
}


//Old & in the way functions ///////////////////////////////////////////////////
//
//* Launch the packet termination dialog box
/*
void CSimulatorDlg::OnSetupPacketTermination()

{
CPacketTerm PacketTerm; //* The dialog for the packet termination parms
struct _PacketTermParms PaktTermParms;
UINT nDM;

	PaktTermParms.nMaxLen = m_nMaxPaktLen; 
	PaktTermParms.ucTermChar = m_ucTermChar; 
	nDM = PacketTerm.DoModal(&PaktTermParms);
	if (nDM == IDCANCEL)
		return;

	m_nMaxPaktLen = PaktTermParms.nMaxLen;
	m_ucTermChar = PaktTermParms.ucTermChar;

	return;
}

/****************************************************************************
*   					   CSimulatorDlg::OnSetupIrq
* Result: void
*   	
* Effect: 
*   	Brings up the IRQ setup dialog
****************************************************************************./

void CSimulatorDlg::OnSetupIrq()

{
CSetupDlg AutoExecDlg;
AUTOEXECUTE_PARMS sAutoExecParms;
UINT nDM;

    sAutoExecParms.nPollInt        = m_ulPollIntrvl;
    sAutoExecParms.bGenRandomPkts  = m_bGenRandomPkts;
	nDM = AutoExecDlg.DoModal(&sAutoExecParms);
	if (nDM == IDCANCEL)
		return;

    m_ulPollIntrvl   = sAutoExecParms.nPollInt;
    m_bGenRandomPkts = sAutoExecParms.bGenRandomPkts; 

	return;
}
/* */
/****************************************************************************
*   						CSimulatorDlg::OnDebug
* Result: void
*   	
* Effect: 
*   	Toggles the debug flag
****************************************************************************./

void CSimulatorDlg::OnDebug()

{
	m_bTrcDetail = !m_bTrcDetail;	

	return;
}
/* */

/****************************************************************************
*   					  CSimulatorDlg::OnClearGenerate
* Result: void
*   	
* Effect: 
*   Clears the window of generated data. This has no other effect

****************************************************************************./
void CSimulatorDlg::OnClearGenerate()

{
	ClearGenerate();

	return;
}
/* */
//*** Clear the device output window
/*
void CSimulatorDlg::OnClearOutputWindow()

{
    ClearOutputWindow();
	//memset(m_szOutPacket, 0x00, 250);
	//m_nPktCount = 0;

    return;
}
/* */