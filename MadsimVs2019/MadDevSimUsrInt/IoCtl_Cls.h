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
/*  Module  NAME : Ioctl_Cls.h                                                 */
/*                                                                             */
/*  DESCRIPTION  : Definition of the Ioctl_Cls class                           */
/*                                                                             */
/*******************************************************************************/


//#include "ntddk.h"
#include "..\Includes\MadDefinition.h" 
#include "..\Includes\MadBusUsrIntBufrs.h" 

class IOCTL_Cls
	{
	public:
	    IOCTL_Cls()
			{
			m_hSimDev    = INVALID_HANDLE_VALUE; 
			m_bIntNeeded = FALSE; // do not generate interrupt
			m_nSerialNo =  (ULONG)-1;
			ZeroMemory(&m_MadDevUsrIntBufr, sizeof(MAD_USRINTBUFR));
			m_MadDevUsrIntBufr.SerialNo = -1;
			m_bOpen = FALSE;
            
#ifdef _DEBUG
			m_bValid = FALSE;
#endif
			}

		~IOCTL_Cls()
			{
			if (!m_bOpen)
			    Close(NULL);
			}

	    BOOL	Open(LONG* nSerialNo);
	    void	Close(PVOID pDevBase);
	    //BOOL	QueryDevice(LPMAD_CNTL pRegs);

	    BOOL	GetDevice(BOOL bRegTrans = TRUE);
 	    void	SetDevice();

//#ifdef BLOCK_MODE
		BOOL 	GetOutput(unsigned char szOutput[], PVOID pPioWrite, LONG Length);
 	    void	SetInput(PVOID pPioRead, unsigned char szInput[], LONG Length);
		BOOL    MapWholeDevice(PVOID *ppDeviceRegs, PVOID *ppPioRead, PVOID *ppPioWrite, PVOID *ppDeviceData);
//#endif

		void	SetReg(UINT registerID, BYTE value);
	    BYTE	GetReg(UINT registerID);
		void	SetReg16(UINT registerID, USHORT uVlue);
	    USHORT 	GetReg16(UINT registerID);

	    void	GenerateInterrupt();
	    void	SetIRQ(UINT irq);

    	//void	SetTrace(DWORD mask);
	    //DWORD	GetTrace();
    	BOOL	InitDriver();
		PVOID   GetUsrIntDataPntr(VOID) {return (PVOID)m_MadDevUsrIntBufr.ucDataBufr;};

	    BOOL	m_bOpen;

	protected:
	    HANDLE		   m_hSimDev;  // simulator handle
		BOOL	       m_bIntNeeded;
		ULONG          m_nSerialNo;
		MAD_USRINTBUFR m_MadDevUsrIntBufr;
//#ifdef BLOCK_MODE
//        BYTE        m_DataBufr[MAD_BLOK_SIZE];
//#endif

		void		DeviceIoControlFailure(UINT id, DWORD err,
		LPDWORD     m_pulBytesRead = NULL, DWORD m_ulBytesExpected = 0);
#ifdef _DEBUG
		BOOL		m_bValid;  	  // helps us with debugging
#endif
	};
