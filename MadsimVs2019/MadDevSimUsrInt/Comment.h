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
/*  Module  NAME : Comment.h                                                   */
/*                                                                             */
/*  DESCRIPTION  : Comment class definition                                    */
/*                                                                             */
/*******************************************************************************/

// Comment.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CComment dialog

class CComment: public CDialog
	{
	// Construction
	public:
	    CComment(CWnd* pParent = NULL);   // standard constructor

	    // Dialog Data
	    //{{AFX_DATA(CComment)
	    enum {IDD = IDD_COMMENT};
	    CEdit			c_Comment;
	    CButton			c_OK;
	    CString			m_Comment;
	    //}}AFX_DATA

	    // Overrides
	    // ClassWizard generated virtual function overrides
	    //{{AFX_VIRTUAL(CComment)
	protected:
	    virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	    //}}AFX_VIRTUAL

	// Implementation
	protected:
	    void UpdateDlgCntls();

	    // Generated message map functions
	    //{{AFX_MSG(CComment)
	    afx_msg void	OnChangeComment();
	    virtual BOOL	OnInitDialog();
	    //}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
