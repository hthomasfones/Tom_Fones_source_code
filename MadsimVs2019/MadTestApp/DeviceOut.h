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
/*  Module  NAME : DeviceOut.h                                                 */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class DeviceOut                       */
/*                                                                             */
/*******************************************************************************/


#if !defined(AFX_DEVICEOUT_H__5E573B79_107A_4683_9190_8315ABC1B1C4__INCLUDED_)
#define AFX_DEVICEOUT_H__5E573B79_107A_4683_9190_8315ABC1B1C4__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DeviceOut.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDeviceOut dialog

class CDeviceOut : public CDialog
{
// Construction
public:
	CDeviceOut(CWnd* pParent = NULL);   // standard constructor
    UINT DoModal(char* pChar); 

// Dialog Data
	//{{AFX_DATA(CDeviceOut)
	enum { IDD = IDD_DevOut };
	CString	m_CSwritepacket;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeviceOut)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDeviceOut)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnChangeOutput();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEVICEOUT_H__5E573B79_107A_4683_9190_8315ABC1B1C4__INCLUDED_)
