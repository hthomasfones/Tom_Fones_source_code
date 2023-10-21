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
/*  Module  NAME : MadDevSimUsrIntDlg.h                                        */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes, structures, classes, etc.              */
/*                                                                             */
/*******************************************************************************/

// MadDevSimusrIntDlg.h : header file
//

#if !defined(AFX_BMIDEVSIMUSRINTDLG_H__F204A419_F114_4B47_9E63_412EA0D74296__INCLUDED_)
#define AFX_BMIDEVSIMUSRINTDLG_H__F204A419_F114_4B47_9E63_412EA0D74296__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CMadDevSimusrIntDlg dialog

class CMadDevSimusrIntDlg : public CDialog
{
// Construction
public:
	CMadDevSimusrIntDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMadDevSimusrIntDlg)
	enum { IDD = IDD_SIMULATOR_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMadDevSimusrIntDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMadDevSimusrIntDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BMIDEVSIMUSRINTDLG_H__F204A419_F114_4B47_9E63_412EA0D74296__INCLUDED_)
