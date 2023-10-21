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
/*  Module  NAME : SetRegBits.h                                                */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class SetRegBits                      */
/*                                                                             */
/*******************************************************************************/

#if !defined(AFX_SETREGBITS_H__0CB310DD_8BBA_4C2F_805D_9D37794D3386__INCLUDED_)
#define AFX_SETREGBITS_H__0CB310DD_8BBA_4C2F_805D_9D37794D3386__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SetRegBits.h : header file
//

typedef struct _SET_REG_PARMS
	{
	ULONG RegBits;
	char  szTitlSufx[25];
	}  SET_REG_PARMS;

  
/////////////////////////////////////////////////////////////////////////////
// CSetRegBits dialog

class CSetRegBits : public CDialog
{
// Construction
public:
	CSetRegBits(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetRegBits)
	enum { IDD = IDD_SetRegBits };
	BOOL	m_bBIT0;
	BOOL	m_bBIT1;
	BOOL	m_bBIT2;
	BOOL	m_bBIT3;
	BOOL	m_bBIT4;
	BOOL	m_bBIT5;
	BOOL	m_bBIT6;
	BOOL	m_bBIT7;
	BOOL	m_bBIT8;
	BOOL	m_bBIT9;
	BOOL	m_bBIT10;
	BOOL	m_bBIT11;
	BOOL	m_bBIT12;
	BOOL	m_bBIT13;
	BOOL	m_bBIT14;
	BOOL	m_bBIT15;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetRegBits)
	public:
	virtual int DoModal(SET_REG_PARMS* pSetRegParms);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSetRegBits)
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETREGBITS_H__0CB310DD_8BBA_4C2F_805D_9D37794D3386__INCLUDED_)
