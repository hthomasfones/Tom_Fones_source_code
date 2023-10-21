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
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : PacketTerm.h                                                */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes, structures, classes, etc.              */
/*                                                                             */
/*******************************************************************************/

#if !defined(AFX_PACKETTERM_H__E5423332_90B6_4CC2_9884_502BA5373BE2__INCLUDED_)
#define AFX_PACKETTERM_H__E5423332_90B6_4CC2_9884_502BA5373BE2__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

struct _PacketTermParms
	{
	short int	nMaxLen;
	BYTE		ucTermChar;
	};

// PacketTerm.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CPacketTerm dialog

class CPacketTerm: public CDialog
	{
	// Construction
	public:
			CPacketTerm(CWnd* pParent = NULL);   // standard constructor
	UINT	DoModal(struct _PacketTermParms* pPaktTermParms);

	// Dialog Data
	//{{AFX_DATA(CPacketTerm)
	enum {IDD = IDD_PacketTermination};
	CString			m_csMaxLen;
	CString			m_csTermChar;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPacketTerm)
	protected:
	virtual BOOL	OnInitDialog();
	virtual void	DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
	protected:

	// Generated message map functions
	//{{AFX_MSG(CPacketTerm)
	//virtual void OnOK();
	//}}AFX_MSG
					DECLARE_MESSAGE_MAP()

	void			CPacketTerm::Num2HexDigit(int, PCHAR);
	short int		m_nMaxLen;
	unsigned char	m_ucTermChar;
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PACKETTERM_H__E5423332_90B6_4CC2_9884_502BA5373BE2__INCLUDED_)
