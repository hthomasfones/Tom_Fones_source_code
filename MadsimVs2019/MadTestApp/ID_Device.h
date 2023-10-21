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
/*  Module  NAME : ID_Device.h                                                 */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class ID_Device                       */
/*                                                                             */
/*******************************************************************************/


#if !defined(AFX_ID_DEVICE_H__7A474F35_8CEC_4DAC_A931_D79D26DB3E7F__INCLUDED_)
#define AFX_ID_DEVICE_H__7A474F35_8CEC_4DAC_A931_D79D26DB3E7F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ID_Device.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ID_Device dialog

class ID_Device : public CDialog
{
// Construction
public:
	ID_Device(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(ID_Device)
	enum { IDD = IDD_ID_Device };
	CString	m_DeviceName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ID_Device)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(ID_Device)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ID_DEVICE_H__7A474F35_8CEC_4DAC_A931_D79D26DB3E7F__INCLUDED_)
