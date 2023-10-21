/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2013 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* XYZ Company                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadBus.sys, MadDevice.sys, MadSimUI.exe, MadTestApp.exe,    */
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : Madxxxxxxxxx.cpp                                            */
/*                                                                             */
/*  DESCRIPTION  : Read Registry functions for any Functional device driver    */
/*                 working with a simulation                                   */
/*                 Drived from WDK-Toaster\bus\bus.c                           */
/*                                                                             */
/*******************************************************************************/


#if !defined(AFX_DEALLOC_H__7FF8FAE1_A1D8_48F0_938E_BBC9317015D0__INCLUDED_)
#define AFX_DEALLOC_H__7FF8FAE1_A1D8_48F0_938E_BBC9317015D0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DeAlloc.h : header file
//

typedef struct _MEM_RLS_BOOLS
	{
    BOOL	bDmaIn;
    BOOL	bDmaOut;
	BOOL	bShared;
	BOOL	bContig;
	BOOL	bLocked;
	BOOL	bPaged;
	} MEM_RLS_BOOLS,* PMEM_RLS_BOOLS;

/////////////////////////////////////////////////////////////////////////////
// DeAlloc dialog

class DeAlloc: public CDialog
	{
	// Construction
	public:
		      DeAlloc(CWnd* pParent = NULL);   // standard constructor
	    UINT  DoModal(PMEM_RLS_BOOLS pMemRlsBools); 

		// Dialog Data
	    //{{AFX_DATA(DeAlloc)
	enum { IDD = IDD_DeallocMemory };
	    BOOL	m_bDmaIn;
	    BOOL	m_bDmaOut;
	    BOOL	m_bShared;
	    BOOL	m_bContig;
	    BOOL	m_bLocked;
	    BOOL	m_bPaged;
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DeAlloc)
	protected:
    	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
	protected:

	    // Generated message map functions
	    //{{AFX_MSG(DeAlloc)
	    virtual void	OnOK();
	    //}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEALLOC_H__7FF8FAE1_A1D8_48F0_938E_BBC9317015D0__INCLUDED_)
