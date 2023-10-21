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
/*  Module  NAME : MadTestApp.h                                                */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class MadDevIoApp                     */
/*                                                                             */
/*******************************************************************************/


// MadDevIo.h : main header file for the ABSDEVIO application
//

#if !defined(AFX_ABSDEVIO_H__84F275F5_5C9F_4B59_BEAA_6206D3006BE5__INCLUDED_)
#define AFX_ABSDEVIO_H__84F275F5_5C9F_4B59_BEAA_6206D3006BE5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include <stddef.h>
#define WIN_TITL_SIZE      150
#define READBUFRSIZE      (MAD_SECTOR_SIZE * MAD_DMA_MAX_SECTORS)  
#define WRITEBUFRSIZE     READBUFRSIZE

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoApp:
// See MadDevIo.cpp for the implementation of this class
//

class CMadDevIoApp : public CWinApp
{
public:
	CMadDevIoApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMadDevIoApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CMadDevIoApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ABSDEVIO_H__84F275F5_5C9F_4B59_BEAA_6206D3006BE5__INCLUDED_)
