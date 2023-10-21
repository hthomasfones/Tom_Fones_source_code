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

// ReleaseMem.cpp : implementation file
//

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "ReleaseMem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DeAlloc dialog


DeAlloc::DeAlloc(CWnd* pParent /*=NULL*/)
	: CDialog(DeAlloc::IDD, pParent)
{
	//{{AFX_DATA_INIT(DeAlloc)
    m_bDmaIn  = FALSE;
    m_bDmaOut = FALSE;
	m_bShared = FALSE;
	m_bContig = FALSE;
	m_bLocked = FALSE;
	m_bPaged = FALSE;
	//}}AFX_DATA_INIT
}


void DeAlloc::DoDataExchange(CDataExchange* pDX)

{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DeAlloc)
	DDX_Check(pDX, IDC_DA_Shared, m_bShared);
	DDX_Check(pDX, IDC_DA_Contig, m_bContig);
	DDX_Check(pDX, IDC_DA_Locked, m_bLocked);
	DDX_Check(pDX, IDC_DA_Paged,  m_bPaged);
	DDX_Check(pDX, IDC_DA_DmaIn,  m_bDmaIn);
	DDX_Check(pDX, IDC_DA_DmaOut, m_bDmaOut);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(DeAlloc, CDialog)
//{{AFX_MSG_MAP(DeAlloc)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//* Overriding DoModal member function to pass parameters
//*
UINT DeAlloc::DoModal(PMEM_RLS_BOOLS pMemRlsBools)

{
UINT nDM;

	nDM = CDialog::DoModal(); //Invoke Base member funxn 1st
	if (nDM == IDCANCEL)
		return IDCANCEL;

	pMemRlsBools->bDmaIn  = m_bDmaIn;
	pMemRlsBools->bDmaOut = m_bDmaOut;
	pMemRlsBools->bShared = m_bShared;
	pMemRlsBools->bContig = m_bContig;
	pMemRlsBools->bLocked = m_bLocked;
	pMemRlsBools->bPaged  = m_bPaged;

	return nDM;
} 


/////////////////////////////////////////////////////////////////////////////
// DeAlloc message handlers


//****
//*
void DeAlloc::OnOK()

{
	CDialog::OnOK();

	return;
}
