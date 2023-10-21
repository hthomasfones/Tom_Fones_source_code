// MadDevIoView.h : interface of the CMadDevIoView class
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
/*  Module  NAME : MadTestAppView.h                                            */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class MadTestIoView                   */
/*                                                                             */
/*******************************************************************************/

//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ABSDEVIOVIEW_H__2C968A73_D337_4EF3_BAD6_D223B6EB1644__INCLUDED_)
#define AFX_ABSDEVIOVIEW_H__2C968A73_D337_4EF3_BAD6_D223B6EB1644__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#include "GenDrivrTestVuParms.h" //Replaces below

#define CLIENTWIDTH         350
#define CLIENTHEIGHT        400
#define MAXTEXTLINES        250
#define MAXTEXTLEN          150
#define STDTEXTHEIGHT        20
#define STDTEXTWIDTH         10
#define MAXVERTSCROLLRANGE  (MAXTEXTLINES * STDTEXTHEIGHT)
#define MAXHORZSCROLLRANGE  (STDTEXTWIDTH * 100)

#include "MadTestAppDoc.h"

class CMadDevIoView : public CView
{
protected: // create from serialization only
	CMadDevIoView();
	DECLARE_DYNCREATE(CMadDevIoView)

// Attributes
public:
	CMadDevIoDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMadDevIoView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMadDevIoView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	//afx_msg void OnPaint();

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CMadDevIoView)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in MadDevIoView.cpp
inline CMadDevIoDoc* CMadDevIoView::GetDocument()
   { return (CMadDevIoDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ABSDEVIOVIEW_H__2C968A73_D337_4EF3_BAD6_D223B6EB1644__INCLUDED_)
