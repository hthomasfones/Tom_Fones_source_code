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
/*  Module  NAME : ErrorMngt.h                                                 */
/*                                                                             */
/*  DESCRIPTION  :                                                             */
/*                                                                             */
/*******************************************************************************/

// InterruptMgt.h : header file
//
typedef enum {eNoIntErrs = 0, eLoseInts, eXtraInts, eBothErrs} etIntErrStyle; 


/////////////////////////////////////////////////////////////////////////////
// CInterruptMgt dialog

class CInterruptMgt: public CDialog
	{
	// Construction
	public:
	    CInterruptMgt(CWnd* pParent = NULL);   // standard constructor

		UINT  DoModal(etIntErrStyle* peIntErrStyle); 

	    // Dialog Data
	    //{{AFX_DATA(CInterruptMgt)
	    enum {IDD = IDD_INTERRUPTS};
	    CButton			c_c_ProbErr;
	    CButton			c_None;
		CButton			c_Lose;
	    CButton			c_Spurious;
	    CButton			c_Both;

	    CNumericEdit	c_ProbLose;
	    CNumericEdit	c_ProbSpurious;
	    CNumericEdit	c_ProbOvr;
	    CNumericEdit	c_ProbErr;
	    CNumericEdit	c_ProbDevBusy;

	    CSpinButtonCtrl	c_SpinSpurious;
	    CSpinButtonCtrl	c_SpinOvr;
	    CSpinButtonCtrl	c_SpinErr;
	    CSpinButtonCtrl	c_SpinLose;
	    CSpinButtonCtrl	c_SpinTimeout;
	    //}}AFX_DATA

	    // Overrides
	    // ClassWizard generated virtual function overrides
	    //{{AFX_VIRTUAL(CInterruptMgt)

	protected:
	    virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	    virtual void	PostNcDestroy();
	    //}}AFX_VIRTUAL

	// Implementation
	protected:
	    void			UpdateControls();
	    BOOL			m_bInit;
		etIntErrStyle   m_eIntErrStyle;

	    // Generated message map functions
	    //{{AFX_MSG(CInterruptMgt)
	    virtual BOOL	OnInitDialog();
		afx_msg void    OnNoIntErrs();
	    afx_msg void	OnSpurious();
	    afx_msg void	OnLose();
	    afx_msg void	OnBoth();
	    afx_msg void	OnChangePlose();
	    afx_msg void	OnChangePspurious();
	    afx_msg void	OnChangePerr();
	    afx_msg void	OnChangePovr();
	    afx_msg void	OnChangePDevBusy(); 
	    afx_msg void	OnHelp();
    	afx_msg void	OnDestroy();
	    afx_msg void	OnCloseDialog();
	    //}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
