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
/*  Module  NAME : RegData.h                                                   */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes, structures, classes, etc.              */
/*                                                                             */
/*******************************************************************************/

// RegData.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRegData window

class CRegData: public CEdit
	{
	// Construction
	public:
					CRegData();

	// Attributes
	public:

	// Operations
	public:
	void			SetWindowText(BYTE value);
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegData)
	//}}AFX_VIRTUAL

	// Implementation
	public:
	virtual			~CRegData();

	// Generated message map functions
	protected:
	//{{AFX_MSG(CRegData)
	//afx_msg void	OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

					DECLARE_MESSAGE_MAP()
	};

/////////////////////////////////////////////////////////////////////////////
