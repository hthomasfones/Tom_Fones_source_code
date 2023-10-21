
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
/*  Module  NAME : AutoExecDlg.h                                               */
/*                                                                             */
/*  DESCRIPTION  : Definition of the AutoExeDlg class                          */
/*                                                                             */
/*******************************************************************************/

//* This structure comminucates parameters between the parent &
//* child dialog boxes
//*
typedef struct _AUTOEXECUTE_PARMS
	{
    LONG    nPollInt;
    BOOL	bGenRandomPkts;
	} AUTOEXECUTE_PARMS, *PAUTOEXECUTE_PARMS;

/////////////////////////////////////////////////////////////////////////////
// CSetupDlg dialog

class CSetupDlg: public CDialog
	{
	// Construction
	public:
	    CSetupDlg(CWnd* pParent = NULL);   // standard constructor

	    UINT  DoModal(PAUTOEXECUTE_PARMS pAutoExecParms); 

		// Dialog Data
	    //{{AFX_DATA(CSetupDlg)
	    enum {IDD = IDD_SETUP};
	    CSpinButtonCtrl	c_SpinGoDoneVariance;
	    CSpinButtonCtrl	c_SpinGoDoneMin;
	    CEdit			c_GoDoneVariance;
	    CEdit			c_GoDoneMin;
	    CSpinButtonCtrl	c_SpinPollingInterval;
	    CEdit			c_PollingInterval;
		CButton         m_cbRandomPkts;
	    CButton			c_OK;
	    CComboBox		c_IRQ;
	    //int				m_IRQ;
		LONG            m_nPollIntrvl;
		BOOL            m_bGenRndmPkts;
	    //}}AFX_DATA


	    // Overrides
	    // ClassWizard generated virtual function overrides
	    //{{AFX_VIRTUAL(CSetupDlg)

	protected:
	    virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
	protected:
	    void			UpdateSetupControls();

	    // Generated message map functions
	    //{{AFX_MSG(CSetupDlg)
	    //afx_msg void	OnSelendokIrq();
	    //virtual void	OnOK();
	    //virtual void	OnRandomPkts();
	    //virtual BOOL	OnInitDialog();
	    afx_msg void	OnHelp();
	    //}}AFX_MSG

		DECLARE_MESSAGE_MAP()
	};
