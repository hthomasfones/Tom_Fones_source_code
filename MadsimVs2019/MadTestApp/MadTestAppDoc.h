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
/*  Module  NAME : MadDevIoDoc.h                                               */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class MadDevIoDoc                     */
/*                                                                             */
/*******************************************************************************/


// MadDevIoDoc.h : interface of the CMadDevIoDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ABSDEVIODOC_H__7F4E6B4B_C4E0_4A95_809A_666F3FED4127__INCLUDED_)
#define AFX_ABSDEVIODOC_H__7F4E6B4B_C4E0_4A95_809A_666F3FED4127__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CMadDevIoDoc : public CDocument
{
protected: // create from serialization only
	CMadDevIoDoc();
	DECLARE_DYNCREATE(CMadDevIoDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMadDevIoDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMadDevIoDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CMadDevIoDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ABSDEVIODOC_H__7F4E6B4B_C4E0_4A95_809A_666F3FED4127__INCLUDED_)
