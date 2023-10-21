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
/*  Module  NAME : MadDevSimUIView.h                                           */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MadDevSimUIView class                     */
/*                                                                             */
/*******************************************************************************/


// MadDevSimUIView.h : interface of the CAbsDevUIView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ABSDEVUIVIEW_H__7000EDBE_E49A_4244_8C0B_67D8D2B8533E__INCLUDED_)
#define AFX_ABSDEVUIVIEW_H__7000EDBE_E49A_4244_8C0B_67D8D2B8533E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CAbsDevUIView: public CView
	{
	protected: // create from serialization only
					CAbsDevUIView();
					DECLARE_DYNCREATE(CAbsDevUIView)

	// Attributes
	public:
	CAbsDevUIDoc*	GetDocument();

	// Operations
	public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAbsDevUIView)
	public:
	virtual void	OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL	PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL	OnPreparePrinting(CPrintInfo* pInfo);
	virtual void	OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void	OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

	// Implementation
	public:
	virtual			~CAbsDevUIView();
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif

	protected:

	// Generated message map functions
	protected:
	//{{AFX_MSG(CAbsDevUIView)
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
					DECLARE_MESSAGE_MAP()
	};

#ifndef _DEBUG  // debug version in AbsDevUIView.cpp
inline CAbsDevUIDoc* CAbsDevUIView::GetDocument()

{
	return (CAbsDevUIDoc *) m_pDocument;
}
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ABSDEVUIVIEW_H__7000EDBE_E49A_4244_8C0B_67D8D2B8533E__INCLUDED_)
