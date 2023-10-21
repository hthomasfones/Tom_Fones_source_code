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
/*  Module  NAME : NumericEdit.h                                               */
/*                                                                             */
/*  DESCRIPTION  : Class definitions function prototypes                       */
/*                 working with a simulation                                   */
/*                 Derived from WDK-Toaster\bus\bus.c                          */
/*                                                                             */
/*******************************************************************************/

// NumericEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// NumericEdit window

class CNumericEdit: public CEdit
	{
	// Construction
	public:
			CNumericEdit();
			CNumericEdit(LPCTSTR fmt);
	void	SetWindowText(int val, LPCTSTR fmt = NULL);
	int		GetWindowInt();
	void	Blank();

	// Attributes
	public:

	// Operations
	public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNumericEdit)
	//}}AFX_VIRTUAL

	// Implementation
	public:
	virtual	~CNumericEdit();

	// Generated message map functions
	protected:
	LPCTSTR	deffmt;

	//{{AFX_MSG(CNumericEdit)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

			DECLARE_MESSAGE_MAP()
	};

/////////////////////////////////////////////////////////////////////////////
