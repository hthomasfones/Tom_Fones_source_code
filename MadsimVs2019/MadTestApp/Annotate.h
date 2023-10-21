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
/*  Exe file ID  : MadTestApp.exe                                              */
/*                                                                             */
/*  Module  NAME : Annotate.h                                                  */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class Annotate                        */
/*                                                                             */
/*******************************************************************************/


#if !defined(AFX_ANNOTATE_H__881E21FB_6C83_4E9A_9C7B_3AED485691F4__INCLUDED_)
#define AFX_ANNOTATE_H__881E21FB_6C83_4E9A_9C7B_3AED485691F4__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Annotate.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAnnotate dialog

class CAnnotate : public CDialog
{
// Construction
public:
	CAnnotate(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAnnotate)
	enum { IDD = IDD_Annotate };
	CString	m_csAnnotateText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAnnotate)
	public:
	virtual int DoModal(PCHAR pChar);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAnnotate)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ANNOTATE_H__881E21FB_6C83_4E9A_9C7B_3AED485691F4__INCLUDED_)
