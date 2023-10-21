// SetRegBits.cpp : implementation file
//

#include "stdafx.h"
#include "MadTestApp.h"
#include "..\Includes\MadDefinition.h"
#include "SetRegBits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSetRegBits dialog


CSetRegBits::CSetRegBits(CWnd* pParent /*=NULL*/)
	: CDialog(CSetRegBits::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSetRegBits)
	m_bBIT0 = FALSE;
	m_bBIT1 = FALSE;
	m_bBIT2 = FALSE;
	m_bBIT3 = FALSE;
	m_bBIT4 = FALSE;
	m_bBIT5 = FALSE;
	m_bBIT6 = FALSE;
	m_bBIT7 = FALSE;
	m_bBIT8 = FALSE;
	m_bBIT9 = FALSE;
	m_bBIT10 = FALSE;
	m_bBIT11 = FALSE;
	m_bBIT12 = FALSE;
	m_bBIT13 = FALSE;
	m_bBIT14 = FALSE;
	m_bBIT15 = FALSE;
	//}}AFX_DATA_INIT
}


void CSetRegBits::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSetRegBits)
	DDX_Check(pDX, IDC_BIT0,  m_bBIT0);
	DDX_Check(pDX, IDC_BIT1,  m_bBIT1);
	DDX_Check(pDX, IDC_BIT2,  m_bBIT2);
	DDX_Check(pDX, IDC_BIT3,  m_bBIT3);
	DDX_Check(pDX, IDC_BIT4,  m_bBIT4);
	DDX_Check(pDX, IDC_BIT5,  m_bBIT5);
	DDX_Check(pDX, IDC_BIT6,  m_bBIT6);
	DDX_Check(pDX, IDC_BIT7,  m_bBIT7);
	DDX_Check(pDX, IDC_BIT8,  m_bBIT8);
	DDX_Check(pDX, IDC_BIT9,  m_bBIT9);
	DDX_Check(pDX, IDC_BIT10, m_bBIT10);
	DDX_Check(pDX, IDC_BIT11, m_bBIT11);
	DDX_Check(pDX, IDC_BIT12, m_bBIT12);
	DDX_Check(pDX, IDC_BIT13, m_bBIT13);
	DDX_Check(pDX, IDC_BIT14, m_bBIT14);
	DDX_Check(pDX, IDC_BIT15, m_bBIT15);
	//}}AFX_DATA_MAP

	//MessageBox("CSetRegBits::DoDataExchange", NULL, MB_OK);

	return;
}

BEGIN_MESSAGE_MAP(CSetRegBits, CDialog)
	//{{AFX_MSG_MAP(CSetRegBits)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetRegBits message handlers

int CSetRegBits::DoModal(SET_REG_PARMS* pSetRegParms) 

{
char szDlgTitl[50] = "Set MAD Register: ";
UINT uRC;
ULONG RegBits = pSetRegParms->RegBits;

    //MessageBox("CSetRegBits::DoModal", NULL, MB_OK);

//* DoDataExchange fails unless these boolean values have their 'TRUE' bit 
//* as the low-order bit - ergo the bit-shifting
//*
    m_bBIT0  = (BOOL)((RegBits & MADMASK_BIT0));
    m_bBIT1  = (BOOL)((RegBits & MADMASK_BIT1)  != 0);
    m_bBIT2  = (BOOL)((RegBits & MADMASK_BIT2)  != 0);
    m_bBIT3  = (BOOL)((RegBits & MADMASK_BIT3)  != 0);
    m_bBIT4  = (BOOL)((RegBits & MADMASK_BIT4)  != 0);
    m_bBIT5  = (BOOL)((RegBits & MADMASK_BIT5)  != 0);
    m_bBIT6  = (BOOL)((RegBits & MADMASK_BIT6)  != 0);
    m_bBIT7  = (BOOL)((RegBits & MADMASK_BIT7)  != 0);
    m_bBIT8  = (BOOL)((RegBits & MADMASK_BIT8)  != 0);
    m_bBIT9  = (BOOL)((RegBits & MADMASK_BIT9)  != 0);
    m_bBIT10 = (BOOL)((RegBits & MADMASK_BIT10) != 0);
    m_bBIT11 = (BOOL)((RegBits & MADMASK_BIT11) != 0);
    m_bBIT12 = (BOOL)((RegBits & MADMASK_BIT12) != 0);
    m_bBIT13 = (BOOL)((RegBits & MADMASK_BIT13) != 0);
    m_bBIT14 = (BOOL)((RegBits & MADMASK_BIT14) != 0);
    m_bBIT15 = (BOOL)((RegBits & MADMASK_BIT15) != 0);

	strcat_s(szDlgTitl, 50, pSetRegParms->szTitlSufx);
    //SetWindowText(szDlgTitl);

	uRC = (UINT)CDialog::DoModal();
    if (uRC == IDCANCEL)
		return uRC;

    RegBits = 0x0000;

    if (m_bBIT0)
		RegBits |= MADMASK_BIT0;

    if (m_bBIT1)
		RegBits |= MADMASK_BIT1;

    if (m_bBIT2)
		RegBits |= MADMASK_BIT2;

    if (m_bBIT3)
		RegBits |= MADMASK_BIT3;

    if (m_bBIT4)
		RegBits |= MADMASK_BIT4;

    if (m_bBIT5)
		RegBits |= MADMASK_BIT5;

    if (m_bBIT6)
		RegBits |= MADMASK_BIT6;

    if (m_bBIT7)
		RegBits |= MADMASK_BIT7;

    if (m_bBIT8)
		RegBits |= MADMASK_BIT8;

    if (m_bBIT9)
		RegBits |= MADMASK_BIT9;

    if (m_bBIT10)
		RegBits |= MADMASK_BIT10;

    if (m_bBIT11)
		RegBits |= MADMASK_BIT11;

    if (m_bBIT12)
		RegBits |= MADMASK_BIT12;

    if (m_bBIT13)
		RegBits |= MADMASK_BIT13;

    if (m_bBIT14)
		RegBits |= MADMASK_BIT14;

    if (m_bBIT15)
		RegBits |= MADMASK_BIT15;

	pSetRegParms->RegBits = RegBits;

	//MessageBox("CSetRegBits::DoModal exit", NULL, MB_OK);

    return IDOK;
}


void CSetRegBits::OnOK() 

{
	CDialog::OnOK();
	//MessageBox("CSetRegBits::OnOK", NULL, MB_OK);

    return;
}



void CSetRegBits::OnCancel() 

{
	CDialog::OnCancel();

    return;
}



