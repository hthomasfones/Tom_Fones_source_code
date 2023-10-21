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
/*  Module  NAME : RegDisplay.h                                                */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes, structures, classes, etc.              */
/*                                                                             */
/*******************************************************************************/

// RegDisplay.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRegDisplay dialog

class CRegDisplay: public CDialog
	{
	// Construction
	public:
	CRegDisplay(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CRegDisplay)
	enum {IDD = IDD_REGISTERS};
	CEdit    		m_ceMesgID;
	CEdit    		m_ceControl;
	CEdit    		m_ceStatus;
	CEdit   		m_ceIntEnable;
	CEdit    		m_ceIntID;
	CEdit    		m_cePioCacheReadLen;
	CEdit    		m_cePioCacheWriteLen;

	//CRegData		m_cData1;
	//CRegData		m_cData2;
	//CRegData		m_cData3;
	CRegData		m_cPowerState;

	//CEdit           m_ceInDMA_Hi;
	//CEdit           m_ceInDMA_Lo;
	//CEdit           m_ceOutDMA_Hi;
	//CEdit           m_ceOutDMA_Lo;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegDisplay)
	protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
	protected:

	// Generated message map functions
	//{{AFX_MSG(CRegDisplay)
	virtual BOOL	OnInitDialog();
	afx_msg void	OnClose();
	afx_msg void	OnOK();
	afx_msg void	OnCancel();
	afx_msg void	OnDestroy();
	afx_msg LRESULT	OnUpdateRegs(WPARAM, LPARAM);
	//}}AFX_MSG
					DECLARE_MESSAGE_MAP()
	public:
		int MesgID;
};
