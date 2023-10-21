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
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <dbt.h>
#include <winerror.h>

#include "..\Includes\MadDefinition.h"  
#include "MadTestApp.h"
#include "MadTestAppView.h"
#include "SetRegBits.h"
#include "MainFrm.h"
#include "..\Includes\MadDevIoctls.h"
#include "..\Includes\MadMonitor.h"
#define DEVINFO

extern "C" {
	       #include "..\Includes\MadPnpPmUtils.h"
			}

#ifdef DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern GUID gTestGUID;

extern char gszTitlPrefx[]; 
extern char gszDeviceName[]; 
extern char gszWinTitl[];

extern HANDLE ghDevice;
extern FILE*  gpLogFile;
extern BOOL   gbBinary;
extern short int gnCurDsplyCnt;
//short int gnTextRow;
extern char gDisplayBufr[MAXTEXTLINES][MAXTEXTLEN];
extern unsigned char gWriteBufr[]; 
extern unsigned char gReadBufr[];

//**************************************************************************
//*** Support functions - not Message map functions 
//*

void CMainFrame::RunAutoScript()

{
static char szCmdFile[] = MadAutoTestName;
static ULONG LoopLimit = 999;
ULONG        LoopCount = LoopLimit;
FILE* pFile;
PCHAR pChar;
UINT uRC;
errno_t errno;
BOOL bRC = TRUE;
char szInfoMsg[MAXTEXTLEN];
char szCmdLine[100];
char ParseChar;

    sprintf_s(szInfoMsg, MAXTEXTLEN, "Run the automated test script...%s", szCmdFile);
    uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
    if (uRC == IDCANCEL)
        return;

StartFile:;
    errno = fopen_s(&pFile, szCmdFile, "r");
	if (pFile == NULL)
	    {
        sprintf_s(szInfoMsg, MAXTEXTLEN, "Open failure...errno=%d", errno);
        //uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_ICONINFORMATION));
		Update_Display_Log(szInfoMsg);
		return;
	    }

	pChar = fgets(szCmdLine, 100, pFile);
	while (pChar != NULL)
	    {
        pChar = Find1stNonBlank(szCmdLine); 
		ParseChar = *pChar;
		if (ParseChar != 0x00)
		    {
			pChar++; //Step over 1st char
		    switch (ParseChar)
		        {
		    	case '#':  //Comment
		    	case '\n': //Empty line
					//Nothing to do
					bRC = TRUE;
				    break;

		        case 'R': //Read
				    bRC = ParseReadCmd(pChar);
				    break;

			    case 'W': //Write
				    bRC = ParseWriteCmd(pChar);
				    break;

			    case 'I': //Ioctl
				    bRC = ParseIoctl(pChar);
				    break;

			    case 'L': //Loop
					ClearClientArea(); //Avoid exhausting the display buffer array
					if (gpLogFile != NULL)
						fflush(gpLogFile);
				    fclose(pFile);

					if (LoopCount < LoopLimit) //We are already decrementing
					    {
						LoopCount--;
						if (LoopCount == 0)
							break;
						else
							goto StartFile; //Recycle the whole hog
					    }

					//Get the indicated limit the 1st time we see the LOOP command
			        pChar = Find1stBlank(pChar);
			        if (*pChar == ' ')
				        {
			            pChar = Find1stNonBlank(pChar);
			            if (*pChar != 0x00)
						    {
			                LoopCount = atoi(pChar);
			                if (LoopCount == 0) //Bad numeric / no numeric
				                LoopCount = LoopLimit - 1;
						    }
					    }

				    goto StartFile; //Recycle the whole hog
				    break;

			    default:;
					bRC = FALSE;
		        } //end switch
		    }

		if (!bRC)
		    {
		    sprintf_s(szInfoMsg, MAXTEXTLEN, "Invalid auto command: %s", szCmdLine);
		    Update_Display_Log(szInfoMsg);
		    }

		pChar = fgets(szCmdLine, 100, pFile);
	    } //end while
	
	fclose(pFile);

    return;
}


BOOL CMainFrame::ParseReadCmd(char szCmdLineParms[])
{

    return ParseIoCmd(szCmdLineParms, FALSE);
}


BOOL CMainFrame::ParseWriteCmd(char szCmdLineParms[])
{

    return ParseIoCmd(szCmdLineParms, TRUE);
}


BOOL CMainFrame::ParseIoCmd(char szCmdParms[], BOOL bWrite)

{
static char szTestData[260] =  {"0123456789ABCDEFGHIJKLNMOPQRSTUVWXYZ"};
PCHAR pChar = Find1stBlank(szCmdParms); 
char ParseChar;
ULONG IoSize;
ULONG NumSectors;

    if (pChar == NULL)
		return FALSE;

	pChar = Find1stNonBlank(pChar);
	ParseChar = *pChar;
	if (ParseChar == 0x00)
		return FALSE;

	pChar++; //Step over 1st char
	switch (ParseChar)
	    {
	   	case 'B': //bytes 
			pChar = Find1stBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;
			
			pChar = Find1stNonBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;

			IoSize = atoi(pChar);
			if (IoSize == 0)
				return FALSE;

			if (!bWrite)
			    Read_N_Bytes(IoSize, FALSE); 
			else
				Write_N_Bytes(szTestData, IoSize, FALSE); 
			break;

	    case 'S': //sectors 
			pChar = Find1stBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;

			pChar = Find1stNonBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;

			IoSize = atoi(pChar);
			if (IoSize == 0)
				return FALSE;

            pChar = Find1stBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;

			pChar = Find1stNonBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;

			NumSectors = atoi(pChar);
			if (NumSectors == 0)
				return FALSE;

			if (!bWrite)
			    Read_N_Sectors(IoSize, NumSectors, FALSE); 
			else
				Write_N_Sectors(IoSize, NumSectors, FALSE); 
		    break;

	    default:;
		    return FALSE;
   	    } //end switch

    return TRUE; //That we issued the i/o - not whether it succeeded
}


BOOL CMainFrame::ParseIoctl(char szCmdLineParms[])
{
PCHAR pChar = Find1stBlank(szCmdLineParms); 
char ParseChar;
BOOL bRC;
ULONG    Ioctl;
ULONG IoCount;
ULONG CacheIndx;
char szInfoMsg[MAXTEXTLEN] = "Script command succeeded: IOCTL ";
char szErrPrefix[150]      = "Script command  failed: IOCTL ";
MADDEV_IOCTL_STRUCT MadDevIoctlStruct;

    if (pChar == NULL)
		return FALSE;

	strcat_s(szInfoMsg, MAXTEXTLEN, szCmdLineParms);
	strcat_s(szErrPrefix, 150, szCmdLineParms);

	pChar = Find1stNonBlank(pChar);
	ParseChar = *pChar;
	if (ParseChar == 0x00)
		return FALSE;

	pChar++; //Step over 1st char
	switch (ParseChar)
	    {
	   	case 'I': //Initialize  
			Ioctl = MADDEV_IOCTL_INITIALIZE;
			break;

	   	case 'N': //Non-valid ioctl  
			Ioctl = MADDEV_IOCTL_CACHE_WRITE+69;
			break;

		case 'R': //Read cache   
			Ioctl = MADDEV_IOCTL_CACHE_READ;
			break;

		case 'W': //Write cache   
			Ioctl = MADDEV_IOCTL_CACHE_WRITE;
			break;

	    case 'A': //align  
			pChar = Find1stBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;

			pChar = Find1stNonBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;

			if (*pChar == 'R')
				Ioctl = MADDEV_IOCTL_ALIGN_READ;
			else
			    {
			    if (*pChar == 'W')
				    Ioctl = MADDEV_IOCTL_ALIGN_WRITE;
				else
					return FALSE;
			    }

            pChar = Find1stBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;

			pChar = Find1stNonBlank(pChar);
			if (*pChar == 0x00)
				return FALSE;

			CacheIndx = atoi(pChar);
			if (CacheIndx == 0)
				return FALSE;

            memcpy_s(MadDevIoctlStruct.DataBufr, MAD_SECTOR_SIZE, (PVOID)&CacheIndx, sizeof(ULONG)); 
			break;

	    default:;
		    return FALSE;
   	    } //end switch
	
	MadDevIoctlStruct.SecurityKey = gTestGUID;
	bRC = Submit_Ioctl((unsigned long)Ioctl, 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), 
                       (PUCHAR)&MadDevIoctlStruct, sizeof(MADDEV_IOCTL_STRUCT), &IoCount, 
		     	       szInfoMsg, szErrPrefix, FALSE);

    return TRUE; //That we issued the ioctl - not whether it succeeded
}


//*** General read N bytes - used by several menu functions above
//*
void CMainFrame::Read_N_Bytes(ULONG ReadLen, BOOL bMesgBox) 

{
ULONG IoCount;
UINT uRC;
BOOL bRC;
DWORD dwGLE;
int   IoctlResult = 1;
char szInfoMsg[MAXTEXTLEN];
char szHexData[50] = "";
char szErrText[50] = "";

    if (bMesgBox)
	    {
        sprintf_s(szInfoMsg, MAXTEXTLEN, "Read %d bytes from the target device ...", ReadLen);
        if (ReadLen > 1) //* Multiple
            {
            uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
            if (uRC == IDCANCEL)
                return;
		    }
        }

 	IoCount = ReadLen;
	bRC = ReadFile(ghDevice, &gReadBufr, ReadLen, &IoCount, NULL);
	if (!bRC) //* > 0
        {
        dwGLE = GetLastError(); 
		Assign_GLE_Text(dwGLE, szErrText);
        sprintf_s(szInfoMsg, MAXTEXTLEN,
			      "Read N bytes failed! GLE = %d:%s ", dwGLE, szErrText);
		if (bMesgBox)
       	    MessageBox(szInfoMsg, gszWinTitl, MB_ICONEXCLAMATION);
        Update_Display_Log(szInfoMsg);
        return;
        }

    sprintf_s(szInfoMsg, MAXTEXTLEN,
   	          "Read %d bytes completed; total bytes=%d; data=", ReadLen, IoCount); 

    if (IoCount > 0)
		{
	    gReadBufr[50] = 0x00;
        strcat_s(szInfoMsg, MAXTEXTLEN, (char *)&gReadBufr); //* Should contain null byte

		sprintf_s(szHexData, 50, "\n; Hex[0]=%02X...[Len-4,-3,-2,-1]=%02X.%02X.%02X.%02X", gReadBufr[0],
		          gReadBufr[IoCount-4], gReadBufr[IoCount-3],
			      gReadBufr[IoCount-2], gReadBufr[IoCount-1]);
        strcat_s(szInfoMsg, MAXTEXTLEN, szHexData);
		}

	if (bMesgBox)
	    uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
    Update_Display_Log(szInfoMsg);
    
    return;	
}


//*** General write N bytes - used by several menu functions above
//*
void CMainFrame::Write_N_Bytes(char szNbytes[], ULONG WriteLen, BOOL bMesgBox) 

{
BOOL bRC;
ULONG nCount;
DWORD dwGLE;
int   IoctlResult = 1;
UINT mbIcon = MB_ICONINFORMATION;
UINT uLen;
char szInfoMsg[MAXTEXTLEN] = "Write N bytes completed.\n ";
char szErrText[50];
char szWrite[256];

    strcpy_s(szWrite, 256, szNbytes); //Leave the passed in copy alone
    uLen = (ULONG)strlen(szWrite);

    strcpy_s((char *)gWriteBufr, WRITEBUFRSIZE, szWrite);
	bRC = WriteFile(ghDevice, gWriteBufr, WriteLen, &nCount, NULL);
    if (bRC)   // Did the write succeed?
	    {
		if (uLen > nCount)
			szWrite[nCount] = 0x00; //Terminate the string for display
        sprintf_s(szInfoMsg, MAXTEXTLEN,
		          "Write N bytes completed; %d bytes written ...\n%s", nCount, szWrite);
	    }
    else
        {
        dwGLE = GetLastError(); 
 		Assign_GLE_Text(dwGLE, szErrText);
		sprintf_s(szInfoMsg, MAXTEXTLEN, "Write N bytes failed! GLE=%d: %s ", dwGLE, szErrText);
        mbIcon = MB_ICONEXCLAMATION;
        }

	if (bMesgBox)
   	    MessageBox(szInfoMsg, gszWinTitl, mbIcon);

	Update_Display_Log(szInfoMsg);

	return;
}


void CMainFrame::Read_N_Sectors(ULONG NumSectors, ULONG Indx, BOOL bMesgBox) 
{
ULONG IoCount = (NumSectors * MAD_SECTOR_SIZE);
UINT  uRC;
char  szInfoMsg[MAXTEXTLEN] = "";
char  szHexData[40] = "";

 	BOOL bRC = RW_N_Sectors(NumSectors, Indx, FALSE, bMesgBox);  
    if (bRC) //Display some of the data read
		{
	    gReadBufr[50] = 0x00; //Introduce a null byte to terminate a string
        strcpy_s(szInfoMsg, MAXTEXTLEN, (char *)&gReadBufr); //* Should contain null byte

        sprintf_s(szHexData, 40, "\nHex data = %02X...%02X.%02X.%02X.%02X", gReadBufr[0],
		          gReadBufr[IoCount-4], gReadBufr[IoCount-3],
			      gReadBufr[IoCount-2], gReadBufr[IoCount-1]);
        strcat_s(szInfoMsg, MAXTEXTLEN, szHexData);

		if (bMesgBox)
	        uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
        Update_Display_Log(szInfoMsg);
		}

	return;
}


void CMainFrame::Write_N_Sectors(ULONG NumSectors, ULONG Indx, BOOL bMesgBox)  
{
static UCHAR DataBytes[20] = "0123456789ABCDEF\0";
register ULONG j;
LONG       Mod; 
LONG       BufrIndx;

	//Initialize the data to be written
	//
	for (j = 0; j < NumSectors; j++)
	    {
		Mod = ((Indx+j) % MAD_DMA_MAX_SECTORS);
		BufrIndx = j * MAD_SECTOR_SIZE;
		memset(&(gWriteBufr[BufrIndx]), DataBytes[Mod], MAD_SECTOR_SIZE);
	    }

	BOOL bRC = RW_N_Sectors(NumSectors, Indx, TRUE, bMesgBox);  

	return;
}


BOOL CMainFrame::RW_N_Sectors(ULONG NumSectors, ULONG Indx, BOOL bWrite, BOOL bMesgBox)  
{
static ULONG FileAccess2 = (FILE_GENERIC_READ | FILE_GENERIC_WRITE);
static ULONG FileShare2 = (FILE_SHARE_READ | FILE_SHARE_WRITE);
OVERLAPPED OvrLapd;
ULONG      IoCount;
UINT uRC;
BOOL bRC;
DWORD dwGLE = NO_ERROR;
char szInfoMsg[MAXTEXTLEN];
char szHexData[40] = "";
char szErrText[50] = "";
HANDLE hDevRA = INVALID_HANDLE_VALUE; //random access

    if (bMesgBox)
	    {
		if (!bWrite)
			sprintf_s(szInfoMsg, MAXTEXTLEN,
			          "Read %d sectors from the target device at sector %d ", NumSectors, Indx);
		else
            sprintf_s(szInfoMsg, MAXTEXTLEN,
			          "Write %d sectors to the target device at sector %d ", NumSectors, Indx);
        uRC = MessageBox(szInfoMsg, gszWinTitl, (MB_OKCANCEL | MB_ICONQUESTION));
        if (uRC == IDCANCEL)
            return FALSE;
        }

	hDevRA = ReOpenFile(ghDevice, FileAccess2, FileShare2, FILE_FLAG_OVERLAPPED);
	if (hDevRA == INVALID_HANDLE_VALUE)
	    {
        dwGLE = GetLastError(); 
		Assign_GLE_Text(dwGLE, szErrText);
        sprintf_s(szInfoMsg, MAXTEXTLEN,
			      "ReOpenFile failed! GLE=%d: %s ", dwGLE, szErrText);
		if (bMesgBox)
       	    MessageBox(szInfoMsg, gszWinTitl, MB_ICONEXCLAMATION);
        Update_Display_Log(szInfoMsg);
        return FALSE;
	    }

	memset(&OvrLapd, 0x00, sizeof(OVERLAPPED));
 	IoCount = NumSectors * MAD_SECTOR_SIZE;
    OvrLapd.OffsetHigh = 0;
	OvrLapd.Offset     = (Indx * MAD_SECTOR_SIZE);
	if (!bWrite)
		bRC = ReadFile(hDevRA, gReadBufr, IoCount, NULL, &OvrLapd);
	else
	    bRC = WriteFile(hDevRA, gWriteBufr, IoCount, NULL, &OvrLapd);

	if (!bRC) 
         dwGLE = GetLastError(); 

	if ((dwGLE != NO_ERROR) && (dwGLE != ERROR_IO_PENDING))
	    {
		Assign_GLE_Text(dwGLE, szErrText);

		if (!bWrite)
			 sprintf_s(szInfoMsg, MAXTEXTLEN,
			           "Initiate indexed read N sectors failed! GLE=%d: %s ",
					   dwGLE, szErrText);
		else
            sprintf_s(szInfoMsg, MAXTEXTLEN,
			          "Initiate indexed write N sectors failed! GLE=%d: %s ",
					  dwGLE, szErrText);
		if (bMesgBox)
       	    MessageBox(szInfoMsg, gszWinTitl, MB_ICONEXCLAMATION);
        Update_Display_Log(szInfoMsg);

		CloseHandle(hDevRA);
        return FALSE;
        }

	bRC = GetOverlappedResult(hDevRA, &OvrLapd, &IoCount, TRUE);
	if (!bRC) //* > 0
        {
        dwGLE = GetLastError(); 
		Assign_GLE_Text(dwGLE, szErrText);
		if (!bWrite)
			 sprintf_s(szInfoMsg, MAXTEXTLEN,
			           "Read-indexed N sectors failed! GLE=%d: %s ", dwGLE, szErrText);
		else
            sprintf_s(szInfoMsg, MAXTEXTLEN,
			          "Write-indexed N sectors failed! GLE=%d: %s ", dwGLE, szErrText);
		if (bMesgBox)
       	    MessageBox(szInfoMsg, gszWinTitl, MB_ICONEXCLAMATION);
        Update_Display_Log(szInfoMsg);

		CloseHandle(hDevRA);
        return FALSE;
        }

	if (!bWrite)
		sprintf_s(szInfoMsg, MAXTEXTLEN,
   	              "Read-indexed %d sectors at sector %d: total bytes = %d\n",
				  NumSectors, Indx, IoCount); 
	else
        sprintf_s(szInfoMsg, MAXTEXTLEN,
   	              "Write-indexed %d sectors at sector %d: total bytes = %d\n",
				  NumSectors, Indx, IoCount); 

 	if (bMesgBox)
	    uRC = MessageBox(szInfoMsg, gszWinTitl, MB_ICONINFORMATION);
    Update_Display_Log(szInfoMsg);

	CloseHandle(hDevRA);

	return TRUE;
}


//****
void CMainFrame::Update_Display_Log(PCHAR pChar) 

{
	if (gpLogFile != NULL)
	    {
		fputs(pChar, gpLogFile);
		fputs("\n", gpLogFile);
	    }

    strcpy_s(gDisplayBufr[gnCurDsplyCnt], MAXTEXTLEN,  pChar); 
    gnCurDsplyCnt++;
 
    InvalidateRect(NULL, TRUE);

    return;
}


//* Clear the client area
//*
void CMainFrame::ClearClientArea() 

{
register int j;
//LRESULT lResult; 

    for (j = 0; j < MAXTEXTLINES; j++)
        memset(gDisplayBufr[j], ' ', MAXTEXTLEN);

    gnCurDsplyCnt = 0;
    
    InvalidateRect(NULL, TRUE);

    return;	
}


//* Support function for the Plug-N-Play Enumerate functions  
//*
void  CMainFrame::Exec_PnP_Enumerate(CString csWinExec)

{
WORD wRC, wMB;
char szInfoMsg[100];

    #pragma warning(suppress: 28159)
    wRC = ::WinExec(csWinExec.GetBuffer(1), SW_MINIMIZE); 
	if (wRC < 32) //* Error
		{
		sprintf_s(szInfoMsg, 100, "WinExec returned %d, command = %s", wRC, csWinExec.GetBuffer(1));
		wMB = MessageBox(szInfoMsg, NULL, MB_ICONSTOP);
		}

    return;
}


//* Support function for the Power Mngt functions  
//*
void  CMainFrame::Exec_PowerMngt_Funxn(CString csSuffix)

{
WORD wRC, wMB;
CString csWinExec = MadPowerCmd; //MadEnum.exe ";
char szInfoMsg[100];

    csWinExec = csWinExec + csSuffix;
    #pragma warning(suppress: 28159)
    wRC = ::WinExec(csWinExec.GetBuffer(1), SW_MINIMIZE); 
	if (wRC < 32) //* Error
		{
		sprintf_s(szInfoMsg, 100, "WinExec returned %d, command = %s", wRC, csWinExec.GetBuffer(1));
		wMB = MessageBox(szInfoMsg, NULL, MB_ICONSTOP);
		}

    return;
}


HANDLE CMainFrame::DeviceOpen(ULONG SerialNo, ULONG OpenFlags) 

{
static char DecDigits[] = "0123456789x";
BOOLEAN bRC = TRUE;
char szObjectName[MAXTEXTLEN-50] = "\\\\?\\\0";
char szInfoMsg[MAXTEXTLEN] = "Target device opened by ";
char szOpenInfo[50] = "";
UINT mbIcon = MB_ICONINFORMATION;
UINT nLen;
UINT rc;
char szSerial[5];
errno_t errno;
HANDLE hDevice = INVALID_HANDLE_VALUE;

//UNICODE_STRING     UniStrPhysMem;
//RtlInitUnicodeString(&UniStrPhysMem,  L"\\Device\\PhysicalMemory");

    strcat_s(szObjectName, MAXTEXTLEN-50, gszDeviceName);  
    errno = _itoa_s(SerialNo, szSerial, 3, 10);
    strcat_s(szObjectName, MAXTEXTLEN-50, szSerial);  
	//rc = MessageBox(szObjectName, gszWinTitl, mbIcon);
	strcpy_s(szOpenInfo, 50, "symbolic link/dos name: ");
    strcat_s(szInfoMsg, MAXTEXTLEN, szOpenInfo);  
    strcat_s(szInfoMsg, MAXTEXTLEN, szObjectName);  
    hDevice = CreateFile(szObjectName, OpenFlags, (FILE_SHARE_READ | FILE_SHARE_WRITE),
					     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
        {
		PDEVICE_INFO pDevInfo = (PDEVICE_INFO)malloc(sizeof(DEVICE_INFO));
		if (pDevInfo == NULL)
			return INVALID_HANDLE_VALUE;

		pDevInfo->ulSerialNo = -1; 
        strcpy_s(szInfoMsg, MAXTEXTLEN, "Target device opened by ");
		strcpy_s(szOpenInfo, 50, "registered GUID: \n");
        strcat_s(szInfoMsg, MAXTEXTLEN, szOpenInfo);  
        bRC = GetPnpDevice((LPGUID)&gTestGUID, SerialNo, pDevInfo);
        if (!bRC)
			{
			sprintf_s(szInfoMsg, MAXTEXTLEN, "Device Not Found! device #%d", SerialNo);
            mbIcon = MB_ICONSTOP;
	        //rc = MessageBox(szInfoMsg, gszWinTitl, mbIcon);
			free(pDevInfo);
			goto LogAndQuit;
			}

        strcpy_s(szObjectName, MAXTEXTLEN-50, pDevInfo->DevicePath);
	    rc = MessageBox(szObjectName, gszWinTitl, mbIcon);
        strcat_s(szInfoMsg, MAXTEXTLEN, szObjectName);  

	    errno = _itoa_s(SerialNo, szSerial, 5, 10);
		hDevice = CreateFile(szObjectName, OpenFlags,
			 	  	         (FILE_SHARE_READ | FILE_SHARE_WRITE),
							 NULL, OPEN_EXISTING, 0, NULL);
		free(pDevInfo);
	    }

	if (hDevice == INVALID_HANDLE_VALUE)
		{
		sprintf_s(szInfoMsg, MAXTEXTLEN,
			      "Target device open failed!\n%s,%d", szObjectName, SerialNo); 
        mbIcon = MB_ICONSTOP;
		}
	else
		{
		strcpy_s(gszWinTitl, WIN_TITL_SIZE, gszTitlPrefx);
		strcat_s(gszWinTitl, WIN_TITL_SIZE, "device x");
		nLen = (ULONG)strlen(gszWinTitl);
		gszWinTitl[nLen-1] = DecDigits[SerialNo];
		CFrameWnd::SetWindowText(gszWinTitl);
		}

LogAndQuit:;
	if (mbIcon != MB_ICONINFORMATION) //* Only hang an error Mesg box
	    rc = MessageBox(szInfoMsg, gszWinTitl, mbIcon);

	if (OpenFlags == GENERIC_READ)
		strcat_s(szInfoMsg, MAXTEXTLEN, "read-only access"); 

	if (OpenFlags == GENERIC_WRITE)
		strcat_s(szInfoMsg, MAXTEXTLEN, "write-only access"); 

    Update_Display_Log(szInfoMsg);

    return hDevice;	
}


//* Initialize a block of memory very distinctively
//*
void CMainFrame::Init_MemBlock(PUCHAR pMemBlock, ULONG ulLen)

{
static ULONG ulInit = 0xEFBEADDE; //* In hex reads like DEADBEEF 
register ULONG j;
PULONG pCurrPntr = (PULONG)pMemBlock;
ULONG uNumInits = ulLen / 4;  //* or sizeof(LONG)

    for (j = 0; j < uNumInits; j++)
		{
        *pCurrPntr = ulInit;
		pCurrPntr++;  //* Increments sizeof LONG
		}
        
    return;
}


BOOL CMainFrame::Submit_Ioctl(ULONG ulIoctl, PUCHAR pWrite, ULONG WriteLen,
							  PUCHAR pRead, ULONG ReadLen, PULONG pIoCount,
							  char szInfoText[], char szErrPrefix[], 
							  BOOL bMesgBox) 

{
UINT mbIcon = MB_ICONINFORMATION;
int  IoctlResult;
DWORD dwGLE;
BOOL bRC;
char szInfoMsg[MAXTEXTLEN] = "";
char szErrText[50];

    if (szInfoText != NULL)
        strcpy_s(szInfoMsg, MAXTEXTLEN, szInfoText);

    IoctlResult = DeviceIoControl(ghDevice, ulIoctl, pWrite, WriteLen,
		                          pRead, ReadLen, pIoCount, NULL);
    bRC = (BOOL)(IoctlResult > 0);
    if (!bRC) // Did the Io Control succeed?
        {
        dwGLE = GetLastError(); 
		Assign_GLE_Text(dwGLE, szErrText);
		if (szErrPrefix != NULL)
		    sprintf_s(szInfoMsg, MAXTEXTLEN, "%s... GLE=%d: %s", szErrPrefix, dwGLE, szErrText);
		else
			sprintf_s(szInfoMsg, MAXTEXTLEN, "Ioctl error GLE=%d: %s", dwGLE, szErrText);
        mbIcon = MB_ICONEXCLAMATION;
        }

    if (bMesgBox)
		if (strlen(szInfoMsg) > 0)
     	    MessageBox(szInfoMsg, gszWinTitl, mbIcon);

	if (strlen(szInfoMsg) > 0)
        Update_Display_Log(szInfoMsg);

	return bRC;
}


BOOL CMainFrame::CreateProcessInClientArea(char szAppName[], char szCmdLine[],
		                                   LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                           LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                           BOOL bInheritHndls,   DWORD dwCreateFlags,
		                                   LPVOID lpEnvironment,
								           LPCTSTR lpCurrentDirectory)

{
BOOL                bRC;
UINT                RC;
char                szInfoMsg[100] = "";
char                szErrMsg[50] = "";
DWORD               dwGLE = 0;
RECT                Rectangl;
STARTUPINFO         StartInfo;
PROCESS_INFORMATION ProcInfo;


//* Initialize a StartInfo struct
//*
   memset(&StartInfo, 0x00, sizeof(STARTUPINFO)); 
   StartInfo.cb          = sizeof(STARTUPINFO);
   StartInfo.dwFlags     = (STARTF_USESHOWWINDOW|STARTF_USEPOSITION|STARTF_USESIZE); 
   StartInfo.wShowWindow = SW_SHOWNORMAL; 

//* Position & size exactly onto the parent client area
//*
    GetWindowRect(&Rectangl);

//* Position   
    StartInfo.dwX     = Rectangl.left + 10;  //Approximately against the left border
    StartInfo.dwY     = Rectangl.top  + 60; //Approximately below the title bar & one menu line 

//* Size 
    StartInfo.dwXSize = Rectangl.right  - Rectangl.left + 20; 
    StartInfo.dwYSize = Rectangl.bottom - Rectangl.top; 

    #pragma warning(suppress: 6335)
    bRC = CreateProcess(szAppName, szCmdLine,
		                lpProcessAttributes, lpThreadAttributes,
						bInheritHndls, dwCreateFlags,
		                lpEnvironment, lpCurrentDirectory,
						&StartInfo, &ProcInfo);
	if (!bRC)
        {
        dwGLE = GetLastError();
		Assign_GLE_Text(dwGLE, szErrMsg);
        sprintf_s(szInfoMsg, 100, "CreateProcess returned %d:%s ", dwGLE, szErrMsg);
        RC = MessageBox(szInfoMsg, NULL, MB_ICONSTOP);
        }

	return bRC;
}


/* **************************************************************************/
void CMainFrame::ViewDevRegMap()
{
CString csWinExec = MadRegDefsViewCmd;
UINT wRC;

    #pragma warning(suppress: 28159)
    wRC = ::WinExec(csWinExec.GetBuffer(1), SW_SHOWNORMAL); 

	return;
}


//*** Assign error text for a GetLastError returned value
//*
void CMainFrame::Assign_GLE_Text(DWORD dwGLE, char szErrText[])

{
	switch (dwGLE)
		{
		case ERROR_INVALID_FUNCTION:
			strcpy_s(szErrText, 50, "Invalid function ");
			break;

		case ERROR_INVALID_HANDLE:
			strcpy_s(szErrText, 50, "Invalid handle...(file opened?)");
			break;

		case ERROR_NOT_READY:
			strcpy_s(szErrText, 50, "Device not ready / Device power failure");
			break;

		case ERROR_BAD_COMMAND:
			strcpy_s(szErrText, 50, "Bad command / Invalid device state");
			break;

		case ERROR_CRC:
			strcpy_s(szErrText, 50, "CRC error ");
			break;

		case ERROR_SECTOR_NOT_FOUND:
			strcpy_s(szErrText, 50, "Error sector not found");
			break;

		case ERROR_GEN_FAILURE:
			strcpy_s(szErrText, 50, "GENERAL FAILURE");
			break;

		case ERROR_INVALID_PARAMETER:
			strcpy_s(szErrText, 50, "INVALID PARAMETER");
			break;

		case ERROR_ADAP_HDW_ERR:
			strcpy_s(szErrText, 50, "Hardware error");
			break;

		case ERROR_SEM_TIMEOUT:
			strcpy_s(szErrText, 50, "TIMEOUT");
			break;

		case ERROR_BUSY:
			strcpy_s(szErrText, 50, "DEVICE BUSY");
			break;

		case ERROR_IO_DEVICE:
			strcpy_s(szErrText, 50, "DEVICE IO ERROR");
			break;

		case ERROR_NO_DATA_DETECTED:
			strcpy_s(szErrText, 50, "No data detected / Device power failure");
			break;

		case ERROR_NO_SYSTEM_RESOURCES:
			strcpy_s(szErrText, 50, "No System Resources");
			break;

		case ERROR_INVALID_ID_AUTHORITY:
			strcpy_s(szErrText, 50, "Invalid security (Interface GUID)");
			break;

		default:
			strcpy_s(szErrText, 50, "*Unrecognized error*");
		}

	return;
}
