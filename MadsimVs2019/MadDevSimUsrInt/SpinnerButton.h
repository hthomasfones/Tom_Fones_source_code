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
/*  Module  NAME : SpinnerButton.h                                             */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes, structures, classes, etc.              */
/*                                                                             */
/*******************************************************************************/

// SpinnerButton.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpinnerButton window

class CSpinnerButton: public CButton
	{
	// Construction
	public:
	CSpinnerButton();
	
	void Pulse()

	{
		pos = (pos + 1) % 8; InvalidateRect(NULL);
	}


	void SetMode(BOOL m)

	{
		mode = m;
	}

	BOOL GetMode()

	{
		return mode;
	}
	// Attributes
	public:

	// Operations
	public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpinnerButton)
	public:
	virtual void	DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

	// Implementation
	public:
	virtual			~CSpinnerButton();

	// Generated message map functions
	protected:
	UINT			pos;
	BOOL			mode;

	static CBrush	green;
	static CBrush	red;

	//{{AFX_MSG(CSpinnerButton)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	};

/////////////////////////////////////////////////////////////////////////////
