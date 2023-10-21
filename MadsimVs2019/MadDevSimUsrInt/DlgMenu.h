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
/*  Module  NAME : DlgMenu.h                                                   */
/*                                                                             */
/*  DESCRIPTION  : Class definition for the dialog menu                        */
/*                                                                             */
/*******************************************************************************/

 // Generated message map functions for the menu in the Dlg box
 //{{AFX_MSG(CSimulatorDlg)
//* File Menu
//*
	    afx_msg void	OnOpenDevice(); 
	    afx_msg void	OnOutputSave();
	    afx_msg void	OnOutputAutoSave();
	    afx_msg void	OnTraceSave();
	    afx_msg void	OnTraceSaveAs();
	    afx_msg void	OnFileExit();
		afx_msg void    OnFileSaveDeviceData();
		afx_msg void    OnFileRetrieveDeviceData();
		afx_msg void	OnFileMapDeviceRegs();

//* Parameters Menu
//*
	    //afx_msg void	OnSetupPacketTermination();
	    //afx_msg void	OnSetupIrq();
	    afx_msg void	OnIntErrMgt();
  
//* View menu
//*
	    afx_msg void	OnViewOutputDisplay(); 
	    //afx_msg void	OnSetupBinaryData(); 
	    afx_msg void	OnViewTraceDisplay(); 
		//afx_msg void	OnDebug();
	    afx_msg void	OnRegDisp();
	    afx_msg void	OnHdwsimTrace();

//* Windows Menu
//*
	    afx_msg void	OnClearTrace();
#if 0 //ndef BLOCK_MODE
    	afx_msg void	OnClearInput();
#endif
	    //afx_msg void    OnClearGenerate(); 
	    //afx_msg void	OnClearOutputWindow();  
	    //afx_msg void	OnResetInputState(); 
	    //afx_msg void	OnResetOutputState(); 

//* Reset menu
//*
		afx_msg void	OnResetDevice(); 
		afx_msg void    OnResetPlayDead();
        afx_msg void	OnResetInitDriver();  

//* Memory menu
//*
	    //afx_msg void	OnAllocMemory();	 
	    //afx_msg void	OnReleaseMemory();     

//* Help
//*
		afx_msg void	OnAppAbout();
	    afx_msg void	OnHelpIndex();
	    //}}AFX_MSG


