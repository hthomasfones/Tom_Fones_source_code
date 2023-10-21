/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2013 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* XYZ Company                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadBus.sys, MadSimUI.exe                                    */
/*                                                                             */
/*  Module  NAME : MadBusUsrIntBufrs.h                                         */
/*                                                                             */
/*  DESCRIPTION  : Definitions of buffers used in Ioctls between MadBus.sys &  */
/*                 MadSimUI.exe                                                */
/*                                                                             */
/*******************************************************************************/

#define USRINT_BLOK_SIZE (PAGE_SIZE / 2)

typedef struct _MAD_SET_INT 
    {
    ULONG interrupt_Line;
    } MAD_SET_INT, * LPMAD_SET_INT, *PMAD_SET_INT;

typedef struct _MAD_DEBUGMASK 
    {
    ULONG  ulTraceMask;
    } MAD_DEBUGMASK, *LPMAD_DEBUGMASK, *PMAD_DEBUGMASK;

typedef struct _MAD_USRINTBUFR
    {
    USHORT       uInstance; //* Which instance of possibly several device simulation(s)
    USHORT       uTemp;
    ULONG        ulTemp;
    ULONG        WriteRegMask;     // used on write, ignored on read
    LONG         SerialNo;        //* 8-byte align the struct below
	//union {
    //MAD_CNTL     Mad_Cntl;         //* MAD_Definition.h
	//UCHAR        RegArray[100];
	//} u;
    UCHAR            ucDataBufr[64];
    } MAD_USRINTBUFR, *PMAD_USRINTBUFR;

//#ifdef BLOCK_MODE
//typedef struct _MAD_USRINT_BLOKIO_BUFR
//    {
//    MAD_USRINTBUFR   Mad_UsrIntBufr; 
//    UCHAR            ucMoreData[USRINT_BLOK_SIZE];
//    } MAD_USRINT_BLOKIO_BUFR, *PMAD_USRINT_BLOKIO_BUFR;
//#endif

//* DebugMask bits - for control from Simulator UI
//* Must peacefully co-exist w. Debug flags in the Toaster infrastructure
//* See MAD_SimBusDefs.h
//*
#define DEBUGMASK_TRACE_REGISTERS 0x00100000 //
#define DEBUGMASK_TRACE_IOCTLS    0x00200000 //
#define DEBUGMASK_TRACE_INTS      0x00400000 //
#define DEBUGMASK_TRACE_ALL       0x00F00000
#define DEBUGMASK_TRACE_NONE      0x000FFFFF

//* Simulated device Reg Masks for control from Simulator UI
//* We have a 16 byte register bank in the Meta-Abstract Device
//*
#define WriteRegMask_CNTL           0xC000
#define WriteRegMask_CNTL1          0x8000
#define WriteRegMask_CNTL2          0x4000

//* Synonyms
#define WriteRegMask_CMD            0xC000
#define WriteRegMask_CMD1           0x8000
#define WriteRegMask_CMD2           0x4000

#define WriteRegMask_STAT           0x3000
#define WriteRegMask_STAT1          0x2000
#define WriteRegMask_STAT2          0x1000

#define WriteRegMask_INT_ACTV       0x0C00 
#define WriteRegMask_INT_ACTV1      0x0800 
#define WriteRegMask_INT_ACTV2      0x0400 

#define WriteRegMask_INT_ID         0x0300 
#define WriteRegMask_INT_ID1        0x0200 
#define WriteRegMask_INT_ID2        0x0100 

#define WriteRegMask_DATALEN_IN     0x00C0
#define WriteRegMask_DATALEN_IN1    0x0080
#define WriteRegMask_DATALEN_IN2    0x0040

#define WriteRegMask_DATALEN_OUT    0x0030
#define WriteRegMask_DATALEN_OUT1   0x0020
#define WriteRegMask_DATALEN_OUT2   0x0010

#define WriteRegMask_DATA1          0x0008
#define WriteRegMask_DATA2          0x0004
#define WriteRegMask_DATA3          0x0002
#define WriteRegMask_DATA4          0x0001
#define WriteRegMask_POWER_STATE    WriteRegMask_DATA4

#ifdef DMA
#define WriteRegMask_DMA_IN         0xFF000000
#define WriteRegMask_DMAIN_HI_HI    0xC0000000
#define WriteRegMask_DMAIN_HI_LO    0x30000000
#define WriteRegMask_DMAIN_LO_HI    0x0C000000
#define WriteRegMask_DMAIN_LO_LO    0x03000000

#define WriteRegMask_DMA_OUT        0x00FF0000
#define WriteRegMask_DMAOUT_HI_HI   0x00C00000
#define WriteRegMask_DMAOUT_HI_LO   0x00300000
#define WriteRegMask_DMAOUT_LO_HI   0x000C0000
#define WriteRegMask_DMAOUT_LO_LO   0x00030000
#endif

//******************************* Memory Allocation flags **********
//*
#define MAF_DMAOUT  0x00000010
#define MAF_DMAIN   0x00000008
#define MAF_SHARED  0x00000004
#define MAF_CONTIG  0x00000002
#define MAF_LOCKED  0x00000001
#define MAF_PAGED   0x00000000

//* Map of the buffer between the driver & the User Interface program
//* for exchanging Alloc-ed memory addresses
//*
typedef struct _MAD_MEM_ALLOC_ADDRS 
    {
    PVOID            pAllocAddr; 
    PVOID            pMappedAddr; 
    PHYSICAL_ADDRESS liPhysAddr;
    } MAD_MEM_ALLOC_ADDRS, *PMAD_MEM_ALLOC_ADDRS;

//* Map of the buffer between the driver & the User Interface program
//* for exchanging Mem Alloc parameters
//*
typedef struct _MAD_MEM_ALLOC_PARMS 
    {
    LONG                   nNumPages; 
    ULONG                  ulMemAllocFlags; //* See #define's above
    ULONG                  ulNumBytes;
    LONG                   nSerialNo;      //* 8-byte align the struct below
    MAD_MEM_ALLOC_ADDRS sMemAllocAddrs; 
    } MAD_MEM_ALLOC_PARMS, *PMAD_MEM_ALLOC_PARMS;
