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
/*  Module  NAME : PacketTerm.cpp                                              */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "PacketTerm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPacketTerm dialog


CPacketTerm::CPacketTerm(CWnd* pParent /*=NULL*/)
	: CDialog(CPacketTerm::IDD, pParent)
{
short int iTemp;
char szNum[10];
	//PCHAR pChar;

	//{{AFX_DATA_INIT(CPacketTerm)
	m_nMaxLen = 9999;
	m_ucTermChar = 0x00;

	_itoa_s(m_nMaxLen, szNum, 10, 10);
	m_csMaxLen = szNum;

	memset(szNum, 0x00, 10);
	iTemp = (short int)(m_ucTermChar / 16);
	Num2HexDigit(iTemp, & szNum[0]);
	iTemp = (short int)(m_ucTermChar % 16);
	Num2HexDigit(iTemp, & szNum[1]);
	m_csTermChar = szNum;
	//}}AFX_DATA_INIT
}


void CPacketTerm::DoDataExchange(CDataExchange* pDX)

{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPacketTerm)
	DDX_Text(pDX, IDC_MaxLen, m_csMaxLen);
	DDV_MaxChars(pDX, m_csMaxLen, 4);
	DDX_Text(pDX, IDC_TermChar, m_csTermChar);
	DDV_MaxChars(pDX, m_csTermChar, 2);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPacketTerm, CDialog)
//{{AFX_MSG_MAP(CPacketTerm)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CPacketTerm::OnInitDialog()

{
short int iTemp;
char szNum[10];

	_itoa_s(m_nMaxLen, szNum,  10, 10);
	m_csMaxLen = szNum;

	memset(szNum, 0x00, 10);
	iTemp = (short int)(m_ucTermChar / 16);
	Num2HexDigit(iTemp, & szNum[0]);
	iTemp = (short int)(m_ucTermChar % 16);
	Num2HexDigit(iTemp, & szNum[1]);
	m_csTermChar = szNum;

	CDialog::OnInitDialog();

	return TRUE;
}


//* Overriding DoModal member function to return an int
//*
UINT CPacketTerm::DoModal(struct _PacketTermParms* pPaktTermParms)

{
UINT nDM;
short int nLen;
short int nHex1, nHex2;
char szTempNum[3] = "";
char* pChar = &szTempNum[0];

	m_nMaxLen = pPaktTermParms->nMaxLen;
	m_ucTermChar = pPaktTermParms->ucTermChar;

	nDM = (UINT)CDialog::DoModal(); //Invoke Base member funxn 1st
	if (nDM == IDCANCEL)
		return IDCANCEL;

	//* OK button clicked - Let's collect the numbers
	//*
	nLen = (short int)m_csMaxLen.GetLength();
	if (nLen > 0)
		pPaktTermParms->nMaxLen = (short int)atoi(m_csMaxLen.GetBuffer(1));

	nLen = (short int)m_csTermChar.GetLength();
	if (nLen == 0)
		return nDM;

	//* Convert 2 hex digits to an unsigned 8-bit quantity
	//*    
	strcpy_s(szTempNum, 3, m_csTermChar.GetBuffer(1));

	if (*pChar > 0x40)  	   //* If it's Alpha
		*pChar = (char)(*pChar & 0xDF); //* 6x --> 4x (a --> A) 

	if ((*pChar < 0x30) || (*pChar > 0x46))
		return nDM; 	//* Invalid hex digit

	if ((*pChar > 0x39) && (*pChar < 0x41))
		return nDM; 	//* Invalid hex digit

	pChar++;  //* &szTempNum[1];  
	if (*pChar > 0x40)  	   //* If it's Alpha
		*pChar = (char)(*pChar & 0xDF); //* 6x --> 4x (a --> A) 

	if ((*pChar < 0x30) || (*pChar > 0x46))
		return nDM; 	//* Invalid hex digit

	if ((*pChar > 0x39) && (*pChar < 0x41))
		return nDM; 	//* Invalid hex digit

	if (*pChar > 0x40)
		nHex2 = (SHORT)((*pChar - 0x40) + 9); //* A - F --> 10 - 15
	else
		nHex2 = (SHORT)(*pChar - 0x30);

	pChar--; //* &szTempNum[0]; 
	if (*pChar > 0x40)
		nHex1 = (SHORT)((*pChar - 0x40) + 9);  //* A - F --> 10 - 15
	else
		nHex1 = (SHORT)(*pChar - 0x30);

	pPaktTermParms->ucTermChar = (UCHAR)(nHex1 * 16 + nHex2);

	return nDM;
}    


/////////////////////////////////////////////////////////////////////////////
// CPacketTerm message handlers

//void CPacketTerm::OnOK() 

//{
//char szTempNum[10]; 

//	CDialog::OnOK();

//    return;
//}


//* Convert an int 2 1 Hex Digit
//*
void CPacketTerm::Num2HexDigit(int iTemp, PCHAR pChar)

{
	switch (iTemp)
		{
		case 0:
			*pChar = '0';
			break;

		case 1:
			*pChar = '1';
			break;

		case 2:
			*pChar = '2';
			break;

		case 3:
			*pChar = '3';
			break;

		case 4:
			*pChar = '4';
			break;

		case 5:
			*pChar = '5';
			break;

		case 6:
			*pChar = '6';
			break;

		case 7:
			*pChar = '7';
			break;

		case 8:
			*pChar = '8';
			break;

		case 9:
			*pChar = '9';
			break;

		case 10:
			*pChar = 'A';
			break;

		case 11:
			*pChar = 'B';
			break;

		case 12:
			*pChar = 'C';
			break;

		case 13:
			*pChar = 'D';
			break;

		case 14:
			*pChar = 'E';
			break;

		case 15:
			*pChar = 'F';
			break;

		default:;
		}  //* end sw

	return;
}
