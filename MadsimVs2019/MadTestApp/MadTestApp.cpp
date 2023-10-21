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
/*  Module  NAME : MadTestApp.cpp                                              */
/*                                                                             */
/*  DESCRIPTION  : Definition of the MFC class MadDevIoApp                     */
/*                                                                             */
/*******************************************************************************/


// MadDevIo.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MadTestApp.h"

#include "MainFrm.h"
#include "MadTestAppDoc.h"
#include "MadTestAppView.h"

#include <initguid.h>
#include "..\Includes\MadGUIDs.h"
#include "..\Includes\MadDefinition.h"  
#include "..\Includes\MadDevIoctls.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GUID gInterfaceGuid = GUID_DEVINTERFACE_MADDEVICE; 
char gszTitlPrefx[] = "Mad Device Test App: ";
char gszDeviceName[100] = MAD_DEVICE_NAME_PREFIX; //"MadDevice";
char gszWinTitl[WIN_TITL_SIZE];

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoApp

BEGIN_MESSAGE_MAP(CMadDevIoApp, CWinApp)
	//{{AFX_MSG_MAP(CMadDevIoApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoApp construction

CMadDevIoApp::CMadDevIoApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
   	//m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);


}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMadDevIoApp object

CMadDevIoApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoApp initialization

#pragma warning( push )
#pragma warning( disable : 4302 )

BOOL CMadDevIoApp::InitInstance()
{
HICON hIcon;
WORD wrc;
WORD wIcon;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CMadDevIoDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CMadDevIoView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

    strcpy_s(gszWinTitl, 150, gszTitlPrefx);
    //strcat_s(gszWinTitl, 150, gszDeviceName);

    m_pMainWnd->SetScrollRange(SB_HORZ, 0, MAXHORZSCROLLRANGE, FALSE);
    m_pMainWnd->SetScrollRange(SB_VERT, 0, MAXVERTSCROLLRANGE, TRUE);

	// The one and only window has been initialized, so show and update it.
    hIcon = LoadIcon(MAKEINTRESOURCE(IDI_ICON3));
	wIcon = WORD(hIcon);
   	wrc = ::SetClassWord(m_pMainWnd->m_hWnd, 12,wIcon);
	m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->SetWindowText(gszWinTitl);
   	wrc = ::SetClassWord(m_pMainWnd->m_hWnd, 12,wIcon);

    //* Resize & position the window
   	m_pMainWnd->MoveWindow(0, 0, (CLIENTWIDTH+10), (CLIENTHEIGHT+10), TRUE);
  	m_pMainWnd->UpdateWindow();
   	wrc = ::SetClassWord(m_pMainWnd->m_hWnd, 12,wIcon);

	return TRUE;
}
#pragma warning( push )

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CMadDevIoApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoApp commands
