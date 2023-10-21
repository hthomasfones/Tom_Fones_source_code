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
/*  Module  NAME : DisplayIO_Dlg.h                                             */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class DisplayIO_Dlg                   */
/*                                                                             */
/*******************************************************************************/


#if !defined(AFX_DISPLAYIO_DLG_H__B364D472_1027_464C_A8CA_B390EE45935F__INCLUDED_)
#define AFX_DISPLAYIO_DLG_H__B364D472_1027_464C_A8CA_B390EE45935F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DisplayIO_Dlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDisplayIO_Dlg dialog

class CDisplayIO_Dlg : public CDialog
{
// Construction
public:
	CDisplayIO_Dlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDisplayIO_Dlg)
	enum { IDD = IDD_Display_IO };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDisplayIO_Dlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDisplayIO_Dlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISPLAYIO_DLG_H__B364D472_1027_464C_A8CA_B390EE45935F__INCLUDED_)
