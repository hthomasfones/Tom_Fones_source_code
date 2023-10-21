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
/*  Exe file ID  : MadTestApp.exe                                              */
/*                                                                             */
/*  Module  NAME : MainFrm.h                                                   */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class MainFrm                         */
/*                                                                             */
/*******************************************************************************/


// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__67765BC6_5B84_45BA_9079_FA7079A35706__INCLUDED_)
#define AFX_MAINFRM_H__67765BC6_5B84_45BA_9079_FA7079A35706__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
    BOOL  m_bBinaryMode;
    BOOL  m_bViewContIO;
	LONG  m_nSerialNo;

	//{{AFX_MSG(CMainFrame)
	afx_msg void OnClearDisplay();
	afx_msg void OnSetBinaryMode();
	afx_msg void OnFileOpenDevice1();
	afx_msg void OnFileOpenDevice2();
	afx_msg void OnFileOpenDevice3();
	afx_msg void OnDeviceClose();
	afx_msg void OnPrint();
	afx_msg void OnPrintSetup();
	
	afx_msg void OnRead16bytes();
	afx_msg void OnRead64bytes();
	afx_msg void OnInputRead256bytes();
	afx_msg void OnRead512bytes();
	afx_msg void OnWrite16bytes();
	//
	afx_msg void OnOutputWrite64bytes();
	afx_msg void OnOutputWrite256bytes();
	afx_msg void OnWrite512bytes();
	afx_msg void OnInitializeDevice();
	afx_msg void OnFileIdentifydriver();
		
	afx_msg void OnOutputWrite_N_Bytes();
	
	afx_msg void OnOutputFile();
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnInputReadFile();
	afx_msg void OnIoControlsNonvalidControl();
	afx_msg void OnVIEWContinuousIO();
	afx_msg void OnINPUTReadPackets();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnWriteContinuousPackets();
	afx_msg void OnWriteContPkts_W_Errs();
	afx_msg void OnInputReadfileErrs();
	afx_msg void OnPnpEjectAlldevices();
	afx_msg void OnPnpEjectDevice1();
	afx_msg void OnPnpEjectDevice2();
	afx_msg void OnPnpEjectDevice3();
	afx_msg void OnPnpPluginAlldevices();
	afx_msg void OnPnpPluginDevice1();
	afx_msg void OnPnpPluginDevice2();
	afx_msg void OnPnpPluginDevice3();
	afx_msg void OnPnpUnplugAlldevices();
	afx_msg void OnPnpUnplugDevice1();
	afx_msg void OnPnpUnplugDevice2();
	afx_msg void OnPnpUnplugDevice3();
	afx_msg void OnPnpMonitor();
	afx_msg void OnFileDeviceOpen();
	afx_msg void OnViewClearDisplay();
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditAnnotate();
	afx_msg void OnEditCopyall();
	afx_msg void OnDevCntrlsWrite();
	afx_msg void OnDevCntrlsRead();
	afx_msg void OnIocntlGetInstatus();
	afx_msg void OnIocntlGetOutStatus();
	afx_msg void OnIocntlSetInRst();
	afx_msg void OnIocntlSetOutRst();
	afx_msg void OnIocntlResetInRst();
	afx_msg void OnIocntlResetOutRst();
	afx_msg void OnIoctlSetControlReg();
	afx_msg void OnIoctlSetIntEnableReg();
	afx_msg void OnPowerMngtDevState0All();
	afx_msg void OnPowerMngtDevState0Unit1();
	afx_msg void OnPowerMngtDevState0Unit2();
	afx_msg void OnPowerMngtDevState0Unit3();
	afx_msg void OnPowerMngtDevState1All();
	afx_msg void OnPowerMngtDevState1Unit1();
	afx_msg void OnPowerMngtDevState1Unit2();
	afx_msg void OnPowerMngtDevState1Unit3();
	afx_msg void OnPowerMngtDevState2All();
	afx_msg void OnPowerMngtDevState2Unit1();
	afx_msg void OnPowerMngtDevState2Unit2();
	afx_msg void OnPowerMngtDevState2Unit3();
	afx_msg void OnPowerMngtDevState3All();
	afx_msg void OnPowerMngtDevState3Unit1();
	afx_msg void OnPowerMngtDevState3Unit2();
	afx_msg void OnPowerMngtDevState3Unit3();
	afx_msg void OnPowerMngtSystemState0();
	afx_msg void OnPowerMngtSystemState1();
	afx_msg void OnPowerMngtSystemState2();
	afx_msg void OnPowerMngtSystemState3();
	afx_msg void OnWmiBusQueryBlock();
	afx_msg void OnWmiBusQueryRegistry();
	afx_msg void OnWmiBusQueryItem();
	afx_msg void OnWmiBusSetItem();
	afx_msg void OnWmiDevQueryBlock();
    afx_msg void OnWmiDevQueryRegistry();
	afx_msg void OnWmiDevSetBlock();
	afx_msg void OnWmiDevSetItem();
    afx_msg void OnWmiDevQueryDriverBlock();
    afx_msg void OnWmiDevSetDriverBlock();
    afx_msg void OnWmiDevExecMethod();
    afx_msg void OnWmiDevFunctionControl();
	afx_msg void OnFileOpenDev1Read();
	afx_msg void OnFileOpenDev1Write();
	afx_msg void OnIoctlGetStatusReg();
	afx_msg void OnIoctlGetIntIdReg();
	afx_msg void OnIoControls_BlueScreen();
	afx_msg void OnIoctlTestInit();
    afx_msg void OnWinDbgAssert();
	afx_msg void OnWinDbgBreak();
    afx_msg void OnWinDbgException();
    afx_msg void OnWinDbgHang();
    afx_msg void OnUM_Hang();
    afx_msg void OnWinDbgVerifier();

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	HANDLE DeviceOpen(ULONG SerialNo, ULONG OpenFlags);
    void Read_N_Bytes(ULONG nRead, BOOL bMesgBox=TRUE); 
	void Write_N_Bytes(char szWrite[], ULONG nWrite, BOOL bMesgBox=TRUE); 
	void Read_N_Sectors(ULONG NumSectors, ULONG Indx, BOOL bMesgBox=TRUE);  
	void Write_N_Sectors(ULONG NumSectors, ULONG Indx, BOOL bMesgBox=TRUE);  
	BOOL RW_N_Sectors(ULONG NumSectors, ULONG Indx, BOOL bWrite, BOOL bMesgBox);  
	void RunAutoScript();
	BOOL ParseReadCmd(char szCmdLineParms[]);
	BOOL ParseWriteCmd(char szCmdLineParms[]);
	BOOL ParseIoCmd(char szCmdLineParms[], BOOL bWrite);
	BOOL ParseIoctl(char szCmdLineParms[]);
	void AppReadFile(/*FILE* pFile,*/ int hOutput,
		             PULONG ulPktCount, PULONG pIoCount, BOOL bAllowErrs);
	void WriteContPackets(BOOL bAllowErrs); 
    void Update_Display_Log(PCHAR pChar); 
	void Exec_PnP_Enumerate(CString csWinExec);
	void Exec_PowerMngt_Funxn(CString csSuffix);
	void Assign_GLE_Text(DWORD dwGLE, char szErrtext[]);
	void Init_MemBlock(PUCHAR pMemBlock, ULONG ulLen);
	void ViewDevRegMap();
	BOOL CreateProcessInClientArea(char szAppName[], char szCmdLine[],
		                           LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                   LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                   BOOL bInheritHndls,   DWORD dwCreateFlags,
		                           LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory);

	BOOL CMainFrame::Submit_Ioctl(ULONG ulIoctl, PUCHAR pWrite, ULONG ulWrite,
							      PUCHAR pRead, ULONG ReadLen, PULONG pIoCount,
							      char szInfoText[], char szErrPrefix[],
							      BOOL bMesgBox = TRUE); 
	void  CMainFrame::ClearClientArea(); 
	PCHAR Find1stNonBlank(PCHAR pChar) {while (*pChar == ' ') pChar++; return pChar;}
	PCHAR Find1stBlank(PCHAR pChar) {while ((*pChar != ' ') && (*pChar != 0x00)) pChar++; return pChar;}

public:
	afx_msg void OnDeviceIoctlGetRegs();
	afx_msg void OnDeviceIoctlMapDeviceRegs();
	afx_msg void OnPioRead();
	afx_msg void OnPioWrite();
	afx_msg void OnPioReceive();
	afx_msg void OnPioTransmit();
	afx_msg void OnPioLoopback();
	afx_msg void OnViewDevRegMap();
	afx_msg void OnHelpAbout();
	afx_msg void OnHelpViewDevRegMap();
	afx_msg void OnHelpShowReadmefile();
	afx_msg void OnHelpViewSimulationDiagram();
	afx_msg void OnCacheAlignRead();
	afx_msg void OnCacheAlignWrite();
	afx_msg void OnCacheRead();
	afx_msg void OnCacheWrite();
	afx_msg void OnInputOneSector();
	afx_msg void OnInputFourSectors();
	afx_msg void OnInputSixteenSectors();
	afx_msg void OnOutputOneSector();
	afx_msg void OnOutputFourSectors();
	afx_msg void OnOutputSixteenSectors();
	afx_msg void OnInputRunScript();
	afx_msg void OnOutputRunScript();
	afx_msg void OnPlugnPlayStress();
	afx_msg void OnPlugnplayStress();
	afx_msg void OnFileLogFileClose();
	afx_msg void OnFileLogFileOpen();
	afx_msg void OnIocontrolsResetIndeces();
	afx_msg void OnWmiBusQuery();
	afx_msg void OnWmiDeviceQuery();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__67765BC6_5B84_45BA_9079_FA7079A35706__INCLUDED_)
