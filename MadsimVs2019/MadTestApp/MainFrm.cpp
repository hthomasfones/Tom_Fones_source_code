// MainFrm.cpp : implementation of the CMainFrame class
//
//#define INITGUID

#include "stdafx.h"
#include <windows.h>
#include <winioctl.h>
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <share.h>
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <dbt.h>
#include <winerror.h>

#include "..\Includes\MadDefinition.h"  
#include "MadTestApp.h"
#include "MadTestAppView.h"
#include "MadTestAppDoc.h"
#include "Annotate.h"
#include "DeviceOut.h"
#include "DisplayIO_Dlg.h"
#include "SetRegBits.h"

#include "MainFrm.h"
#include "..\Includes\MadMonitor.h"
#define DEVINFO

extern "C" {
	       #include "..\Includes\MadPnpPmUtils.h"
		   }

#include "..\Includes\MadDevIoctls.h"
#include "ID_Device.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern char gszTitlPrefx[]; 
extern char gszDeviceName[]; 
extern char gszWinTitl[];
extern GUID gInterfaceGuid; //= GUID_MAD_DEVICE_INTERFACE_CLASS;
GUID        BOGUS_GUID = 
            {0xffffffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

MADDEV_MAP_VIEWS gMadDevMapViews = {NULL, NULL, NULL};
HANDLE ghDevice = (HANDLE)-1;
FILE*  gpLogFile = NULL;
BOOL   gbBinary = FALSE;
short int gnCurDsplyCnt = 0;
short int gnTextColumn;
short int gnTextRow;

char gDisplayBufr[MAXTEXTLINES][MAXTEXTLEN];
unsigned char gWriteBufr[WRITEBUFRSIZE]; 
unsigned char gReadBufr[READBUFRSIZE];

#define MAD_POWER_USER
#ifdef MAD_POWER_USER //We know the interface guid; we will pass security checks in the driver and have full power
GUID   gTestGUID = gInterfaceGuid; 
#else //We're a dumb end-user; open by symbolic link, only do reads & writes (no ioctls) 
GUID   gTestGUID = BOGUS_GUID;   
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_COMMAND(ID_SetBinaryMode, OnSetBinaryMode)
	ON_COMMAND(ID_FILE_OPEN_DEVICE1, OnFileOpenDevice1)
	ON_COMMAND(ID_FILE_OPEN_DEVICE2, OnFileOpenDevice2)
	ON_COMMAND(ID_FILE_OPEN_DEVICE3, OnFileOpenDevice3)
	ON_COMMAND(ID_FILE_CLOSE, OnDeviceClose)
	ON_COMMAND(ID_FILE_PRINT_SETUP, OnPrintSetup)
	ON_COMMAND(ID_DEVICE_CLOSE,    OnDeviceClose)
	
	ON_COMMAND(ID_INPUT_READ1BYTE, OnRead16bytes)
	ON_COMMAND(ID_INPUT_READ10BYTES, OnRead64bytes)
	ON_COMMAND(ID_INPUT_READPACKET, OnInputRead256bytes)
	ON_COMMAND(ID_INPUT_512BYTES, OnRead512bytes)
	ON_COMMAND(ID_OUTPUT_WRITE1BYTE, OnWrite16bytes)
	ON_COMMAND(ID_OUTPUT_WRITESTRING, OnOutputWrite64bytes)
	ON_COMMAND(ID_OUTPUT_WRITEPACKET, OnOutputWrite256bytes)
	ON_COMMAND(ID_FILE_INITIALIZEDEVICE, OnInitializeDevice)
	ON_COMMAND(ID_FILE_IDENTIFYDRIVER, OnFileIdentifydriver)
	ON_COMMAND(ID_OUTPUT_WRITENBYTES, OnOutputWrite_N_Bytes)
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(ID_IOCONTROLS_NONVALIDCONTROL, OnIoControlsNonvalidControl)
	ON_COMMAND(ID_VIEW_ContinuousIO, OnVIEWContinuousIO)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_COMMAND(ID_PNP_EJECT_ALL,               OnPnpEjectAlldevices)
	ON_COMMAND(ID_PNP_EJECT_UNIT1,             OnPnpEjectDevice1)
	ON_COMMAND(ID_PNP_EJECT_UNIT2,             OnPnpEjectDevice2)
	ON_COMMAND(ID_PNP_EJECT_UNIT3,             OnPnpEjectDevice3)
	ON_COMMAND(ID_PNP_PLUGIN_ALL,              OnPnpPluginAlldevices)
	ON_COMMAND(ID_PNP_PLUGIN_UNIT1,            OnPnpPluginDevice1)
	ON_COMMAND(ID_PNP_PLUGIN_UNIT2,            OnPnpPluginDevice2)
	ON_COMMAND(ID_PNP_PLUGIN_UNIT3,            OnPnpPluginDevice3)
	ON_COMMAND(ID_PNP_UNPLUG_ALL,              OnPnpUnplugAlldevices)
	ON_COMMAND(ID_PNP_UNPLUG_UNIT1,            OnPnpUnplugDevice1)
	ON_COMMAND(ID_PNP_UNPLUG_UNIT2,            OnPnpUnplugDevice2)
	ON_COMMAND(ID_PNP_UNPLUG_UNIT3,            OnPnpUnplugDevice3)
	ON_COMMAND(ID_PNP_MONITOR,                 OnPnpMonitor)
	ON_COMMAND(ID_VIEW_CLEARDISPLAY,           OnViewClearDisplay)
	ON_COMMAND(ID_EDIT_COPYALL,                OnEditCopyAll)
	ON_COMMAND(ID_EDIT_ANNOTATE,               OnEditAnnotate)
	//ON_COMMAND(ID_DEVCNTRLS_WRITE,             OnDevCntrlsWrite)
	//ON_COMMAND(ID_DEVCNTRLS_READ,              OnDevCntrlsRead)
	ON_COMMAND(ID_IOCTL_SET_MAD_CONTROL_REG,       OnIoctlSetControlReg)
	ON_COMMAND(ID_IOCTL_SET_INTEN_REG,         OnIoctlSetIntEnableReg)
	ON_COMMAND(ID_IOCONTROLS_INITIALIZEDEVICE, OnInitializeDevice)
	ON_COMMAND(ID_IOCTL_TESTINIT,              OnIoctlTestInit)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE0_UNIT1,   OnPowerMngtDevState0Unit1)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE0_UNIT2,   OnPowerMngtDevState0Unit2)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE0_UNIT3,   OnPowerMngtDevState0Unit3)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE1_UNIT1,   OnPowerMngtDevState1Unit1)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE1_UNIT2,   OnPowerMngtDevState1Unit2)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE1_UNIT3,   OnPowerMngtDevState1Unit3)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE2_UNIT1,   OnPowerMngtDevState2Unit1)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE2_UNIT2,   OnPowerMngtDevState2Unit2)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE2_UNIT3,   OnPowerMngtDevState2Unit3)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE3_UNIT1,   OnPowerMngtDevState3Unit1)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE3_UNIT2,   OnPowerMngtDevState3Unit2)
	ON_COMMAND(ID_POWERMNGT_DEVSTATE3_UNIT3,   OnPowerMngtDevState3Unit3)
	ON_COMMAND(ID_FILE_OPEN_DEV1_READ,         OnFileOpenDev1Read)
	ON_COMMAND(ID_FILE_OPEN_DEV1_WRITE,        OnFileOpenDev1Write)
	//ON_COMMAND(ID_IOCONTROLS_BLUE_SCREEN, OnIoControls_BlueScreen)

	//ON_COMMAND(ID_MadBus_QUERYBLOCK, OnWmiBusQueryBlock)    
    //ON_COMMAND(ID_MadBus_QUERYITEM, OnWmiBusQueryItem)           
    //ON_COMMAND(ID_MadBus_SETITEM, OnWmiBusSetItem)             
    //ON_COMMAND(ID_MadBus_QUERYREGISTRY, OnWmiBusQueryRegistry)           
    //ON_COMMAND(ID_DEV_QUERYDRIVERBLOCK, OnWmiDevQueryDriverBlock)        
    //ON_COMMAND(ID_DEV_SETDRIVERBLOCK,  OnWmiDevSetDriverBlock)         
    //ON_COMMAND(ID_DEV_QUERYBLOCK, OnWmiDevQueryBlock)              
    //ON_COMMAND(ID_DEV_QUERYREGISTRY, OnWmiDevQueryRegistry)           
    //ON_COMMAND(ID_DEV_SETBLOCK, OnWmiDevSetBlock)                
    //ON_COMMAND(ID_DEV_SETITEM, OnWmiDevSetItem)                 
    //ON_COMMAND(ID_DEV_EXECMETHOD,  OnWmiDevExecMethod)             
    //ON_COMMAND(ID_DEV_FUNCTIONCONTROL,  OnWmiDevFunctionControl)     
	//ON_COMMAND(ID_WINDBG_ASSERT, OnWinDbgAssert)        
	//ON_COMMAND(ID_WINDBG_BREAK, OnWinDbgBreak)        
	//ON_COMMAND(ID_WINDBG_EXCEPTION, OnWinDbgException)        
	//ON_COMMAND(ID_WINDBG_HANG, OnWinDbgHang)        
	//ON_COMMAND(ID_WINDBG_UMHANG, OnUM_Hang)        
	//ON_COMMAND(ID_WINDBG_VERIFIER, OnWinDbgVerifier)        
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_DEVICE_IOCTL_GETREGS, &CMainFrame::OnDeviceIoctlGetRegs)
	ON_COMMAND(ID_DEVICE_IOCTL_MAPDEVICEREGS, &CMainFrame::OnDeviceIoctlMapDeviceRegs)
	ON_COMMAND(ID_PIO_READ, &CMainFrame::OnPioRead)
	ON_COMMAND(ID_PIO_WRITE, &CMainFrame::OnPioWrite)
	ON_COMMAND(ID_PIO_RECEIVE, &CMainFrame::OnPioReceive)
	ON_COMMAND(ID_PIO_TRANSMIT, &CMainFrame::OnPioTransmit)
	ON_COMMAND(ID_PIO_LOOPBACK, &CMainFrame::OnPioLoopback)
	ON_COMMAND(ID_HELP_ABOUT, &CMainFrame::OnHelpAbout)
	ON_COMMAND(ID_HELP_VIEW_DEVREGMAP, &CMainFrame::OnHelpViewDevRegMap)
	ON_COMMAND(ID_HELP_SHow_READMEFILE, &CMainFrame::OnHelpShowReadmefile)
	ON_COMMAND(ID_HELP_VIEW_SIMULATION_DIAGRAM, &CMainFrame::OnHelpViewSimulationDiagram)
	ON_COMMAND(ID_CACHE_ALIGN_READ, &CMainFrame::OnCacheAlignRead)
	ON_COMMAND(ID_CACHE_ALIGN_WRITE, &CMainFrame::OnCacheAlignWrite)
	ON_COMMAND(ID_CACHE_READ, &CMainFrame::OnCacheRead)
	ON_COMMAND(ID_CACHE_WRITE, &CMainFrame::OnCacheWrite)
	ON_COMMAND(ID_INPUT_ONE_SECTOR, &CMainFrame::OnInputOneSector)
	ON_COMMAND(ID_INPUT_FOUR_SECTORS, &CMainFrame::OnInputFourSectors)
	ON_COMMAND(ID_INPUT_SIXTEEN_SECTORS, &CMainFrame::OnInputSixteenSectors)
	ON_COMMAND(ID_OUTPUT_ONE_SECTOR, &CMainFrame::OnOutputOneSector)
	ON_COMMAND(ID_OUTPUT_FOUR_SECTORS, &CMainFrame::OnOutputFourSectors)
	ON_COMMAND(ID_OUTPUT_SIXTEEN_SECTORS, &CMainFrame::OnOutputSixteenSectors)
	ON_COMMAND(ID_INPUT_RUN_SCRIPT, &CMainFrame::OnInputRunScript)
	ON_COMMAND(ID_OUTPUT_RUN_SCRIPT, &CMainFrame::OnOutputRunScript)
	ON_COMMAND(ID_PLUGNPLAY_PNPSTRESS, &CMainFrame::OnPlugnplayStress)
	ON_COMMAND(ID_FILE_LOGFILECLOSE, &CMainFrame::OnFileLogFileClose)
	ON_COMMAND(ID_FILE_LOGFILEOPEN, &CMainFrame::OnFileLogFileOpen)
	ON_COMMAND(ID_IOCONTROLS_RESET_INDECES, &CMainFrame::OnIocontrolsResetIndeces)
	ON_COMMAND(ID_WMI_BUS_QUERY, &CMainFrame::OnWmiBusQuery)
	ON_COMMAND(ID_WMI_DEVICE_QUERY, &CMainFrame::OnWmiDeviceQuery)
	END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()

{
    m_bBinaryMode  = FALSE;
    m_bViewContIO  = FALSE;
    m_nSerialNo    = 0;

    return;
}


CMainFrame::~CMainFrame()
{

}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)

{
    cs.style = cs.style | WS_HSCROLL | WS_VSCROLL;
   
	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
// Windows (WM_xxx messages 1st) *************************
//*

//*** Position the window vertically
//* 
void CMainFrame::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 

{
static int nHscrollPos = 0;
int rc = 0;
int nScrollPos;

    nScrollPos = GetScrollPos(SB_HORZ);
    switch (nSBCode)
        {
        case SB_LINELEFT:
            nHscrollPos = nScrollPos - STDTEXTWIDTH;
            break;
        case SB_LINERIGHT:
            nHscrollPos = nScrollPos + STDTEXTWIDTH;
            break;
       case SB_PAGELEFT:
            nHscrollPos = nScrollPos - CLIENTWIDTH;
            break; 
       case SB_PAGERIGHT:
            nHscrollPos = nScrollPos + CLIENTWIDTH;
            break;           
       case SB_THUMBPOSITION:
             nHscrollPos = nPos;
             break; 
       default:;
       } //end sw

    //* Normalize out-of-bounds if necessary  
    nHscrollPos = __max(nHscrollPos, 0);
    nHscrollPos = __min(nHscrollPos, MAXHORZSCROLLRANGE);

    if (nHscrollPos != nScrollPos)
        {
        nScrollPos = SetScrollPos(SB_HORZ, nHscrollPos, TRUE); 
        gnTextColumn = nHscrollPos / STDTEXTWIDTH;
        InvalidateRect(NULL, TRUE);
        }   

	return;
}


//*** Position the window horizontally
//* 
void CMainFrame::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 

{
static int nVscrollPos = 0;
int rc = 0;
int nScrollPos;

    nScrollPos = GetScrollPos(SB_VERT);
    switch (nSBCode)
        {
        case SB_LINEUP:
            nVscrollPos = nScrollPos - STDTEXTHEIGHT;
            break;
        case SB_LINERIGHT:
            nVscrollPos = nScrollPos + STDTEXTHEIGHT;
            break;
       case SB_PAGEUP:
            nVscrollPos = nScrollPos - CLIENTHEIGHT;
            break; 
       case SB_PAGERIGHT:
            nVscrollPos = nScrollPos + CLIENTHEIGHT;
            break;           
       case SB_THUMBPOSITION:
             nVscrollPos = nPos;
             break; 
       default:;
       } //end sw

    //* Normalize out-of-bounds if necessary  
    nVscrollPos = __max(nVscrollPos, 0);
    nVscrollPos = __min(nVscrollPos, MAXVERTSCROLLRANGE);

   if (nVscrollPos != nScrollPos)
        {
        nScrollPos = SetScrollPos(SB_VERT, nVscrollPos, TRUE); 
        gnTextRow  = nVscrollPos / STDTEXTHEIGHT;
        InvalidateRect(NULL, TRUE);
        }   

    return;
}


//*************
//*
void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu) 

{
CMenu* pMenu;

	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    pMenu = GetMenu();

    pMenu->CheckMenuItem(ID_SetBinaryMode, 
                         (m_bBinaryMode  ?  MF_CHECKED : MF_UNCHECKED));

    pMenu->CheckMenuItem(ID_VIEW_ContinuousIO, 
                         (m_bViewContIO  ?  MF_CHECKED : MF_UNCHECKED));

    return;	
}


//* Application defined menu mesages ******************************
//*
void CMainFrame::OnClearDisplay() 

{

}


//*** File Menu Functions ********************************************************
//*
void CMainFrame::OnFileIdentifydriver() 

{
static char szDevIDPrefx[] = "Current Device ID is: ";
UINT uRC;
ID_Device ID_DeviceDlg;
char szCurDeviceID[100]; 

    return;

    strcpy_s(szCurDeviceID, 100, szDevIDPrefx);
    strcat_s(szCurDeviceID, 100, gszDeviceName);

    uRC = MessageBox(szCurDeviceID, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
    if (uRC == IDCANCEL) 
		return;
        
    ID_DeviceDlg.DoModal();

    strcpy_s(szCurDeviceID, 100, szDevIDPrefx);
    strcat_s(szCurDeviceID, 100, gszDeviceName);
    Update_Display_Log(szCurDeviceID);

//* Update the Window Title
//*
    strcpy_s(gszWinTitl, WIN_TITL_SIZE, gszTitlPrefx);
    //strcat_s(gszWinTitl, WIN_TITL_SIZE, gszDeviceName);
    CFrameWnd::SetWindowText(gszWinTitl);

    //uRC = (UINT)PostMessage(WM_PAINT, 0, 0); 
 
    return; 
} 

//*** OnFileDeviceOpen
//*
void CMainFrame::OnFileDeviceOpen() 

{
ULONG SerialNo = 2;
//int nLen;
BOOL bRC;

    ghDevice = DeviceOpen(SerialNo, (GENERIC_READ | GENERIC_WRITE)); 
	bRC = (ghDevice != INVALID_HANDLE_VALUE);

 	return;
}


//*** OnFileOpenDevice1
//* 
void CMainFrame::OnFileOpenDevice1() 

{
BOOL bRC;

    ghDevice = DeviceOpen(1, (GENERIC_READ | GENERIC_WRITE)); 
	bRC = (ghDevice != INVALID_HANDLE_VALUE);
	if (bRC)
		m_nSerialNo = 1;

	return;
}


//*** OnFileOpenDevice2
//* 
void CMainFrame::OnFileOpenDevice2() 

{
BOOL bRC;

    ghDevice = DeviceOpen(2, (GENERIC_READ | GENERIC_WRITE)); 
	bRC = (ghDevice != INVALID_HANDLE_VALUE);
	if (bRC)
		m_nSerialNo = 2;

	return;
}


//*** OnFileOpenDevice3
//* 
void CMainFrame::OnFileOpenDevice3() 

{
BOOL bRC;

    ghDevice = DeviceOpen(3, (GENERIC_READ | GENERIC_WRITE)); 
	bRC = (ghDevice != INVALID_HANDLE_VALUE);
	if (bRC)
		m_nSerialNo = 3;

	return;
}


//* This function is useful for verifying that opening for read-access
//* exclusively works properly
//*
void CMainFrame::OnFileOpenDev1Read() 

{
BOOL bRC;

    ghDevice = DeviceOpen(1, GENERIC_READ); 
	bRC = (ghDevice != INVALID_HANDLE_VALUE);
	if (bRC)
		m_nSerialNo = 1;

	return;
}


//* This function is useful for verifying that opening for write-access
//* exclusively works properly
//*
void CMainFrame::OnFileOpenDev1Write() 

{
BOOL bRC;

    ghDevice = DeviceOpen(1, GENERIC_WRITE); 
	bRC = (ghDevice != INVALID_HANDLE_VALUE);
	if (bRC)
		m_nSerialNo = 1;

	return;	
}


//*** OnFileOpenDevice1
//*
void CMainFrame::OnDeviceClose() 

{
char szInfoMsg[100] = "IOCTL: unmap whole device succeeded.";
char szErrPrefix[]  = "IOCTL: unmap whole device failed";
ULONG Count;
BOOL bRC = TRUE;
errno_t errno;
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;

    if (gMadDevMapViews.pDeviceRegs != NULL)
		{
		MadDevIoctlStruct.SecurityKey = gTestGUID;
		errno = memcpy_s(MadDevIoctlStruct.DataBufr, MAD_SECTOR_SIZE,
			             &gMadDevMapViews, sizeof(MADDEV_MAP_VIEWS));
	    bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_UNMAP_VIEWS, 
		                   (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT),
	                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT),
					       &Count, NULL, szErrPrefix); 
        gMadDevMapViews.pDeviceRegs = NULL;
        gMadDevMapViews.pPioRead    = NULL;
        gMadDevMapViews.pPioWrite   = NULL;
		}

    CloseHandle(ghDevice);
    ghDevice = (HANDLE)-1;

	strcpy_s(gszWinTitl, WIN_TITL_SIZE, gszTitlPrefx);
	CFrameWnd::SetWindowText(gszWinTitl);

	strcpy_s(szInfoMsg, 100, "Target device closed.");
    Update_Display_Log(szInfoMsg);

    return;
}

void CMainFrame::OnFileLogFileOpen()
{
static char szLogFile[] = MadTestAppLog;
UINT mb_Icon = MB_ICONINFORMATION;
char szInfoMsg[MAXTEXTLEN];
UINT uRC;
errno_t errno;

    errno = fopen_s(&gpLogFile, szLogFile, "w");
	if (gpLogFile == NULL)
	    {
        sprintf_s(szInfoMsg, MAXTEXTLEN, "Open failure...errno=%d", errno);
		mb_Icon = MB_ICONEXCLAMATION;
	    }
	else
        sprintf_s(szInfoMsg, MAXTEXTLEN, "Log file opened: %s", szLogFile);


    uRC = MessageBox(szInfoMsg, gszWinTitl, mb_Icon);
	Update_Display_Log(szInfoMsg);
	
	return;
}


void CMainFrame::OnFileLogFileClose()
{
	if (gpLogFile != NULL)
	    {
		fclose(gpLogFile);
		gpLogFile = NULL;
	    }

	return;
}


//*** OnPrintSetup
//* 
void CMainFrame::OnPrintSetup() 
{

    return;
}


//*** OnPrint
//* 
void CMainFrame::OnPrint() 

{

	return;
}


//*** Here are the Plug-n-Play menu functions *********************************
//*

//*** This function launches the monitor program and places it in our Client Area
//*
void CMainFrame::OnPnpMonitor() 

{
DWORD dwFlags = 0; //, dwGLE;
BOOL bRC;
char szAppName[] = MadMonCmd; //"MadMonitor.exe "; //* The device PnP Monitor program
char szCmdLine[] = "";

    bRC = CreateProcessInClientArea(szAppName, szCmdLine,
		                            NULL, NULL, FALSE, dwFlags, NULL, NULL);
 
    return;	
}


//***********************************
//*
void CMainFrame::OnPnpEjectAlldevices() 
{
CString csWinExec = MadEjectCmd; 

    csWinExec = csWinExec + "*"; 
    Exec_PnP_Enumerate(csWinExec);
        
    return;	
}


//***********************************
//*
void CMainFrame::OnPnpEjectDevice1() 

{
CString csWinExec = MadEjectCmd; //"MadEnum.exe -E 1";

    csWinExec = csWinExec + "1"; 
    Exec_PnP_Enumerate(csWinExec);
        
    return;	
}


//***********************************
//*
void CMainFrame::OnPnpEjectDevice2() 

{
CString csWinExec = MadEjectCmd; //"MadEnum.exe -E 2";

    csWinExec = csWinExec + "2"; 
    Exec_PnP_Enumerate(csWinExec);
        
    return;	
}


//***********************************
//*
void CMainFrame::OnPnpEjectDevice3() 

{
CString csWinExec = MadEjectCmd; //"MadEnum.exe -E 3";

    csWinExec = csWinExec + "3"; 
    Exec_PnP_Enumerate(csWinExec);
        
    return;	
}


//***********************************
//*
void CMainFrame::OnPnpPluginAlldevices() 

{
CString csWinExec = MadPlugInCmd; //* = "MadEnum.exe -P 1";
CString csWinExecX;

    csWinExecX = csWinExec + "1";
    Exec_PnP_Enumerate(csWinExecX);

	csWinExecX = csWinExec + "2";
    Exec_PnP_Enumerate(csWinExecX);
 
    csWinExecX = csWinExec + "3";
    Exec_PnP_Enumerate(csWinExecX);
         
    return;		
}


//***********************************
//*
void CMainFrame::OnPnpPluginDevice1() 

{
CString csWinExec = MadPlugInCmd; 

    csWinExec = csWinExec + "1";
    Exec_PnP_Enumerate(csWinExec);
        
    return;	
}


//***********************************
//*
void CMainFrame::OnPnpPluginDevice2() 

{
CString csWinExec = MadPlugInCmd; 

    csWinExec = csWinExec + "2";
    Exec_PnP_Enumerate(csWinExec);
        
    return;	
}


//***********************************
//*
void CMainFrame::OnPnpPluginDevice3() 

{
CString csWinExec = MadPlugInCmd;; 

    csWinExec = csWinExec + "3";
    Exec_PnP_Enumerate(csWinExec);
        
    return;		
}


//***********************************
//*
void CMainFrame::OnPnpUnplugAlldevices() 

{
CString csWinExec   = MadUnplugCmd;
CString csWinExecX;

    csWinExecX = csWinExec + "1";
    Exec_PnP_Enumerate(csWinExecX);

	csWinExecX = csWinExec + "2";
    Exec_PnP_Enumerate(csWinExecX);
 
    csWinExecX = csWinExec + "3";
    Exec_PnP_Enumerate(csWinExecX);
         
    return;		
}


//***********************************
//*
void CMainFrame::OnPnpUnplugDevice1() 

{
CString csWinExec = MadUnplugCmd; 

    csWinExec = csWinExec + "1";
    Exec_PnP_Enumerate(csWinExec);
        
    return;	
}


//***********************************
//*
void CMainFrame::OnPnpUnplugDevice2() 

{
CString csWinExec = MadUnplugCmd; 

    csWinExec = csWinExec + "2";
    Exec_PnP_Enumerate(csWinExec);
        
    return;	
}


//***********************************
//*
void CMainFrame::OnPnpUnplugDevice3() 

{
CString csWinExec = MadUnplugCmd; 

    csWinExec = csWinExec + "3";
    Exec_PnP_Enumerate(csWinExec);
        
    return;	
}


void CMainFrame::OnPlugnplayStress()
{
DWORD dwFlags = 0; 
BOOL bRC;
UINT uRC;
char szInfoMsg[MAXTEXTLEN];
char szAppName[] = MadPnpStressName; 
char szCmdLine[] = "";

    sprintf_s(szInfoMsg, MAXTEXTLEN,
	"Run the automated test script %s\n\n!*** Please do not run this script until all (three ?) device units have the driver installed ***!",
		      szAppName);
    uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
    if (uRC == IDCANCEL)
        return;

    bRC = CreateProcessInClientArea(szAppName, szCmdLine,
		                            NULL, NULL, FALSE, dwFlags, NULL, NULL);
        
	return;
}

//***********************************
//*
void CMainFrame::OnPowerMngtDevState0Unit1() 

{
CString csSuffix = " -S0 1";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;	
}


//***********************************
//*
void CMainFrame::OnPowerMngtDevState0Unit2() 

{
CString csSuffix = " -S0 2";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;	
}


//***********************************
//*
void CMainFrame::OnPowerMngtDevState0Unit3() 

{
CString csSuffix = " -S0 3";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;
}


//***********************************
//*
void CMainFrame::OnPowerMngtDevState1Unit1() 

{
CString csSuffix = " -S1 1";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;
}


//***********************************
//*
void CMainFrame::OnPowerMngtDevState1Unit2() 

{
CString csSuffix = " -S1 2";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;	
}


//******************************************
//*
void CMainFrame::OnPowerMngtDevState1Unit3() 

{
CString csSuffix = " -S1 3";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;
}



//***************************************
//*
void CMainFrame::OnPowerMngtDevState2Unit1() 

{

CString csSuffix = " -S2 1";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;	
}


//***************************************
//*
void CMainFrame::OnPowerMngtDevState2Unit2() 

{

CString csSuffix = " -S2 2";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;	
}


//***************************************
//*
void CMainFrame::OnPowerMngtDevState2Unit3() 

{
CString csSuffix = " -S2 3";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;
}



//***************************************
//*
void CMainFrame::OnPowerMngtDevState3Unit1() 

{
CString csSuffix = " -S3 1";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;
}


//***************************************
//*
void CMainFrame::OnPowerMngtDevState3Unit2() 

{
CString csSuffix = " -S3 2";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;	
}


//***************************************
//*
void CMainFrame::OnPowerMngtDevState3Unit3() 

{
CString csSuffix = " -S3 3";

    Exec_PowerMngt_Funxn(csSuffix);
	
    return;
}



//*** Input From Device menu functions **********************************************
//*
void CMainFrame::OnRead16bytes() 

{
	Read_N_Bytes(16);

    return;	
}


//**********************************
//*
void CMainFrame::OnRead64bytes() 

{
    Read_N_Bytes(64);

    return;
}


//**********************************
//*
void CMainFrame::OnInputRead256bytes() 

{
    Read_N_Bytes(256); 

    return;
}


//**********************************
//*
void CMainFrame::OnRead512bytes() 

{
    Read_N_Bytes(512); 
	
	return;
}

#if 0 //#ifndef BLOCK_MODE
#endif

void CMainFrame::OnInputOneSector()
{
    Read_N_Sectors(1, 1);

	return;
}


void CMainFrame::OnInputFourSectors()
{
	Read_N_Sectors(4, 4);

	return;
}


void CMainFrame::OnInputSixteenSectors()
{
    Read_N_Sectors(16, 16);

	return;
}


void CMainFrame::OnInputRunScript()
{
    RunAutoScript();

    return;
}


//*** Output To Device Menu Functions ****************************
//*
void CMainFrame::OnWrite16bytes() 

{
static char szTestData[30] =  {"ABCDEFGHIJKLNMOPQRSTUVWXYZ"};
static short int nDX = -1;
UINT mbIcon = MB_ICONINFORMATION;
int   IoctlResult = 1;
char szInfoMsg[100] = "Write 16 bytes completed.";

    nDX++;
    nDX = (nDX % 26); //* Cycle through the hard-coded test data

	Write_N_Bytes(szTestData, 16); 

    return;
}


//* Just like write N bytes with the Null terminator included
//*
void CMainFrame::OnOutputWrite64bytes() 

{
CDeviceOut DevOutDlg;
UINT nDM;
//DWORD dwGLE;
int   IoctlResult = 1;
UINT mbIcon = MB_ICONINFORMATION;
char szWriteString[250] = "";
char szInfoMsg[100] = "Write 64 bytes completed.\n ";

    nDM = DevOutDlg.DoModal(szWriteString);
    if (nDM == IDCANCEL)
        {
        //MessageBox("User hit <Cancel>", gszWinTitl, MB_ICONINFORMATION);
        return;
        }

	Write_N_Bytes(szWriteString, 64); 

    return;    
}


//***********************************
//*
void CMainFrame::OnOutputWrite256bytes() 

{
CDeviceOut DevOutDlg;
ULONG nCount = 0;
UINT nDM;
UINT uLen = 256;
int   IoctlResult = 1;
UINT mbIcon = MB_ICONINFORMATION;
char szWritePacket[260] = "";
char szInfoMsg[100] = "Write 256 bytes ...";

    nDM = DevOutDlg.DoModal(szWritePacket);
    if (nDM == IDCANCEL)
        {
        //MessageBox("User hit <Cancel>", gszWinTitl, MB_ICONINFORMATION);
        return;
        }

	Write_N_Bytes(szWritePacket, 256); 

    return;
}


//**************************************
//*
void CMainFrame::OnOutputWrite_N_Bytes() 

{
register ULONG j;
CDeviceOut DevOutDlg;
BOOL bRC;
ULONG nCount;
UINT nDM;
DWORD dwGLE;
int   IoctlResult = 1;
UINT mbIcon = MB_ICONINFORMATION;
UINT uLen;
char szWrite_N_Bytes[250] = "";
char szInfoMsg[300] = "Write N bytes completed.\n ";
char szErrText[50];

    nDM = DevOutDlg.DoModal(szWrite_N_Bytes);
    if (nDM == IDCANCEL)
        {
        //MessageBox("User hit <Cancel>", gszWinTitl, MB_ICONINFORMATION);
        return;
        }

    uLen = (ULONG)strlen(szWrite_N_Bytes);

//#ifdef BLOCK_MODE
    uLen = MAD_BLOCK_SIZE;
//#endif

    if (m_bBinaryMode)
        for (j = 0; j < uLen; j++)
            szWrite_N_Bytes[j] = szWrite_N_Bytes[j] | 0x80; //* Set the high-order bit

    strcpy_s((char *)gWriteBufr, uLen, szWrite_N_Bytes);
	bRC = WriteFile(ghDevice, gWriteBufr, uLen, &nCount, NULL);
    if (bRC)   // Did the write succeed?
        sprintf_s(szInfoMsg, 100,
		          "Write %d bytes completes ... \n%s", uLen, szWrite_N_Bytes);
    else
        {
        dwGLE = GetLastError(); 
 		Assign_GLE_Text(dwGLE, szErrText);
		sprintf_s(szInfoMsg, 100, "Write N bytes failed! GLE = %d:%s ", dwGLE, szErrText);
        mbIcon = MB_ICONEXCLAMATION;
        }

   	MessageBox(szInfoMsg, gszWinTitl, mbIcon);
    Update_Display_Log(szInfoMsg);

    return;	
}


void CMainFrame::OnOutputOneSector()
{
    Write_N_Sectors(1, 1);

	return;
}


void CMainFrame::OnOutputFourSectors()
{
    Write_N_Sectors(4, 4);

	return;
}


void CMainFrame::OnOutputSixteenSectors()
{
	Write_N_Sectors(16, 16);

	return;
}


void CMainFrame::OnOutputRunScript()
{
    RunAutoScript();

    return;
}

#if 0
#endif

//*** Device I/O Control menu functions ***********************************
//*
void CMainFrame::OnInitializeDevice() 

{
static char szInfoMsg[]   = "IOCTL: Initialize target device succeeded.";
static char szErrPrefix[] = "IOCTL: Initialize target device failed";
ULONG nCount;
BOOL bRC;
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;

    MadDevIoctlStruct.SecurityKey = gTestGUID;
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_INITIALIZE, 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &nCount, 
		     	       szInfoMsg, szErrPrefix); 

    return;	
}


void CMainFrame::OnIoctlTestInit() 

{
static char szInfoMsg[]   = "IOCTL: Test-Init target device succeeded.";
static char szErrPrefix[] = "IOCTL: Test-Init target device failed";

	//bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_TEST_INIT, 
    //                  NULL, 0, NULL, 0, &nCount, szInfoMsg, szErrPrefix); 

    return;	
	
}


void CMainFrame::OnIocontrolsResetIndeces()
{
static char szInfoMsg[]   = "IOCTL: Reset device indeces succeeded.";
static char szErrPrefix[] = "IOCTL: Reset device indeces failed";
ULONG nCount;
BOOL bRC;
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;

    MadDevIoctlStruct.SecurityKey = gTestGUID;
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_RESET_INDECES, 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &nCount, 
		     	       szInfoMsg, szErrPrefix); 

    return;	
}


void CMainFrame::OnCacheRead()
{
static char szErrPrefix[] = "IOCTL: Cache read from the target device failed";
ULONG nCount;
UINT RC;
BOOL bRC;
char szInfoMsg[MAXTEXTLEN] = ""; //IOCTL: Cache read from the target device succeeded.";
//char szHexData[40] = "";
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;

    sprintf_s(szInfoMsg, 100, "Read from the read-cache");
    RC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
	if (RC == IDCANCEL)
         return;

    MadDevIoctlStruct.SecurityKey = gTestGUID;
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_CACHE_READ, 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &nCount, 
		     	       NULL, szErrPrefix); 
	if (!bRC)
		return;

	if (nCount > 0)
		{
        sprintf_s(szInfoMsg, MAXTEXTLEN, "%d bytes returned: \n", nCount);
		//RC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
		MadDevIoctlStruct.DataBufr[50] = 0x00;
        strcat_s(szInfoMsg, MAXTEXTLEN, (char *)MadDevIoctlStruct.DataBufr); //* Should contain null byte
		RC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
        Update_Display_Log(szInfoMsg);
		}

    return;	
}


void CMainFrame::OnCacheWrite()
{
static char szErrPrefix[] = "IOCTL: Cache write to the target device failed";
static char szTestData[30] =  {"ABCDEFGHIJKLNMOPQRSTUVWXYZ"};
ULONG nCount;
UINT RC;
BOOL bRC;
char szInfoMsg[MAXTEXTLEN] = "IOCTL: Cache write to the target device succeeded";
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;

    sprintf_s(szInfoMsg, 100, "Write through to the write-cache");
    RC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
	if (RC == IDCANCEL)
         return;

    sprintf_s(szInfoMsg, 100, "IOCTL: Cache write to the target device succeeded.");
    MadDevIoctlStruct.SecurityKey = gTestGUID;
	strcpy_s(MadDevIoctlStruct.DataBufr, MAD_SECTOR_SIZE, szTestData); 
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_CACHE_WRITE, 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &nCount, 
		     	       szInfoMsg, szErrPrefix); 

	return;	
}

void CMainFrame::OnCacheAlignRead()
{
static ULONG CacheIndx = 35;
static char szErrPrefix[] = "IOCTL: Align write cache to the target device failed";
ULONG nCount;
//UINT RC;
BOOL bRC;
char szInfoMsg[MAXTEXTLEN] = "IOCTL: Align write to the target device succeeded";
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;

    //sprintf_s(szInfoMsg, 100, "Read through the read cache");
    //RC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
	//if (RC == IDCANCEL)
    //     return;

    MadDevIoctlStruct.SecurityKey = gTestGUID;
	memcpy_s(MadDevIoctlStruct.DataBufr, MAD_SECTOR_SIZE, (PVOID)&CacheIndx, sizeof(ULONG)); 
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_ALIGN_READ, 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &nCount, 
		     	       szInfoMsg, szErrPrefix); 

	return;	
}


void CMainFrame::OnCacheAlignWrite()
{
static ULONG CacheIndx = 35;
static char szErrPrefix[] = "IOCTL: Align write cache to the target device failed";
ULONG nCount;
//UINT RC;
BOOL bRC;
char szInfoMsg[MAXTEXTLEN] = "IOCTL: Align write to the target device succeeded";
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;
//char szHexData[40];

    //sprintf_s(szInfoMsg, 100, "Read through the read cache");
    //RC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
	//if (RC == IDCANCEL)
    //     return;

    MadDevIoctlStruct.SecurityKey = gTestGUID;
	memcpy_s(MadDevIoctlStruct.DataBufr, MAD_SECTOR_SIZE, (PVOID)&CacheIndx, sizeof(ULONG)); 
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_ALIGN_WRITE, 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &nCount, 
		     	       szInfoMsg, szErrPrefix); 

	return;	
}


//* Try an I/O operation (READ) using DeviceIoControl 
/*
void CMainFrame::OnDevCntrlsRead() 

{
ULONG nCount;
//UINT mbIcon = MB_ICONINFORMATION;
//int   IoctlResult = 1;
//DWORD dwGLE;
BOOL bRC;
char szInfoMsg[] = "IOCTL: Read 1 byte completed.";
char szErrPrefix[] = "IOCTL: Read 1 byte failed";

	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_READ, 
                       (PUCHAR)&gWriteBufr, 1, (PUCHAR)&gReadBufr, 1, &nCount, 
		     	       szInfoMsg, szErrPrefix);  

    return;
}
/* */

//* Try an I/O operation (WRITE) using DeviceIoControl 
/*
void CMainFrame::OnDevCntrlsWrite() 

{
static char szTestData[30] =  {"ABCDEFGHIJKLNMOPQRSTUVWXYZ"};
static short int nDX = -1;
ULONG nCount;
//UINT mbIcon = MB_ICONINFORMATION;
//DWORD dwGLE;
//int   IoctlResult = 1;
BOOL bRC;
char szInfoMsg[] =   "IOCTL: Write 1 byte completed.";
char szErrPrefix[] = "IOCTL: Write 1 byte failed";

    nDX++;
    nDX = (nDX % 26); //* Cycle through the hard-coded test data

    gWriteBufr[0] = szTestData[nDX];
    if (m_bBinaryMode)
        gWriteBufr[0] = gWriteBufr[0] | 0x80; //* Set the high-order bit

	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_WRITE, 
                       (PUCHAR)&gWriteBufr, 1, (PUCHAR)&gReadBufr, 1, &nCount, 
		     	       szInfoMsg, szErrPrefix);  

    return;
}
/* */


//************* Set 16-bit Int ID Reg
/* */
void CMainFrame::OnIoctlSetIntEnableReg() 

{
static char szTitlSufx[] = "Int-Enable";
CSetRegBits SetRegBitsDlg;
SET_REG_PARMS SetRegParms;
ULONG IntEnableReg;
UINT uRC;
ULONG IoCount;
BOOL bRC;
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;
char szInfoMsgGet[50]   = "Get Int-Enable Reg succeeds";
char szErrPrefixGet[] = "Get Int-Enable Reg failed";
char szInfoMsgSet[]   = "Set Int-Enable Reg succeeds";
char szErrPrefixSet[] = "Set Int-Enable Reg failed";

    MadDevIoctlStruct.SecurityKey = gTestGUID;
	memset(MadDevIoctlStruct.DataBufr, 0x00, MAD_SECTOR_SIZE);
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_GET_INTEN_REG,  
		               (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &IoCount, 
	     	           NULL, szErrPrefixGet);  
	if (!bRC)
		return;

	IntEnableReg = *((PULONG)MadDevIoctlStruct.DataBufr);
	sprintf_s(szInfoMsgGet, 50, "IntEnable = 0x%X", IntEnableReg);
	//MessageBox(szInfoMsgGet, NULL, MB_OK);

	SetRegParms.RegBits = IntEnableReg;
	strcpy_s(SetRegParms.szTitlSufx, 20, szTitlSufx);
	//SetRegBitsDlg.SetWindowText(szTitlSufx);
    uRC = SetRegBitsDlg.DoModal(&SetRegParms);
	if (uRC == IDCANCEL)
		return;

	memset(MadDevIoctlStruct.DataBufr, 0x00, MAD_SECTOR_SIZE);
	IntEnableReg = SetRegParms.RegBits;
	*((PULONG)MadDevIoctlStruct.DataBufr) = IntEnableReg;
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_SET_INTEN_REG,  
					   (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT),
					   (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &IoCount, 
	     	           szInfoMsgSet, szErrPrefixSet);  

	return;
}


//************* Set 16-bit Int ID Reg
/* */
void CMainFrame::OnIoctlSetControlReg() 

{
static char szTitlSufx[] = "Control";
CSetRegBits SetRegBitsDlg;
SET_REG_PARMS SetRegParms;
ULONG IntEnableReg;
UINT uRC;
ULONG IoCount;
BOOL bRC;
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;
char szInfoMsgGet[50]   = "Get Control Reg succeeds";
char szErrPrefixGet[] = "Get Control Reg failed";
char szInfoMsgSet[]   = "Set Control Reg succeeds";
char szErrPrefixSet[] = "Set Control Reg failed";

    MadDevIoctlStruct.SecurityKey = gTestGUID;
	memset(MadDevIoctlStruct.DataBufr, 0x00, MAD_SECTOR_SIZE);
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_GET_MAD_CONTROL_REG,  
		               (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &IoCount, 
	     	           NULL, szErrPrefixGet);  
	if (!bRC)
		return;

	IntEnableReg = *((PULONG)MadDevIoctlStruct.DataBufr);
	sprintf_s(szInfoMsgGet, 50, "Control = 0x%X", IntEnableReg);
	//MessageBox(szInfoMsgGet, NULL, MB_OK);

	SetRegParms.RegBits = IntEnableReg;
	strcpy_s(SetRegParms.szTitlSufx, 20, szTitlSufx);
	//SetRegBitsDlg.SetWindowText(szTitlSufx);
    uRC = SetRegBitsDlg.DoModal(&SetRegParms);
	if (uRC == IDCANCEL)
		return;

	memset(MadDevIoctlStruct.DataBufr, 0x00, MAD_SECTOR_SIZE);
	IntEnableReg = SetRegParms.RegBits;
	*((PULONG)MadDevIoctlStruct.DataBufr) = IntEnableReg;
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_SET_MAD_CONTROL_REG,  
					   (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT),
					   (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &IoCount, 
	     	           szInfoMsgSet, szErrPrefixSet);  

	return;
}



/* */


//************ Invalid Device Control
//*
void CMainFrame::OnIoControlsNonvalidControl() 

{
static char szInfoMsg[100] = "WARNING! Target device Nonvalid IOCTL succeeded!";
static char szErrPrefix[] = "Target device Nonvalid IOCTL failed";
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;
BOOL bRC;
ULONG nCount;

	MadDevIoctlStruct.SecurityKey = gTestGUID;
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_CACHE_WRITE+69, 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &nCount, 
		     	       szInfoMsg, szErrPrefix); 

    return;	
}


void CMainFrame::OnIoControls_BlueScreen() 

{
//ULONG nCount;
int IoctlResult = 1;
DWORD dwGLE;
//BOOL bRC;
//UINT mbIcon = MB_ICONEXCLAMATION;
char szInfoMsg[100];
char szErrText[50];

    //IoctlResult = DeviceIoControl(ghDevice, MADDEV_IOCTL_BLUE_SCREEN, 
    //                              NULL, 0, NULL, 0, &nCount, NULL);
	dwGLE = GetLastError();
	Assign_GLE_Text(dwGLE, szErrText);
   	sprintf_s(szInfoMsg, 100, " Target device Blue Screen returned *!*  GLE = %d:%s ",
              dwGLE, szErrText);

    MessageBox(szInfoMsg, gszWinTitl, MB_ICONEXCLAMATION);
    Update_Display_Log(szInfoMsg);

    return;		

}




void CMainFrame::OnDeviceIoctlMapDeviceRegs()
{
BOOL bRC = TRUE;
char szInfoMsg[150] = "IOCTL: map whole device succeeded.";
char szErrPrefix[]  = "IOCTL: map whole device failed";
ULONG Count;
errno_t errno;
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;

    MadDevIoctlStruct.SecurityKey = gTestGUID;
	bRC = Submit_Ioctl((unsigned long)MADDEV_IOCTL_MAP_VIEWS, 
 	                   (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT),
	                   (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT),
					   &Count, NULL, szErrPrefix); 
	if (!bRC)
		return;

	errno = memcpy_s(&gMadDevMapViews, sizeof(MADDEV_MAP_VIEWS), 
		             MadDevIoctlStruct.DataBufr,  sizeof(MADDEV_MAP_VIEWS));
	sprintf_s(szInfoMsg, 150, "Device mapping:\n MadRegsPA=0x%X:%X, pMadRegs=%p, pPioRead=%p, pPioWrite=%p",
		      gMadDevMapViews.liDeviceRegs.HighPart, gMadDevMapViews.liDeviceRegs.LowPart,
		      gMadDevMapViews.pDeviceRegs, gMadDevMapViews.pPioRead, gMadDevMapViews.pPioWrite); 
	MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
    Update_Display_Log(szInfoMsg);

	return;
}


void CMainFrame::OnDeviceIoctlGetRegs()
{
UINT mbIcon = MB_ICONINFORMATION;
PMADREGS pMadRegs = (PMADREGS)gMadDevMapViews.pDeviceRegs;
char szInfoMsg[MAXTEXTLEN] = "";

    if (pMadRegs == NULL)
	    {
       	sprintf_s(szInfoMsg, MAXTEXTLEN, "Device registers not mapped. Can't display them!");
        mbIcon = MB_ICONERROR;  
	    }
	else
		{
      	sprintf_s(szInfoMsg, MAXTEXTLEN,
			      "MadRegs: MesgID=0x%X, Control=0x%X, Status=0x%X, IntEnable=0x%X, IntId=0x%X",
				  pMadRegs->MesgID, pMadRegs->Control, pMadRegs->Status, pMadRegs->IntEnable, pMadRegs->IntID);
        MessageBox(szInfoMsg, gszWinTitl, mbIcon);
        Update_Display_Log(szInfoMsg);

		sprintf_s(szInfoMsg, MAXTEXTLEN,
			      "MadRegs: PioCacheReadLen=%d, PioCacheWriteLen=%d, ByteIndxRd=%d, ByteIndxWr=%d, CacheIndxRd=%d, CacheIndxWr=%d",
				   pMadRegs->PioCacheReadLen, pMadRegs->PioCacheWriteLen, pMadRegs->ByteIndxRd, pMadRegs->ByteIndxWr, pMadRegs->CacheIndxRd, pMadRegs->CacheIndxWr);
	    }

	MessageBox(szInfoMsg, gszWinTitl, mbIcon);
    Update_Display_Log(szInfoMsg);
 
	return;	
}


void CMainFrame::OnPioRead()
{
//UINT mbIcon = MB_ICONINFORMATION;
UINT     uRC;
size_t   PioLen = 0;
PMADREGS pMadRegs = (PMADREGS)gMadDevMapViews.pDeviceRegs;
PVOID    pPioRead = gMadDevMapViews.pPioRead;
char     szInfoMsg[MAXTEXTLEN] = "";

    if (pMadRegs == NULL)
	    {
       	sprintf_s(szInfoMsg, MAXTEXTLEN, "Device registers not mapped. Can't do PIO read!");
		uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONERROR);
        return;
	    }
   
	sprintf_s(szInfoMsg, MAXTEXTLEN, "Read data from the PIO read region? (%p)", pPioRead);
	uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
    if ((uRC == IDCANCEL) || (uRC == IDNO))
        return;

    memset(gReadBufr, 0x00, READBUFRSIZE);
	PioLen = pMadRegs->PioCacheReadLen;
    memcpy(gReadBufr, pPioRead, PioLen);
	pMadRegs->PioCacheReadLen = 0; // Declare the buffer empty after a read

	gReadBufr[MAXTEXTLEN - 25] = 0x00;
	sprintf_s(szInfoMsg, MAXTEXTLEN, "%d bytes read: %s", (ULONG)PioLen, gReadBufr);
	uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
    Update_Display_Log(szInfoMsg);
 
	return;	
}


void CMainFrame::OnPioWrite()
{
UINT       uRC;
UINT       nDM;
size_t     PioLen = 0;
PMADREGS   pMadRegs = (PMADREGS)gMadDevMapViews.pDeviceRegs;
PVOID      pPioWrite = gMadDevMapViews.pPioWrite;
CDeviceOut DevOutDlg;
char       szWrite_N_Bytes[250] = "";
char       szInfoMsg[MAXTEXTLEN] = "";

    if (pMadRegs == NULL)
	    {
       	sprintf_s(szInfoMsg, MAXTEXTLEN, "Device registers not mapped. Can't do PIO write!");
		uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONERROR);
        return;
	    }
	
	sprintf_s(szInfoMsg, MAXTEXTLEN, "Write data to the PIO write region (%p) ?", pPioWrite);
	uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
    if ((uRC == IDCANCEL) || (uRC == IDNO))
        return;
	
	nDM = DevOutDlg.DoModal(szWrite_N_Bytes);
    if (nDM == IDCANCEL)
        {
        //MessageBox("User hit <Cancel>", gszWinTitl, MB_ICONINFORMATION);
        return;
        }
	
	PioLen = strlen(szWrite_N_Bytes)+1;
    memcpy(pPioWrite, szWrite_N_Bytes, PioLen);
	pMadRegs->PioCacheWriteLen = (ULONG)PioLen; 

	sprintf_s(szInfoMsg, MAXTEXTLEN, "%d bytes written to PIO write region", (ULONG)PioLen);
	uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
    Update_Display_Log(szInfoMsg);
 
	return;	
}


void CMainFrame::OnPioReceive()
{
UINT       uRC;
UINT       nDM;
size_t     PioLen = 0;
PMADREGS   pMadRegs = (PMADREGS)gMadDevMapViews.pDeviceRegs;
PVOID      pPioRead = gMadDevMapViews.pPioRead;
CDeviceOut DevOutDlg;
char       szWrite_N_Bytes[250] = "";
char       szInfoMsg[MAXTEXTLEN] = "";

    if (pMadRegs == NULL)
	    {
       	sprintf_s(szInfoMsg, MAXTEXTLEN, "Device registers not mapped. Can't do PIO write!");
		uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONERROR);
        return;
	    }
	
	sprintf_s(szInfoMsg, MAXTEXTLEN, "Receive data into the PIO read region (%p) ?", pPioRead);
	uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
    if ((uRC == IDCANCEL) || (uRC == IDNO))
        return;
	
	nDM = DevOutDlg.DoModal(szWrite_N_Bytes);
    if (nDM == IDCANCEL)
        {
        //MessageBox("User hit <Cancel>", gszWinTitl, MB_ICONINFORMATION);
        return;
        }
	
	PioLen = strlen(szWrite_N_Bytes)+1;
    memcpy(pPioRead, szWrite_N_Bytes, PioLen);
	pMadRegs->PioCacheReadLen = (ULONG)PioLen; 

	sprintf_s(szInfoMsg, MAXTEXTLEN,
		      "%d bytes received into PIO read region", (ULONG)PioLen);
	uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
    Update_Display_Log(szInfoMsg);
 
	return;		
}


void CMainFrame::OnPioTransmit()
{
UINT     uRC;
size_t   PioLen = 0;
PMADREGS pMadRegs = (PMADREGS)gMadDevMapViews.pDeviceRegs;
PVOID    pPioWrite = gMadDevMapViews.pPioWrite;
char     szInfoMsg[MAXTEXTLEN] = "";

    if (pMadRegs == NULL)
	    {
       	sprintf_s(szInfoMsg, MAXTEXTLEN, "Device registers not mapped. Can't do PIO xmit!");
		uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONERROR);
        return;
	    }

	PioLen = pMadRegs->PioCacheWriteLen;
	sprintf_s(szInfoMsg, MAXTEXTLEN,
		      "Transmit %d bytes from the PIO write region:%p  (PioWrite --> Xmit) ?",
			  (ULONG)PioLen, pPioWrite);
	uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
    if ((uRC == IDCANCEL) || (uRC == IDNO))
        return;

	//Just logically flush the buffer (declare it to be xmitted)
    memset(pPioWrite, 0x00, PioLen);
	pMadRegs->PioCacheWriteLen = 0; // Declare the buffer empty after a xmit

	sprintf_s(szInfoMsg, MAXTEXTLEN, "%d bytes transmitted", (ULONG)PioLen);
	uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
    Update_Display_Log(szInfoMsg);
 
	return;	
}


void CMainFrame::OnPioLoopback()
{
UINT     uRC;
size_t   PioLen = 0;
PMADREGS pMadRegs = (PMADREGS)gMadDevMapViews.pDeviceRegs;
PVOID    pPioRead = gMadDevMapViews.pPioRead;
PVOID    pPioWrite = gMadDevMapViews.pPioWrite;
char     szInfoMsg[MAXTEXTLEN] = "";

    if (pMadRegs == NULL)
	    {
       	sprintf_s(szInfoMsg, MAXTEXTLEN, "Device registers not mapped. Can't do PIO loopback!");
		uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONERROR);
        return;
	    }

	PioLen = pMadRegs->PioCacheWriteLen;
	sprintf_s(szInfoMsg, MAXTEXTLEN, 
		      "Loopback (Xmit|Recv) %d bytes between PIO regions: (PioWrite --> PioRead)", (ULONG)PioLen);
	uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
    if ((uRC == IDCANCEL) || (uRC == IDNO))
        return;

    memcpy(pPioRead, pPioWrite, PioLen);
	pMadRegs->PioCacheReadLen  = (ULONG)PioLen; 
	pMadRegs->PioCacheWriteLen = 0; // Declare the buffer empty after a xmit

	sprintf_s(szInfoMsg, MAXTEXTLEN, "%d bytes transferred", (ULONG)PioLen);
	uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
    Update_Display_Log(szInfoMsg);
 
	return;	
}

//*** Edit Menu Functions ********************************************************
//*
//*** Input some Text from the User and append it to the Client Area Display
//*
void CMainFrame::OnEditAnnotate() 

{
UINT uRC;
char szAnnotateText[MAXTEXTLEN] = "";
CAnnotate AnnotateDlg;

    uRC = AnnotateDlg.DoModal(szAnnotateText);
	if (uRC == IDCANCEL)
		return;


    Update_Display_Log(szAnnotateText);

	return;
}


//*** Select All / Copy to Clipboard 
//*
void CMainFrame::OnEditCopyAll() 

{
register int j;
HGLOBAL hGlobal;
PVOID pGlobal = NULL;
char szClipText[]  = "Here is some Clipboard text.";
//CAbsDevIoDoc AbsDevIoDoc;

	hGlobal = GlobalAlloc((GMEM_SHARE | GMEM_ZEROINIT), MAXTEXTLINES * MAXTEXTLEN);
	if (hGlobal != NULL)
	    pGlobal = GlobalLock(hGlobal);
	if (pGlobal == NULL)
		return;

//*** Insert client area text
//*
    for (j = 0; j < gnCurDsplyCnt; j++)
		{ 
	    strcat_s((PCHAR)pGlobal, MAXTEXTLEN, gDisplayBufr[j]);
		strcat_s((PCHAR)pGlobal, MAXTEXTLEN, "\n");
		}

	GlobalUnlock(pGlobal);

	OpenClipboard();
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hGlobal);
	CloseClipboard();

	return;
}



//*** View Menu Functions ********************************************************
//*
void CMainFrame::OnSetBinaryMode() 

{
    m_bBinaryMode = !m_bBinaryMode;

    return;	
}


//* Clear the client area
//*
void CMainFrame::OnViewClearDisplay() 

{
	ClearClientArea(); 
    return;	
}


//***********************************
//*
void CMainFrame::OnVIEWContinuousIO() 

{
    m_bViewContIO = !m_bViewContIO;

    return;	
}


//*** Help & View Menu functions 
//***
void CMainFrame::OnHelpAbout()
{
	// TODO: Add your command handler code here
}


void CMainFrame::OnViewDevRegMap()
{
	ViewDevRegMap();

	return;
}


void CMainFrame::OnHelpViewDevRegMap()
{
	ViewDevRegMap();
	
	return;
}


void CMainFrame::OnHelpShowReadmefile()
{
CString csWinExec = MadViewReadmeCmd;
UINT wRC;

    #pragma warning(suppress: 28159)
    wRC = ::WinExec(csWinExec.GetBuffer(1), SW_SHOWNORMAL); 

	return;
}


void CMainFrame::OnHelpViewSimulationDiagram()
{
CString csWinExec = MadViewDfdCmd ; //"MSPaint.exe MadSimDFDiagram.bmp ";
UINT wRC;

    #pragma warning(suppress: 28159)
    wRC = ::WinExec(csWinExec.GetBuffer(1), SW_SHOWNORMAL); 

	return;
}

void CMainFrame::OnWmiBusQuery()
{
CString csWinExec = MadBusWmiCmd; 
UINT wRC;

    #pragma warning(suppress: 28159)
	wRC = ::WinExec(csWinExec.GetBuffer(1), SW_SHOWNORMAL);

	return;
}


void CMainFrame::OnWmiDeviceQuery()
{
CString csWinExec = MadDevWmiCmd;
UINT wRC;

    #pragma warning(suppress: 28159)
	wRC = ::WinExec(csWinExec.GetBuffer(1), SW_SHOWNORMAL); 

	return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
void CMainFrame::OnWmiBusQueryBlock()
{
    return;	
}

void CMainFrame::OnWmiBusQueryRegistry()
{
    return;	
}

void CMainFrame::OnWmiBusQueryItem()
{
    return;	
}

void CMainFrame::OnWmiBusSetItem()
{
    return;	
}

void CMainFrame::OnWmiDevQueryBlock()
{
    return;	
}

void CMainFrame::OnWmiDevQueryRegistry()
{
    return;	
}

void CMainFrame::OnWmiDevSetBlock()
{
    return;	
}

void CMainFrame::OnWmiDevSetItem()
{
    return;	
}

void CMainFrame::OnWmiDevQueryDriverBlock()
{
    return;	
}

void CMainFrame::OnWmiDevSetDriverBlock()
{
    return;	
}

void CMainFrame::OnWmiDevExecMethod()
{
    return;	
}

void CMainFrame::OnWmiDevFunctionControl()
{
    return;	
}


void CMainFrame::OnWinDbgAssert()

{
//ULONG nCount;
int IoctlResult = 1;
DWORD dwGLE;
UINT mbIcon = MB_ICONINFORMATION;
char szInfoMsg[100] = "ASSERT IOCTL succeeded.";
char szErrText[50]  = "";

    //IoctlResult = DeviceIoControl(ghDevice, MADDEV_IOCTL_ASSERT, 
    //                             NULL, 0, NULL, 0, &nCount, NULL);
    if (!IoctlResult) // Did the IOCTL succeed?
        {
		dwGLE = GetLastError();
		Assign_GLE_Text(dwGLE, szErrText);
       	sprintf_s(szInfoMsg, 100,  "ASSERT IOCTL failure! GLE=%d:%s ", dwGLE, szErrText);

        mbIcon = MB_ICONEXCLAMATION;  //* The error state
        }

    MessageBox(szInfoMsg, gszWinTitl, mbIcon);
    Update_Display_Log(szInfoMsg);
	
	return;	
}


void CMainFrame::OnWinDbgBreak()

{
//ULONG nCount;
int IoctlResult = 1;
DWORD dwGLE;
UINT mbIcon = MB_ICONINFORMATION;
char szInfoMsg[100] = "BreakPoint IOCTL succeeded.";
char szErrText[50]  = "";

    //IoctlResult = DeviceIoControl(ghDevice, MADDEV_IOCTL_BREAK, 
    //                              NULL, 0, NULL, 0, &nCount, NULL);
    if (!IoctlResult) // Did the IOCTL succeed?
        {
		dwGLE = GetLastError();
		Assign_GLE_Text(dwGLE, szErrText);
       	sprintf_s(szInfoMsg, 100, "BreakPoint IOCTL failure! GLE = %d:%s ",
                dwGLE, szErrText);

        mbIcon = MB_ICONEXCLAMATION;  //* The error state
        }

    MessageBox(szInfoMsg, gszWinTitl, mbIcon);
    Update_Display_Log(szInfoMsg);
	
	return;	
}


void CMainFrame::OnWinDbgException()

{
//ULONG nCount;
int IoctlResult = 1;
//DWORD dwGLE;
UINT mbIcon = MB_ICONINFORMATION;
char szInfoMsg[100] = "Exception IOCTL succeeded.";
char szErrText[50]  = "";

    //IoctlResult = DeviceIoControl(ghDevice, MADDEV_IOCTL_EXCEPTION, 
    //                             NULL, 0, NULL, 0, &nCount, NULL);
    if (IoctlResult) // Did the IOCTL succeed?
        {
		//dwGLE = GetLastError();
		//Assign_GLE_Text(dwGLE, szErrText);
       	sprintf_s(szInfoMsg, 100, "Exception IOCTL succeeded!");
        mbIcon = MB_ICONEXCLAMATION;  //* The error state
        }

    MessageBox(szInfoMsg, gszWinTitl, mbIcon);
    Update_Display_Log(szInfoMsg);
	
	return;	
}


void CMainFrame::OnWinDbgHang()

{
//ULONG nCount;
int IoctlResult = 1;
//DWORD dwGLE;
UINT mbIcon = MB_ICONINFORMATION;
char szInfoMsg[100] = "Hang IOCTL succeeded.";
char szErrText[50]  = "";

    //IoctlResult = DeviceIoControl(ghDevice, MADDEV_IOCTL_HANG, 
    //                              NULL, 0, NULL, 0, &nCount, NULL);
    if (IoctlResult) // Did the IOCTL succeed?
        {
		//dwGLE = GetLastError();
		//Assign_GLE_Text(dwGLE, szErrText);
       	sprintf_s(szInfoMsg, 100, "Hang IOCTL succeeded!");
        mbIcon = MB_ICONEXCLAMATION;  //* The error state
        }

    MessageBox(szInfoMsg, gszWinTitl, mbIcon);
    Update_Display_Log(szInfoMsg);
	
	return;	
}


void CMainFrame::OnUM_Hang()

{
UINT a = 0;

	while (TRUE)
		{
        if (a > 0);
		}; 

    return;
}


void CMainFrame::OnWinDbgVerifier()

{
//ULONG nCount;
int IoctlResult = 1;
//DWORD dwGLE;
UINT mbIcon = MB_ICONINFORMATION;
char szInfoMsg[100] = "Verifier IOCTL succeeded.";
char szErrText[50]  = "";

    //IoctlResult = DeviceIoControl(ghDevice, MADDEV_IOCTL_VERIFIER, 
    //                              NULL, 0, NULL, 0, &nCount, NULL);
    if (IoctlResult) // Did the IOCTL succeed?
        {
		//dwGLE = GetLastError();
		//Assign_GLE_Text(dwGLE, szErrText);
       	sprintf_s(szInfoMsg, 100, "Verifier IOCTL succeeded!");
        mbIcon = MB_ICONEXCLAMATION;  //* The error state
        }

    MessageBox(szInfoMsg, gszWinTitl, mbIcon);
    Update_Display_Log(szInfoMsg);
	
	return;	
}
#endif


//*** Read from the device & store in an open file until a Device I/O error
//* 
void CMainFrame::AppReadFile(int hOutput, PULONG pulPktCount, PULONG pIoCount, BOOL bAllowErrs)

{
#if 0 //#ifndef BLOCK_MODE
    static char EOLine[4] = "\r\n";
#endif

//#define READLIMIT 50
//FILE* pFile;
BOOL bContinue = TRUE;
BOOL bRC;
//register int j;
short int nLineCnt = 0;
short int uRC;
//long int nGLE;
ULONG ReadLenCnt = 0;
//ULONG ulWrite;
DWORD dwGLE;
//long int nLoopLim;
PUCHAR pByte = &gReadBufr[0];
int IoctlResult = 1;
char szInfoMsg[100];
char szErrText[50];
int rc;

    ReadLenCnt = READBUFRSIZE;
//#ifdef BLOCK_MODE
//	ReadLenCnt = MAD_BLOK_SIZE;
//#endif

	Init_MemBlock((PUCHAR)&gReadBufr, ReadLenCnt);
//    IoctlResult = DeviceIoControl(ghDevice, MADDEV_IOCTL_READ, NULL, 0,
//                                  &gReadBufr, READLIMIT, &ReadLenCnt, NULL);
//    if (!IoctlResult) // Did the IOCTL succeed?
	bRC = ReadFile(ghDevice, &gReadBufr, READBUFRSIZE, &ReadLenCnt, NULL);
	if (!bRC) //* > 0
       	{
		if (!bAllowErrs)
			{
			*pIoCount = *pIoCount + ReadLenCnt;
            dwGLE = GetLastError(); 
    		Assign_GLE_Text(dwGLE, szErrText);
 			sprintf_s(szInfoMsg, 100,
				      "Target device read failure: GLE = %d:%s ", dwGLE, szErrText);
			Update_Display_Log(szInfoMsg);

			//uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONSTOP);
			return; 
			}
        }
 
    while (bContinue)
        {
        *pIoCount = *pIoCount + ReadLenCnt;
        (*pulPktCount)++;

   		//ulWrite = fwrite(&gReadBufr, sizeof(CHAR), ReadLenCnt, pFile);
		//nLoopLim = min(READLIMIT, ReadLenCnt);
        //for (j = 0; j < ReadLenCnt; j++)
		//	 uRC = putc((int)gReadBufr[j], pFile);
		rc = _write(hOutput, &gReadBufr, ReadLenCnt);


#if 0 //#ifndef BLOCK_MODE
		if (!m_bBinaryMode) //* Add a CR-LF if it's text
			rc = _write(hOutput, &EOLine, strlen(EOLine));
#endif

    ReadLenCnt = READBUFRSIZE;
    Init_MemBlock((PUCHAR)&gReadBufr, ReadLenCnt);
//      IoctlResult = DeviceIoControl(ghDevice, MADDEV_IOCTL_READ, NULL, 0,
//                                    &gReadBufr, READLIMIT, &ReadLenCnt, NULL);
//		if (!IoctlResult)
    	bRC = ReadFile(ghDevice, &gReadBufr, ReadLenCnt, &ReadLenCnt, NULL);
	    if (!bRC) //* > 0
 			{
			dwGLE = GetLastError();
    		Assign_GLE_Text(dwGLE, szErrText);
            sprintf_s(szInfoMsg, 100, "Read Error. GLE = %d:%s ", dwGLE, szErrText); 
            //uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONEXCLAMATION);
            Update_Display_Log(szInfoMsg);

			if ((!bAllowErrs) || (dwGLE == 121)) //* Timeout
				bContinue = FALSE; //* We're through
			}
		} //* end while

    uRC = _flushall();

	return;
}
