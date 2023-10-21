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
/*  Module  NAME : IoCtl_Cls.cpp                                               */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"

#include <stdlib.h>
#include <wtypes.h>
#include <setupapi.h>
#include <winioctl.h>

#include "resource.h"
#include "IoCtl_Cls.h"  //#includes "MadDefinition.h", MadBusUsrIntBufrs.h 
#include "..\Includes\MadBusIoctls.h" 
#include "MadDevSimUsrInt.h"
#include "format.h"
//#include "MadBusIoctls.h" 

//extern "C" 
//		{
extern  HANDLE MadDevSimOpen(void); //* MadDevSimOpen
//		}

//
/****************************************************************************
*   						   IOCTL_Cls::Open
* Result: BOOL
*   	TRUE if handle open successfully
*	FALSE if device open failed
* Effect: 
*   	Opens the Model-Abstract-Demo device simulator
*       1st try the NT4.0 non-PnP driver
*        Then search for the PnP BUS driver if necessary  
****************************************************************************/

BOOL IOCTL_Cls::Open(PLONG pnSerialNo)

{
UINT uRC;
char szWinExec[100] = MadPlugInCmd;
char szInfoMsg[200];
char szSerNum[10];
BOOL bRC;
	
	m_bOpen = TRUE;
		
    m_hSimDev = MadDevSimOpen();
	if (m_hSimDev == INVALID_HANDLE_VALUE)
		{
		/* failed */
		m_bOpen = FALSE;
		return FALSE;
		} /* failed */

#ifdef _DEBUG
	m_bValid = FALSE;
#endif

//* Determine our device serial number to plug in
	bRC = InitDriver();
	if (!bRC) 
	    {
		sprintf_s(szInfoMsg, 200, "InitDriver failure! ");
    	uRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONEXCLAMATION);
		m_bOpen = FALSE;
        return FALSE;
	    }

	sprintf_s(szInfoMsg, 200, "InitDriver success ");
	//uRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

//* Plugin our device
//*	
	m_nSerialNo = (ULONG)m_MadDevUsrIntBufr.SerialNo;
	//ASSERT(m_nSerialNo > 0);
	if ((m_nSerialNo < 1) || (m_nSerialNo > MAD_MAX_DEVICES))
	    {
		sprintf_s(szInfoMsg, 200,
			      "Assigning Serial# failed  (%d)!\nNo more seats available on the bus.\nCarry on with the existing passengers :)\n\n(Max number of bus slots available: N-1 for N processors)",
			      m_nSerialNo);
    	uRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONEXCLAMATION);
		m_bOpen = FALSE;
	    }
	else
	    {
    	_itoa_s(m_nSerialNo, szSerNum, 10, 10);
    	strcat_s(szWinExec, 100, szSerNum);
        //uRC = MessageBox(NULL, szWinExec, NULL, MB_ICONINFORMATION);
#pragma warning(suppress: 28159)
        uRC = ::WinExec(szWinExec, SW_MINIMIZE); 
     	sprintf_s(szInfoMsg, 200, "WinExec returns %d: GLE=%d", uRC, GetLastError());
        //bRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);
    	if (uRC >= 32)
	    	*pnSerialNo = m_nSerialNo;
        else
	    	{
	    	sprintf_s(szInfoMsg, 200, "WinExec failure!: GLE=%d", GetLastError());
        	uRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONERROR);
	    	m_bOpen = FALSE;
	    	}
	    }

	return m_bOpen;
}


/****************************************************************************
*   						  IOCTL_Cls::Close
* Result: void
*   	
* Effect: 
*   	Closes the handle if it is open
****************************************************************************/

void IOCTL_Cls::Close(PVOID pDevBase)

{
UINT uRC;
char szWinExec[100] = MadUnplugCmd; 
char szSerNum[5];
static DWORD dwSize = sizeof(MAD_USRINTBUFR);
DWORD dwBytesRead = 0;
BOOL bResult;
char szInfoMsg[100];
MAD_USRINTBUFR TempBufr;
PMADBUS_MAP_WHOLE_DEVICE pMapWholeDevice = (PMADBUS_MAP_WHOLE_DEVICE)TempBufr.ucDataBufr;

	if (m_hSimDev == INVALID_HANDLE_VALUE)
	    {
		sprintf_s(szInfoMsg, 100,
			      "IOCTL_Cls::Close: m_hSimDev == INVALID_HANDLE_VALUE - no action");
		::MessageBox(NULL, szInfoMsg, NULL, MB_ICONERROR);
		return;
	    }
	else
	    {
		if (pDevBase != NULL) //We have device area mapping to unmap
			{
	        memset(&TempBufr, 0xFF, dwSize);
	        TempBufr.WriteRegMask = 0; // nothing to write
	        TempBufr.SerialNo = m_nSerialNo; //* Must identify device instance by serial no
		    pMapWholeDevice->pDeviceRegs = pDevBase;
	        bResult = DeviceIoControl(m_hSimDev, IOCTL_MADBUS_UNMAP_WHOLE_DEVICE,
	    			  	              &TempBufr, dwSize,  
				  	                  &TempBufr, dwSize, &dwBytesRead, NULL); 
	        if (!bResult )
		        {
		        sprintf_s(szInfoMsg, 100,
			              "IOCTL_Cls::UnMapWholeDevice: failed! GLE=%d, Handle=%p, pDevBase=%p",
						  GetLastError(), m_hSimDev, pDevBase);
		        ::MessageBox(NULL, szInfoMsg, NULL, MB_ICONERROR);
		        }
		    }

		CloseHandle(m_hSimDev);
		m_hSimDev = INVALID_HANDLE_VALUE;
		}

#ifdef _DEBUG
	m_bValid = FALSE;
#endif

//* Unplug our device
//*	
	_itoa_s(m_nSerialNo, szSerNum, 5, 10);
	strcat_s(szWinExec, 100, szSerNum);
    //uRC = ::MessageBox(NULL, szWinExec, NULL, MB_ICONINFORMATION);
#pragma warning(suppress: 28159)
    uRC = ::WinExec(szWinExec, SW_MINIMIZE); 

    return;
}


/****************************************************************************
*   						 IOCTL_Cls::InitDriver
* Result: DWORD
*   	Current trace flag, or 0 if simulator not valid
****************************************************************************/

BOOL IOCTL_Cls::InitDriver()

{
//ULONG dwTemp;
BOOL bResult;
DWORD dwBytesRead; 

	if (m_hSimDev == INVALID_HANDLE_VALUE)
		return 0;

	bResult = DeviceIoControl(m_hSimDev, IOCTL_MADBUS_INITIALIZE,
				  	          &m_MadDevUsrIntBufr, sizeof(MAD_USRINTBUFR), 
				  	          &m_MadDevUsrIntBufr, sizeof(MAD_USRINTBUFR), &dwBytesRead, NULL);	
	return bResult;
}


BOOL IOCTL_Cls::MapWholeDevice(PVOID* ppDeviceRegs, PVOID* ppPioRead, PVOID* ppPioWrite, PVOID* ppDeviceData)

{
static DWORD dwSize = sizeof(MAD_USRINTBUFR);
DWORD dwBytesRead = 0;
BOOL bResult;
BOOL bRC = TRUE;
DWORD   GLE;
//DWORD GetDevRetry = 3;
MAD_USRINTBUFR TempBufr;
PMADBUS_MAP_WHOLE_DEVICE pMapWholeDevice = (PMADBUS_MAP_WHOLE_DEVICE)TempBufr.ucDataBufr;
char szInfoMsg[250];

	if (m_hSimDev == INVALID_HANDLE_VALUE)
	    {
		sprintf_s(szInfoMsg, 250,
			      "IOCTL_Cls::MapWholeDevice: m_hSimDev == INVALID_HANDLE_VALUE - returning FALSE");
		::MessageBox(NULL, szInfoMsg, NULL, MB_ICONERROR);
		return FALSE;
	    }

//GetDeviceLoop:;
	memset(&TempBufr, 0xFF, dwSize);
	TempBufr.WriteRegMask = 0; // nothing to write
	TempBufr.SerialNo = m_nSerialNo; //* Must identify device instance by serial no
	bResult = DeviceIoControl(m_hSimDev, IOCTL_MADBUS_MAP_WHOLE_DEVICE,
				  	          &TempBufr, dwSize,  
				  	          &TempBufr, dwSize, &dwBytesRead, NULL); 
	if (!bResult || (dwBytesRead != dwSize))
		{
		GLE = GetLastError();
		sprintf_s(szInfoMsg, 250,
			      "IOCTL_Cls::MapWholeDevice: failed! GLE=%d, Handle=%p", GLE, m_hSimDev);
		::MessageBox(NULL, szInfoMsg, NULL, MB_ICONERROR);
		bRC = FALSE;
		}

    sprintf_s(szInfoMsg, 250, "Device mapping: Device_PA,Regs,PioRead,PioWrite,Data = 0x%X:%X\n%p, %p, %p, %p\n",
		      pMapWholeDevice->liDeviceRegs.HighPart, pMapWholeDevice->liDeviceRegs.LowPart,
		      pMapWholeDevice->pDeviceRegs, pMapWholeDevice->pPioRead,
			  pMapWholeDevice->pPioWrite, pMapWholeDevice->pDeviceData);
    //::MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

	//Save local copies of device pointers
	*ppDeviceRegs = pMapWholeDevice->pDeviceRegs;
	*ppPioRead    = pMapWholeDevice->pPioRead;
	*ppPioWrite   = pMapWholeDevice->pPioWrite;
	*ppDeviceData = pMapWholeDevice->pDeviceData;

	return bRC;
}


/****************************************************************************
*   				  IOCTL_Cls::DeviceIoControlFailure
* Inputs:
*   	UINT id: IDS_ entry of message
*	DWORD err: Error code from GetLastError
*	LPDWORD BytesRead: Pointer to number of bytes read, or NULL if not
*			meaningful (default NULL)
*	DWORD BytesExpected: If BytesRead is non-NULL, this is the number
*			of bytes expected; if BytesRead is NULL, this is 
*			ignored (default 0)
* Result: void
*   	
* Effect: 
*   	Issues error message
****************************************************************************/

void IOCTL_Cls::DeviceIoControlFailure(UINT id, DWORD err, LPDWORD BytesRead,
	                                   DWORD BytesExpected)

{
CString s; 
UINT uid = id;

#pragma warning(suppress: 6031)
	s.LoadString(IDS_DEVICEIO_FAILED_WRITE);
	if (err != 0)
		{
		/* additional information */
		s += _T("\r\n");
		s += formatError(err);
		} /* additional information */

	if (BytesRead != NULL && *BytesRead != BytesExpected)
		{
		/* wrong length */
		CString fmt;
#pragma warning(suppress: 6031)
		fmt.LoadString(IDS_WRONG_LENGTH_READ);
		CString t;
		t.Format(fmt, BytesExpected, *BytesRead);
		s += _T("\r\n");
		s += t;
		} /* wrong length */

	AfxMessageBox(s, MB_ICONERROR | MB_OK);

    return;
}


#if 0
/****************************************************************************
*   						 IOCTL_Cls::GetTrace
* Result: DWORD
*   	Current trace flag, or 0 if simulator not valid
****************************************************************************./

DWORD IOCTL_Cls::GetTrace()

{
PMAD_DEBUGMASK pMask = (PMAD_DEBUGMASK)&(m_MadDevUsrIntBufr.ucDataBufr);
DWORD BytesReturned;
BOOL bRC;

	if (m_hSimDev == INVALID_HANDLE_VALUE)
		return 0;

	bRC = DeviceIoControl(m_hSimDev, IOCTL_MADBUS_GET_TRACE,
		  	              NULL, 0, &m_MadDevUsrIntBufr, sizeof(MAD_USRINTBUFR),
				  	      &BytesReturned, NULL);
	if (!bRC)
		DeviceIoControlFailure(IDS_DEVICEIO_FAILED_GET_TRACE, GetLastError());

	return pMask->ulTraceMask;
}

/****************************************************************************
*   						 IOCTL_Cls::SetTrace
* Inputs:
*   	DWORD trace: Trace flags to set
* Result: void
*   	
* Effect: 
*   	Sets the trace flags if there is a valid simulator
****************************************************************************./

void IOCTL_Cls::SetTrace(DWORD trace)

{
PMAD_DEBUGMASK pMask = (PMAD_DEBUGMASK)&(m_MadDevUsrIntBufr.ucDataBufr);

	if (m_hSimDev == INVALID_HANDLE_VALUE)
		return;

	pMask->ulTraceMask = trace;

	DWORD RequiredButNotMeaningful; // the name says it all
	BOOL result = DeviceIoControl(m_hSimDev, IOCTL_MADBUS_SET_TRACE,
				  	              &m_MadDevUsrIntBufr, sizeof(MAD_USRINTBUFR),
				  	              NULL, 0, &RequiredButNotMeaningful, NULL);
	if (!result)
		DeviceIoControlFailure(IDS_DEVICEIO_FAILED_SET_TRACE, GetLastError());

	return;
}
/* */
#endif

