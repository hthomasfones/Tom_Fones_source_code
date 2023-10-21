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
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : MadDevSimUIDlg.h                                            */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MadDevSimUIDlg class                      */
/*                                                                             */
/*******************************************************************************/


/////////////////////////////////////////////////////////////////////////////
// CSimulatorDlg dialog

class CInterruptMgt;

//class CMadDevSimUIDlg: public CDialog
class CSimulatorDlg: public CDialog
	{
	// Construction
	public:
	    CSimulatorDlg(CWnd* pParent = NULL);	// standard constructor
	    //BOOL OnInitDialog(void);

	    // Dialog Data
	    //{{AFX_DATA(CSimulatorDlg)
	    enum {IDD = IDD_SIMULATOR_DIALOG};

//* Run mode controls
//*
		CButton			m_cbManual;
	    CButton			m_cbBurstInput;
	    CButton			m_cbAutoInput;
	    CButton			m_cbAutoOutput;
	    CButton			m_cbAutoDuplex;
		CButton			m_cbSingleStep;
	    CSpinnerButton	m_csbGoHackIn;
	    CSpinnerButton	m_csbGoHackOut;

	    CButton			m_cbInt; //* Issue Interrupt

//* Error injection controls
//*
	    CButton			m_cbXtraInts;
	    CButton			m_cbGenlErrs;
	    CButton			m_cbOvrUndrErrs;
	    CButton			m_cbDevBusyErrs;
	    CButton			m_cbError;

//* Device register controls
//*
		CNumericEdit		m_ceMesgID;

		 // Control Register
		CButton		    	m_cbCNTL_15;
		CButton		    	m_cbCNTL_14;
		CButton		    	m_cbCNTL_13;
		CButton		    	m_cbCNTL_12;
		CButton		    	m_cbCNTL_11;
		CButton		    	m_cbCNTL_10;
		CButton		    	m_cbCNTL_9;
		CButton		    	m_cbCNTL_8;
		CButton		    	m_cbCNTL_7;
		CButton		    	m_cbCNTL_6;
		CButton		    	m_cbCNTL_5;
		CButton		    	m_cbCNTL_4;
		CButton		    	m_cbCNTL_3;
		CButton		    	m_cbCNTL_2;
		CButton		    	m_cbCNTL_1;
		CButton		    	m_cbCNTL_0;

		// Status Register
		CButton		    	m_cbSTAT_15;
		CButton		    	m_cbSTAT_14;
		CButton		    	m_cbSTAT_13;
		CButton		    	m_cbSTAT_12;
		CButton		    	m_cbSTAT_11;
		CButton		    	m_cbSTAT_10;
		CButton		    	m_cbSTAT_9;
		CButton		    	m_cbSTAT_8;
		CButton		    	m_cbSTAT_7;
		CButton		    	m_cbSTAT_6;
		CButton		    	m_cbSTAT_5;
		CButton		    	m_cbSTAT_4;
		CButton		    	m_cbSTAT_3;
		CButton		    	m_cbSTAT_2;
		CButton		    	m_cbSTAT_1;
		CButton		    	m_cbSTAT_0;

		// Int Enable (Active) Register
		CButton		    	m_cbINTEN_15;
		CButton		    	m_cbINTEN_14;
		CButton		    	m_cbINTEN_13;
		CButton		    	m_cbINTEN_12;
		CButton		    	m_cbINTEN_11;
		CButton		    	m_cbINTEN_10;
		CButton		    	m_cbINTEN_9;
		CButton		    	m_cbINTEN_8;
		CButton		    	m_cbINTEN_7;
		CButton		    	m_cbINTEN_6;
		CButton		    	m_cbINTEN_5;
		CButton		    	m_cbINTEN_4;
		CButton		    	m_cbINTEN_3;
		CButton		    	m_cbINTEN_2;
		CButton		    	m_cbINTEN_1;
		CButton		    	m_cbINTEN_0;

		// Int ID Register
		CButton		    	m_cbINTID_15;
		CButton		    	m_cbINTID_14;
		CButton		    	m_cbINTID_13;
		CButton		    	m_cbINTID_12;
		CButton		    	m_cbINTID_11;
		CButton		    	m_cbINTID_10;
		CButton		    	m_cbINTID_9;
		CButton		    	m_cbINTID_8;
		CButton		    	m_cbINTID_7;
		CButton		    	m_cbINTID_6;
		CButton		    	m_cbINTID_5;
		CButton		    	m_cbINTID_4;
		CButton		    	m_cbINTID_3;
		CButton		    	m_cbINTID_2;
		CButton		    	m_cbINTID_1;
		CButton		    	m_cbINTID_0;

	    CButton			m_cbGenerate;
	    CComboBox		m_cbxData;

#if 0 //#ifndef BLOCK_MODE
	    CEdit			m_ceTransfer;
	    CButton			m_cbSend;
#endif
		CListBox		m_lbOutPackets;

		CButton			m_cbGetState;
	    CTraceWnd		m_wndTrace;

 	    //CButton			m_cbOK; //* Obsolete 

//* variables
//*
		short int		m_nMaxPaktLen;
	    BYTE			m_ucTermChar; 
		//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimulatorDlg)

	protected:
	    virtual void	DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	    //}}AFX_VIRTUAL

	// Implementation
	protected:
		etIntErrStyle   m_eIntErrStyle;

	    // The following are used to trace the register CHANGES
	    BYTE			m_ucCommandIn;
	    BYTE			m_ucCommandOut;
	    BYTE			m_ucStatusIn;
	    BYTE			m_ucStatusOut;

	    BOOL			m_bErr;

	    BOOL			m_bTrcDetail; 	  // TRUE if detailed trace is in effect
	    BOOL			m_bDisplayOutput; // TRUE if the device output is active
	    BOOL			m_bDisplayTrace;  // TRUE if the device output is active
		//BOOL			m_bBinaryData;    // TRUE if 8-bit data is 2 B expected/xmitted
	    BOOL			m_bManual;  	  // TRUE if in manual mode
	    //BOOL			m_bAutoInput;     // TRUE if in continuous input mode
	    //BOOL			m_bAutoOutput;    // TRUE if in continuous output mode
	    BOOL			m_bAutoDuplex;   // TRUE if in continuous output mode
		BOOL            m_bAutoSave;      // TRUE if saving output to file
        BOOL			m_bGenRandomPkts; 

		etRunMode		m_eRunMode;
	    etAutoInState	m_eAutoInState;

        int             m_hOutput;
		PVOID           m_pDeviceRegs;
        PVOID           m_pPioRead;
		PVOID           m_pPioWrite;
		PVOID           m_pDeviceData;

    //#ifdef DMA_MDL_SG_DRVR 
	//    ULONG           m_ulSGL_DX;
    //#endif //* DMA_MDL_SG_DRVR

#if 0 //#ifndef  BLOCK_MODE 
        FILE            *m_pFile;
#endif //* NOT BLOCK_MODE 
		
		//FILE*           m_pFile;
	    ULONG			m_nPktsSent;
	    ULONG			m_nPktsRecvd;
	    HICON			m_hIcon;
	    BOOL			m_bPolling;
	    HWND			m_hMainWnd; // required for cross-thread SendMessage
	    HANDLE			m_hFreeRun; // event handle used for free-run
	    HANDLE			m_hStop;    // interlock to keep manual transition from happening
                                 	// in middle of cycle
	    HANDLE			m_hPause;   // holds the polling thread until the message is
	                                // processed
    	DWORD			m_ulIRQ;
	    DWORD			m_ulGoDoneMin;
	    DWORD			m_ulGoDoneVariance;
	    DWORD			m_ulProbGenlErr;
	    DWORD			m_ulProbOvrUnd;
	    DWORD			m_ulProbLost;
	    DWORD			m_ulProbSpurious;
	    DWORD			m_ulProbDevBusy;
	    DWORD			m_ulPollIntrvl;
		LONG            m_nSerialNo;
        char            m_szOutFile[50];

		CString			m_csSaveFileName;
	    CString			m_csInput; // Current input string
	    CString			m_csOutput; // simulated output string if using GoHack stuff

#if 0 //#ifndef BLOCK_MODE //* Memory for the ReceiveNext function (char-mode)
		char            m_szOutPacket[250];
        int             m_nPktCount;
#endif

		IOCTL_Cls		m_IoCtlObj;

		CInterruptMgt*	m_pIntMngr;
	    CRegDisplay*	m_pRegDisp;
	    CWinThread*		m_pPollThread;

		void			Shutdown();
		void			UpdateDlgCntls(BOOL bValid = TRUE);
	    void			SetDoneOut(BOOL done, LPCTSTR why = NULL);
	    static UINT		Watcher(LPVOID me);
	    void			Poll();
	    void			DoTraceSave(BOOL mode);

	    void			LogControlChange(USHORT uOld, USHORT uNew, LPCTSTR reason = NULL);
		void			LogStatusChange(USHORT uOld, USHORT uNew, LPCTSTR reason = NULL);
	    void			LogIntEnableChange(USHORT uOld, USHORT uNew, LPCTSTR reason = NULL);
		void			LogIntIdChange(USHORT uOld, USHORT uNew, LPCTSTR reason = NULL);
	    void			LogChange(USHORT uOld, USHORT uNew, USHORT uStatus, UINT type,
		                          LPCTSTR reason = NULL);

		USHORT			Get_Control_DlgCntls();
	    USHORT			Get_Status_DlgCntls();
		USHORT			Get_IntEnable_DlgCntls();
	    USHORT			Get_IntID_DlgCntls();

	    void			PollInput();
	    void			PollOutput();
	    void			PollAutoInput();
	    void			PollAutoOutput();
	    
		void			GenerateAndLogInterrupt(LPCTSTR annotation = NULL);
		void			LogInterrupt(LPCTSTR annotation = NULL);
	    void			SetTransfer(CString& s);

#if 0 //#ifndef BLOCK_MODE
		BOOL			SendNext(BOOL bNext);
		void			ReceiveNext();
#endif

		void			ControlReg_2_DlgCntls(USHORT uControl);
	    void			StatusReg_2_DlgCntls(USHORT uStatus);
	    void			IntEnableReg_2_DlgCntls(USHORT IntEnable);
	    void			IntIdReg_2_DlgCntls(USHORT uIntId);
	    void			Set_DlgCntl(CButton& ctl, USHORT Uvalue, USHORT uMask);
	    void			LogErrorChange(CButton& ctl);
	    void			StartPollingCycle();
	    void			EndPollingCycle(BOOL bOutput = FALSE);
	    void			OpenRegisterTransaction(LPCTSTR where);
	    void			CloseRegisterTransaction(LPCTSTR where,
			                                     BOOL bOutput = FALSE);
	    void			SetRunMode(etRunMode eRunMode); 
	    void			Process_Output_Data(USHORT uStatus, BYTE ucData,
		                                    BOOL bClearHiBit); 
		void	        ClearInput();
	    void            ClearGenerate(); 
	    void	        ClearOutputWindow();  
	    //void	        ResetInputState(); 
	    //void	        ResetOutputState(); 
		void	        ResetDevice(); 
	    void	        ResetPlayDead(); 
	    void	        InitDeviceIn(); 
	    void	        InitDeviceOut(); 
	    void	        GetDeviceState();
	    //void	        GetOutputState();
		void	        ClearTrace();
		DWORD			GoDoneDelay();
		void			GeneratePacket(void);

//#ifdef BLOCK_MODE
		void            Set_Block_Input(char szData[]);
		PUCHAR          Get_Block_Output(void);
//#endif
		void            FileAccessHelperFunxn(BOOL bWrite);

		//USHORT         GetDlgMesgID (void){USHORT MesgIdAsc;char szMesgID[5]; m_ceMesgID.GetLine(0, szMesgID);MesgIdAsc=(SHORT)(szMesgID[0]);return (MesgIdAsc & 0x0F);} 
		
        #include "ErrorFunxns.h" 

	    // Generated message map functions
	    //{{AFX_MSG(CSimulatorDlg)
	    virtual BOOL	OnInitDialog();
	    afx_msg void	OnSysCommand(UINT nID, LPARAM lParam);
		afx_msg void	OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	    afx_msg void	OnDestroy();
	    afx_msg void	OnPaint();
	    afx_msg HCURSOR	OnQueryDragIcon();
	    virtual void	OnCancel();
	    virtual void	OnOK();

		#include "DlgMenu.h"   //*Dialog menu message handlers defined separately 
        //#include "Automatic.h"

//* Run Mode messages
//*		
		afx_msg void	OnManual();
	    //afx_msg void	OnAutoInput();
	    //afx_msg void	OnAutoOutput();
	    afx_msg void	OnAutoDuplex();

		afx_msg void	OnInt(); //* Issue Interrupt

//* Error injection messsages
//*
	    afx_msg void	OnXtraInts();
	    afx_msg void	OnRandErrs();
	    afx_msg void	OnOvrUndErrs();
	    afx_msg void	OnDevBusyErrs();
	    //afx_msg void	OnError();
	    afx_msg void	OnErrors();

	    afx_msg void	OnBusyOut();
	    afx_msg void	OnSelendokData();
	    afx_msg void	OnClose();
	    afx_msg void	OnSize(UINT nType, int cx, int cy);
	    //afx_msg void	OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	    afx_msg void	OnEditchangeData();
	    afx_msg void	OnTimer(UINT nIDEvent);

//* Device Register messages
//*
		afx_msg void	OnStatus_15();
		afx_msg void	OnStatus_14();
		afx_msg void	OnStatus_13();
		afx_msg void	OnStatus_12();
		afx_msg void	OnStatus_11();
		afx_msg void	OnStatus_10();
		afx_msg void	OnStatus_9();
		afx_msg void	OnStatus_8();
		afx_msg void	OnStatus_7();
	    afx_msg void	OnStatus_6();
	    afx_msg void	OnStatus_5();
	    afx_msg void	OnStatus_4();
	    afx_msg void	OnStatus_3();
	    afx_msg void	OnStatus_2();
	    afx_msg void	OnStatus_1();
	    afx_msg void	OnStatus_0();
		afx_msg void	OnIntID_15();
		afx_msg void	OnIntID_14();
		afx_msg void	OnIntID_13();
		afx_msg void	OnIntID_12();
		afx_msg void	OnIntID_11();
		afx_msg void	OnIntID_10();
		afx_msg void	OnIntID_9();
		afx_msg void	OnIntID_8();
		afx_msg void	OnIntID_7();
	    afx_msg void	OnIntID_6();
	    afx_msg void	OnIntID_5();
	    afx_msg void	OnIntID_4();
	    afx_msg void	OnIntID_3();
	    afx_msg void	OnIntID_2();
	    afx_msg void	OnIntID_1();
	    afx_msg void	OnIntID_0();

//* Data Input/Output messages
//*		
	    //afx_msg void	OnGenerate();
	    afx_msg void	OnAutoInPktEnd();

#if 0 //#ifndef BLOCK_MODE
		afx_msg void	OnSend();
#endif

//* Get device state messages
//
		afx_msg void	OnGetIOState();
	    //afx_msg void	OnGetInputState();
	    //afx_msg void	OnGetOutputState();

	    afx_msg void	OnComment(); //* Trace comment message

//* Assigned message id mesgs
//*
		afx_msg LRESULT	OnPulse(WPARAM, LPARAM);
	    afx_msg LRESULT	OnPoll(WPARAM, LPARAM);
	    afx_msg LRESULT	OnImgrClose(WPARAM, LPARAM);
	    afx_msg LRESULT	OnRegClose(WPARAM, LPARAM);
	    afx_msg LRESULT	OnOpenFailed(WPARAM, LPARAM);
	    afx_msg LRESULT	OnSetTimerOut(WPARAM, LPARAM);
	    afx_msg LRESULT	OnSetTimerIn(WPARAM, LPARAM);
	    //afx_msg LRESULT	OnExecuteGoHackIn(WPARAM, LPARAM);
	    //afx_msg LRESULT	OnExecuteGoHackOut(WPARAM, LPARAM);
	    afx_msg LRESULT	OnSetManualMode(WPARAM, LPARAM);
//		afx_msg LRESULT OnGenRandomPkts(WPARAM wParam, LPARAM lParam);
	    afx_msg LRESULT	OnUpdateProbabilities(WPARAM, LPARAM);
		//afx_msg LRESULT	OnAutoInput(WPARAM, LPARAM);
	    //afx_msg LRESULT	OnAutoOutput(WPARAM, LPARAM);
	    //}}AFX_MSG

    	// Nominally obsolete, redo as void w/o WPARAM, LPARAM
	    //afx_msg LRESULT	OnIACKInSet(WPARAM, LPARAM);
	    //afx_msg LRESULT	OnIACKOutSet(WPARAM, LPARAM);
	    //afx_msg LRESULT	IE_Out_Set(WPARAM, LPARAM);
	    //afx_msg LRESULT	IE_In_Set(WPARAM, LPARAM);
	    afx_msg LRESULT	OnUpdateDlgCntls(WPARAM, LPARAM);
		DECLARE_MESSAGE_MAP()
	};
