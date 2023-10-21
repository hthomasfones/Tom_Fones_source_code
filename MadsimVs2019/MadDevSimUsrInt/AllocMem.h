/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Meta-Abstract Device Simulation Subsystem                   */
/*  COPYRIGHT    : (c) 2004 HTF CONSULTING                                     */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadDevSimUI.exe                                             */
/*                                                                             */
/*  Module  NAME : AllocMem.h                                                  */
/*  DESCRIPTION  : Function prototypes, etc.                                   */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/

#if !defined(AFX_ALLOCMEM_H__39B852D9_E14E_42D9_84A2_374A882DFB50__INCLUDED_)
#define AFX_ALLOCMEM_H__39B852D9_E14E_42D9_84A2_374A882DFB50__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AllocMem.h : header file
//

typedef struct _MEM_ALLOC_DLG_PARMS
	{
	LONG	nNumPages;
	ULONG	ulMemAllocFlags;
	} MEM_ALLOC_DLG_PARMS,* PMEM_ALLOC_DLG_PARMS;

/////////////////////////////////////////////////////////////////////////////
// CAllocMem dialog

class CAllocMem: public CDialog
	{
	// Construction
	public:
			CAllocMem(CWnd* pParent = NULL);   // standard constructor
	UINT	DoModal(PMEM_ALLOC_DLG_PARMS pMemAllocDlgParms); 

	// Dialog Data
	//{{AFX_DATA(CAllocMem)
	enum { IDD = IDD_AllocMemory };
	long	m_nNumPages;
	BOOL	m_bDmaIn;
	BOOL	m_bDmaOut;
	BOOL	m_bShared;
	BOOL	m_bContig;
	BOOL	m_bLocked;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAllocMem)
	protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
	protected:

	// Generated message map functions
	//{{AFX_MSG(CAllocMem)
	virtual void	OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALLOCMEM_H__39B852D9_E14E_42D9_84A2_374A882DFB50__INCLUDED_)
