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
/*  Module  NAME : Hdwtrace.h                                                  */
/*                                                                             */
/*  DESCRIPTION  : Definition of the HdwTrace class                            */
/*                                                                             */
/*******************************************************************************/


// HdwTrace.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHdwTrace dialog

class CHdwTrace: public CDialog
	{
	// Construction
	public:
	    CHdwTrace(CWnd* pParent = NULL);   // standard constructor

	    // Dialog Data
	    //{{AFX_DATA(CHdwTrace)
	    enum {IDD = IDD_HDWTRACE};
	    CButton			c_IoctlTrace;
	    CButton			c_RegTrace;
	    CButton			c_IntTrace;
	    CButton			c_OK;
	    //}}AFX_DATA

	    IOCTL_Cls*		m_pIoCtlObj;
	    // Overrides
	    // ClassWizard generated virtual function overrides
	    //{{AFX_VIRTUAL(CHdwTrace)

	protected:
	    virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	    //}}AFX_VIRTUAL

	// Implementation
	protected:
    	// Generated message map functions
	    //{{AFX_MSG(CHdwTrace)
	    virtual void	OnOK();
	    virtual BOOL	OnInitDialog();
	    //}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
