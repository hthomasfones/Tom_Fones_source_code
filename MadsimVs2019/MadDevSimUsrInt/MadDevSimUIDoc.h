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
/*  Module  NAME : MadDevSimUIDoc.h                                            */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MadDevSimUIDoc class                      */
/*                                                                             */
/*******************************************************************************/


//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ABSDEVUIDOC_H__48A1E4C3_29C8_4A22_A0D5_AD3463776565__INCLUDED_)
#define AFX_ABSDEVUIDOC_H__48A1E4C3_29C8_4A22_A0D5_AD3463776565__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CAbsDevUIDoc : public CDocument
{
protected: // create from serialization only
	CAbsDevUIDoc();
	DECLARE_DYNCREATE(CAbsDevUIDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAbsDevUIDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAbsDevUIDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CAbsDevUIDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ABSDEVUIDOC_H__48A1E4C3_29C8_4A22_A0D5_AD3463776565__INCLUDED_)
