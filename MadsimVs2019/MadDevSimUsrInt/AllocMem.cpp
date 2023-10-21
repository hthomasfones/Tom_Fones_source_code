/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Meta-Abstract Device Simulation Subsystem                   */
/*  COPYRIGHT    : (c) 2004 HTF CONSULTING                                     */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadDevSimUI.exe                                             */
/*                                                                             */
/*  Module  NAME : AllocMem.cpp                                                */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/

// AllocMem.cpp : implementation file
//

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "MadDefinition.h" 
#include "MadBusUsrIntBufrs.h"
#include "AllocMem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAllocMem dialog


CAllocMem::CAllocMem(CWnd* pParent /*=NULL*/)
	: CDialog(CAllocMem::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAllocMem)
	m_nNumPages = 0;
	m_bDmaIn  = FALSE;
	m_bDmaOut = FALSE;
	m_bShared = FALSE;
	m_bContig = FALSE;
	m_bLocked = FALSE;
	//}}AFX_DATA_INIT
}


void CAllocMem::DoDataExchange(CDataExchange* pDX)

{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAllocMem)
	DDX_Text(pDX, IDC_Numpages, m_nNumPages);
	DDV_MinMaxLong(pDX, m_nNumPages, 1, 2000000);
	DDX_Check(pDX, IDC_DmaIn,  m_bDmaIn);
	DDX_Check(pDX, IDC_DmaOut, m_bDmaOut);
	DDX_Check(pDX, IDC_Shared, m_bShared);
	DDX_Check(pDX, IDC_Contig, m_bContig);  
	DDX_Check(pDX, IDC_Locked, m_bLocked);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAllocMem, CDialog)
//{{AFX_MSG_MAP(CAllocMem)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//* Overriding DoModal member function to pass parameters
//*
UINT CAllocMem::DoModal(PMEM_ALLOC_DLG_PARMS pMemAllocDlgParms)

{
UINT nDM;
ULONG ulMemAllocFlags = 0;

	m_nNumPages = 0;
	m_bDmaIn  = FALSE;
	m_bDmaOut = FALSE;
	m_bShared = FALSE;
	m_bContig = FALSE;
	m_bLocked = FALSE;

	nDM = CDialog::DoModal(); //Invoke Base member funxn 1st
	if (nDM == IDCANCEL)
		return IDCANCEL;

	if (m_bDmaIn)
		ulMemAllocFlags = ulMemAllocFlags | MAF_DMAIN;

	if (m_bDmaOut)
		ulMemAllocFlags = ulMemAllocFlags | MAF_DMAOUT;

	if (m_bShared)
		ulMemAllocFlags = ulMemAllocFlags | MAF_SHARED;

	if (m_bContig)
		ulMemAllocFlags = ulMemAllocFlags | MAF_CONTIG;

	if (m_bLocked)
		ulMemAllocFlags = ulMemAllocFlags | MAF_LOCKED;

	pMemAllocDlgParms->ulMemAllocFlags = ulMemAllocFlags;
	pMemAllocDlgParms->nNumPages = m_nNumPages;

	return nDM;
} 


/////////////////////////////////////////////////////////////////////////////
// CAllocMem message handlers

void CAllocMem::OnOK()

{
	CDialog::OnOK();
}
