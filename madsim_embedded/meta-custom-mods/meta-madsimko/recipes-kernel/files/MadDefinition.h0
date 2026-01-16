/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2015 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */ 
/* XYZ Company                                                                 */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadBus.sys, MadDevice.sys, MadSimUI.exe, MadTestApp.exe,    */ 
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : MadDefinition.h                                             */
/*                                                                             */
/*  DESCRIPTION  : Properties & Definitions for the Model-Abstract Device      */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/

//Configuration-Property definitions ............................................
//
typedef enum  _MAD_DEV_INT_MODE {ePOLLED=0, 
                                 eLEGACY, eLineBased=eLEGACY, eLevelSensitive=eLEGACY,
                                 eMSI, eLATCHED=eMSI}  MAD_DEV_INT_MODE;
//
#define MADDEV_INT_MODE eLEGACY //Not really used - unless the device driver needs to support both 

typedef enum  _MADDEV_IO_TYPE 
              {eNoIO=0, eBufrdRead, eBufrdWrite, eCachedRead, eCachedWrite, 
			   eReadAlign, eWriteAlign, eDmaRead, eDmaWrite, 
			   eSgDmaRead, eSgDmaWrite, eMltplIO, eInvalidIO = eMltplIO} MADDEV_IO_TYPE;

//Names .........................................................................
//
#define MAD_DFLT_LOCALE_ID           0x0409 //USA 
#define MAD_DEVICE_NAME_PREFIX       "MadDevice"
#define MAD_OBJECTNAME_UNITNUM_WSTR  L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_\0"

//* PnP enumeration commands:
//
#define    MadPlugInCmd         "MadEnum.exe -p "
#define    MadUnplugCmd         "MadEnum.exe -u "
#define    MadEjectCmd          "MadEnum.exe -e "
#define    MadPowerCmd          "MadEnum.exe "

// Other commands
//
#define    MadTestAppCmd        "MadTestApp.exe "
#define    MadMonCmd            "MadMonitor.exe "
#define    MadBusWmiCmd         "MadBusWmi.cmd"
#define    MadDevWmiCmd         "MadDevWmi.cmd"
#define    MadRegDefsViewCmd    "Notepad.exe  MadRegDefs.txt"
#define    MadViewReadmeCmd     "Notepad.exe  Readme.Mad.txt"
#define    MadViewDfdCmd        "MSPaint.exe  MadSimDFDiagram.bmp "
#define    MadPnpStressName     "PnpStress.cmd"
#define    MadAutoTestName      "MadTestRWI.lst"
//
#define    MadTestAppLog        "MadTestApp.log"

//General MACRO definitions .................................................................
//
#define BRKPNT DbgBreakPoint();

#define GENERIC_SWITCH_DEFAULTCASE_ASSERT \
	    ASSERTMSG("Undefined case value in switch statement *!*", FALSE); //Always asserts; should present filename:line#

//*** This macro is used in the assert macro Ex: ASSERT(VALID_KERNEL_ADDRESS(pX));
//*
#ifdef _X86_ //* 32-bit world
    #define VALID_KERNEL_ADDRESS(X) (((ULONG_PTR)X > 0x80000000)  &&    \
		                     ((ULONG_PTR)X % sizeof(ULONG) == 0)) 
#else //* Currently treating all 64-bit platforms the same
    #define X64_KERNEL_BASE          0xFF00000000000000  //* Needs research
    #define X64_KERNEL_EXTENT             0x80000000000  //* 8 Terabytes
    #define X64_KERNEL_HI_LIMIT      (X64_KERNEL_BASE + X64_KERNEL_EXTENT)

    //#define VALID_KERNEL_ADDRESS(X) (((ULONG_PTR)X > X64_KERNEL_BASE)      &&  \
    //		 	              ((ULONG_PTR)X < X64_KERNEL_HI_LIMIT)  &&  \
    //		                      ((ULONG_PTR)X % sizeof(ULONG) == 0)) 

    #define VALID_KERNEL_ADDRESS(X)  TRUE  //* If necessary
#endif	

//*** This #define is used like this:
//* #ifdef PCI32
//* ASSERT(liPhysAddr.HighPart == 0L);
//* #endif
//*
//#ifdef _X86_       //* If this is a 32-bit build
//    #define PCI32  //* 32-bit PCI: devices only know 32-bit physical addresses
//#endif

//#define PCI64    //* Maybe someday - but until then ...
//#ifndef PCI64
//    #define PCI32  //* Even #ifdef _X64_ ... (even if this build is for 64-bit Windows)
                   //* PCI devices will only know 32-bit physical addresses
//#endif

#ifdef WIN32 //* We're included by a Windows App - not a WDK Driver 
    // Define the page size for the Intel 386 as 4096 (0x1000). (WDM.h)
    #define PAGE_SIZE 0x1000
    
    // Define a 64-bit quantity known to drivers but not to Win Apps
    typedef struct _PHYSICAL_ADDRESS 
        {
        ULONG LowPart;
        ULONG HighPart;
        } PHYSICAL_ADDRESS;
#endif 

#ifndef min
#define min(_a, _b)     (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef max
#define max(_a, _b)     (((_a) > (_b)) ? (_a) : (_b))
#endif

//Operation-function definitions .................................................................
//
#ifdef  REAL_MODE // Not SIMULATION_MODE ... Buckle-up - we're in uncharted waters
//
#define SIMULATION_ASSERT(x)
//
#else //SIMULATION_MODE
//
    #define SIMULATION_ASSERT(x)  ASSERT(x) //Used as a sanity check to verify things known about the simulation

    #ifndef _WINDOWS //* We're included by a WDK Driver - not a Windows App 
        // Simulator & simulation_mode functions to replace WdfInterruptAcquireLock & WdfInterruptReleaseLock
        //
        #define MAD_ACQUIRE_LOCK_RAISE_IRQL(hLock, HiIRQL, pSvIRQL)  \
                WdfSpinLockAcquire(hLock); KeRaiseIrql(HiIRQL, pSvIRQL);

        #define MAD_LOWER_IRQL_RELEASE_LOCK(LoIRQL, hLock)           \
	            KeLowerIrql(LoIRQL); WdfSpinLockRelease(hLock); 
    #endif

    #ifdef _AMD64_
    #define MAD_KMDF_WONT_CREATE_DMA_ENABLER //Because the kernel won't create the dma adapter for a non-dma-capable bus in 64-bit mode
	                                         //Therefore driver-framework can't create a dma enabler
	                                         //See this article... https://www.osronline.com/ShowThread.cfm?link=182978
    #endif                                    
//
#endif //SIMULATION_MODE

//Logging definitions ............................................................
//
#if DBG
    #pragma message("DBG build!")
    #define BUILD_DATE  __DATE__  
    #define BUILD_TIME  __TIME__
#else
    #define MAD_FREEBUILD_LOGGING // If desirable
    #ifdef  MAD_FREEBUILD_LOGGING //Make KdPrint(s) work as if DBG
        #undef  KdPrint
	    #define KdPrint(_x_) DbgPrint _x_
        //
	    #define BUILD_DATE  "_DATE_" //Because the compiler doesn't define __DATE__
	    #define BUILD_TIME  "_TIME_" //Because the compiler doesn't define __TIME__
    #endif
#endif

#ifdef TRACE_ETW
    #define WPP_TRACING //Defined here because i can't get it to work properly in the VS project(s)
#endif
//#define WPP_TRACING
#ifndef WPP_TRACING   //Revert to KdPrint for WinDbg-over-Firewire / DbgView.exe / etc
    #define TraceEvents(X,Y, ...)   KdPrint((__VA_ARGS__))
#endif

//Power management profile ......................................
//
#undef MAD_POWER_MANAGEMENT_ENABLED
//#define MAD_HOST_CONTROL_POWER_MNGT
#define MAD_DEVICE_CONTROL_POWER_MNGT
//
#ifdef MAD_HOST_CONTROL_POWER_MNGT //HIPM in SATA-speak
    #ifdef MAD_DEVICE_CONTROL_POWER_MNGT //Cause the compilation to abort here 
    #error Define error. MAD_HOST_CONTROL_POWER_MNGT & MAD_DEVICE_CONTROL_POWER_MNGT are mutually exclusive *!*
    #endif 

    #define MAD_IDLE_TIMEOUT    180000; // 3 minute idle timeout in millisecs
    #define MAD_POWER_MANAGEMENT_ENABLED
#endif //MAD_HOST_CONTROL_POWER_MNGT
//
#ifdef MAD_DEVICE_CONTROL_POWER_MNGT  //DIPM in SATA-speak
    #define MAD_DFLT_POWER_CHECK_DELAY  3000 //Millisecs
    #define MAD_POWER_MANAGEMENT_ENABLED   
#endif 

//WMI statistics-related constants
//
#define    MAD_XFER_UNIT_SIZE               1024
#define	   MAD_MILLIWATTS_PER_XFER_UNIT      250
#define    MAD_DMA_MILLIWATTS_PER_XFER_UNIT  500 
#define    MAD_MILLIWATTS_PER_IDLE_SECOND    100

//#define IO_TIMER
#ifdef IO_TIMER
    #define READTIMEOUT       30 //* Seconds
    #define WRITETIMEOUT      30 //* Seconds
    #define PULSETIMEOUT      30 //* Seconds
#endif

//*** Device property definitions ................................................................
//
//WDF_DMA_PROFILE
//
#define  MAD_DMA_ALIGN_REQ   FILE_QUAD_ALIGNMENT
#define  MAD_DMA_PROFILE     WdfDmaProfileScatterGather   //Or WdfDmaProfileScatterGather64
#define  MAD_MEM_CACHE_TYPE  MmNonCached

//Assuming the host must eXclusive-OR Status & IntID register bits on real hardware
//That is the host can't set these bits to one - bits are reset to zero by writing one (X-ORing)
//
#define XOR_REGBITS_SET_BY_DEVICE 

//Configuration Mngt parameters
//
#define MAD_DFLT_DEVICE_IRQL        9
#define MAD_DFLT_IDT_BASE_INDX      0x40 
#define MAD_DFLT_AFFINITY_ON        1 
#define MAD_DFLT_NUM_FILTERS        0
#define MAD_DFLT_DMA_ENABLED        FALSE 
#define MADDEV_LOW_PHYS_ADDR        {0, 0}
#define MADDEV_HIGH_PHYS_ADDR       {0xFFFF0000, 0} //Highest 32-bit page
#define MAD_MAX_DEVICES             9  // 1..9 = SerialNo: (One-digit name suffix may need enhancement)
#define MADDEV_TOTL_CM_RESOURCES    4  // Device registers memory(IoPort), Int Parms, PIO read memory, PIO write memory  

//* Device Memory extents & configuration 
//
#define MAD_SIZEOF_REGISTER_BLOCK    sizeof(MADREGS)
#define MAD_SECTOR_SIZE              512
#define MAD_BLOCK_SIZE               MAD_SECTOR_SIZE  
#define MAD_DEVICE_MAX_SECTORS       64  // A power of two please
#define MAD_DFLT_DATA_EXTENT         (MAD_SECTOR_SIZE * MAD_DEVICE_MAX_SECTORS)
#define MAD_MIN_DATA_EXTENT          (MAD_SECTOR_SIZE * 16)
#define MAD_MAX_DATA_EXTENT          (MAD_SECTOR_SIZE * 2048) //1 MB *?*
#define MAD_CACHE_NUM_SECTORS         1
#define MAD_CACHE_SIZE_BYTES         (MAD_CACHE_NUM_SECTORS * MAD_SECTOR_SIZE)
//
#define MAD_REGISTER_BLOCK_SIZE      MAD_SECTOR_SIZE 
#define MAD_MAPD_READ_OFFSET         MAD_REGISTER_BLOCK_SIZE //MAPD as in mapped
#define MAD_MAPD_READ_SIZE           (MAD_SECTOR_SIZE * 1)  // * N sectors if useful 
#define MAD_MAPD_WRITE_OFFSET        (MAD_MAPD_READ_OFFSET + MAD_MAPD_READ_SIZE) 
#define MAD_MAPD_WRITE_SIZE          MAD_MAPD_READ_SIZE  
//
#define MAD_CACHE_READ_OFFSET        MAD_MAPD_READ_OFFSET
#define MAD_CACHE_WRITE_OFFSET       MAD_MAPD_WRITE_OFFSET
//
#define MAD_DEVICE_DATA_OFFSET       (MAD_MAPD_WRITE_OFFSET + MAD_MAPD_WRITE_SIZE)
#define MAD_INTER_DEVICE_MARGIN      MAD_SECTOR_SIZE  
//
#define MAD_DEVICE_MEM_SIZE_NODATA  \
        (MAD_REGISTER_BLOCK_SIZE + MAD_MAPD_READ_SIZE + MAD_MAPD_WRITE_SIZE)
//
#define MAD_DEVICE_MAP_MEM_SIZE         \
        (MAD_DEVICE_MEM_SIZE_NODATA + MAD_DFLT_DATA_EXTENT)
//
#define MAD_64KB_ALIGNMENT            0x10000
#define MAD_64KB_ALIGN_MASK           0xFFFFFFFF0000

//Device I-O count & size defines  
//
#define MAD_DMA_MAX_SECTORS          16 
#define MAD_DMA_MAX_BYTES            (MAD_DMA_MAX_SECTORS * MAD_SECTOR_SIZE)
//
#define MAD_UNITIO_SIZE_BYTES        16
#define MAD_BYTE_COUNT_MULT          MAD_UNITIO_SIZE_BYTES

// The larger our device extent the coarser the granularity of the cache alignment/indexing
// given that we have N bits (4) for offseting/indexing
//
#define MAD_CACHE_OFFSET_BITS        4
#define MAD_CACHE_OFFSET_RANGE       (1 << MAD_CACHE_OFFSET_BITS) // 2 ^ CACHE_OFFSET_BITS
#define MAD_CACHE_ALIGN_MULTIPLE     (MAD_DEVICE_MAX_SECTORS / MAD_CACHE_OFFSET_RANGE)

//*** Device registers & register mask values ....................................................
//
// Using the Universe_II:Tundra controller chip for the VmeBus as the model for our DMA controller
//
typedef struct _MAD_DMA_CHAIN_ELEMENT
        {
		ULONG64   HostAddr;     //* Physical address of the host data
		ULONG64   DevLoclAddr;  //* Device-relative address for data xfers
        ULONG     DmaCntl;      //* DMA Control 
        ULONG     DXBC;         //* DMA Transfer Byte Count
        ULONG64   CDPP;         //* Chained-DMA Packet Pointer to the next element
        }  MAD_DMA_CHAIN_ELEMENT,  *PMAD_DMA_CHAIN_ELEMENT; 

#ifdef __cplusplus //Because C won't allow this
typedef struct _EMPTY_STRUCT {} EmptyStruct;
#endif

//DMA Controller register mask defines
//
#define MAD_DMA_CNTL_INIT        0x00440044 //Address space & address alignment bits (arbitrary - not really used)
#define MAD_DMA_CNTL_H2D         0x80000000 //Indicates host to device (dma-write)
//
#define MAD_DMA_CDPP_END         0x00000001 //Chained-DMA End-of-list

//*** Define the device registers in memory  
//*
typedef struct _MADREGS
    {
    ULONG    MesgID;
    ULONG    Control;
    ULONG    Status;
    ULONG    IntEnable;
    ULONG    IntID;
    //
    ULONG    PioCacheReadLen;    
	ULONG    PioCacheWriteLen;   
    ULONG    CacheIndxRd;   //Which sector
    ULONG    CacheIndxWr;   //Which sector
    //
    ULONG    ByteIndxRd;
    ULONG    ByteIndxWr;
    //  
    ULONG    PowerState;

// Here we have 64-bit alignment because we have an even # of 32-bit regs above (12)
// (Assuming pagesize(+) alignment of the whole memory allocation)
//  
// This aggregate of registers must match the chain element above so that we can do
// a bulk transfer of registers across the bus. 
// This is NOT workable if the real device register layout doesn't cooperate  
//
	#ifdef __cplusplus
	EmptyStruct      OneBlockDmaRegs; //This has the same offset as the following registers
    #endif
	//
    ULONG64          HostAddr;        //* Physical address of the host data
    ULONG64          DevLoclAddr;     //* Local (system/pci) adddress
    ULONG            DmaCntl;         //* DMA Control-Status
    ULONG            DTBC;            //* DMA Xfer byte count
    ULONG64          BCDPP;           //* Base Chained-DMA Pkt Pntr
    } MADREGS, *PMADREGS;

// Register offsets 	
//*
#define MADREG_MESG_ID        offsetof(MADREGS, MesgId)
#define MADREG_CNTL           offsetof(MADREGS, Control)
#define MADREG_CMD            offsetof(MADREGS, Control)
#define MADREG_CNTL1          offsetof(MADREGS, Control)
#define MADREG_CMD1           offsetof(MADREGS, Control)
#define MADREG_CNTL2          (offsetof(MADREGS, Control)+1)
#define MADREG_CMD2           (offsetof(MADREGS, Control)+1)
#define MADREG_STAT           offsetof(MADREGS, Status)
#define MADREG_STAT1          offsetof(MADREGS, Status)
#define MADREG_STAT2          (offsetof(MADREGS, Status)+1)
#define MADREG_INT_ACTV       offsetof(MADREGS, IntEnable)
#define MADREG_INT_ACTV1      offsetof(MADREGS, IntEnable) 
#define MADREG_INT_ACTV2      (offsetof(MADREGS, IntEnable)+1) 
#define MADREG_INT_ID         offsetof(MADREGS, IntId) 
#define MADREG_INT_ID1        offsetof(MADREGS, IntId) 
#define MADREG_INT_ID2        (offsetof(MADREGS, IntId)+1) 
//
#define MADREG_PIO_READ_LEN    offsetof(MADREGS, PioCacheReadLen)
#define MADREG_PIO_READ_LEN1   offsetof(MADREGS, PioCacheReadLen)
#define MADREG_PIO_READ_LEN2   (offsetof(MADREGS, PioCacheReadLen)+1)
#define MADREG_PIO_WRITE_LEN   offsetof(MADREGS, PioCacheWriteLen)
#define MADREG_PIO_WRITE_LEN1   offsetof(MADREGS, PioCacheWriteLen)
#define MADREG_PIO_WRITE_LEN2  (offsetof(MADREGS, PioCacheWriteLen)+1)
#define MADREG_POWER_STATE     offsetof(MADREGS, PowerState)

//* REGISTER MASK BITS 
//*
#define MAD_INT_BUFRD_INPUT_BIT          0x00000001  //Enable-indicate buffered input
#define MAD_INT_DMA_INPUT_BIT            0x00000002  //Enable-indicate DMA input
#define MAD_INT_ALIGN_INPUT_BIT          0x00000008  //Enable-indicate read cache alignment
#define MAD_INT_INPUT_MASK        \
        (MAD_INT_BUFRD_INPUT_BIT | MAD_INT_DMA_INPUT_BIT | MAD_INT_ALIGN_INPUT_BIT) 
//
#define MAD_INT_BUFRD_OUTPUT_BIT         0x00000100  //Enable-indicate buffered output
#define MAD_INT_DMA_OUTPUT_BIT           0x00000200  //Enable-indicate DMA ioutput
#define MAD_INT_ALIGN_OUTPUT_BIT         0x00000800  //Enable-indicate write cache alignment
#define MAD_INT_OUTPUT_MASK        \
        (MAD_INT_BUFRD_OUTPUT_BIT | MAD_INT_DMA_OUTPUT_BIT | MAD_INT_ALIGN_OUTPUT_BIT) 

#define MAD_INT_STATUS_ALERT_BIT         0x00008000  // The "Oh S**t!" indicator
#define MAD_INT_ALL_VALID_MASK           (MAD_INT_STATUS_ALERT_BIT | MAD_INT_INPUT_MASK | MAD_INT_OUTPUT_MASK)
#define MAD_INT_ALL_INVALID_MASK         ~MAD_INT_ALL_VALID_MASK

#define MAD_ALL_INTS_ENABLED_BITS    \
        (MAD_INT_INPUT_MASK | MAD_INT_OUTPUT_MASK | MAD_INT_STATUS_ALERT_BIT)
#define MAD_ALL_INTS_DISABLED            0x00000000
//
#define MAD_STATUS_NO_ERROR_MASK         0x00000000  //Status
#define MAD_STATUS_GENERAL_ERR_BIT       0x00000001  //Status  
#define MAD_STATUS_OVER_UNDER_ERR_BIT    0x00000002  //Status 
#define MAD_STATUS_DEVICE_BUSY_BIT       0x00000004  //Status 
#define MAD_STATUS_DEVICE_FAILURE_BIT    0x00000008  //Status
#define MAD_STATUS_INVALID_IO_BIT        0x00000100  //Status ... not visible in the UI
#define MAD_STATUS_RESOURCE_ERROR_BIT    0x00000200  //Status ... not visible in the UI
#define MAD_STATUS_BUS_ERROR_BIT         MAD_STATUS_RESOURCE_ERROR_BIT  
#define MAD_STATUS_TIMEOUT_ERROR_BIT     0x00010000  //Status ... not visible in the UI
//
#define MAD_STATUS_READ_COUNT_MASK       0x000000F0  //Count of completed input - up to 16
#define MAD_STATUS_READ_COUNT_SHIFT               4
//
#define MAD_STATUS_WRITE_MASK            0x0000F000  //Count of completed output - up to 16
#define MAD_STATUS_WRITE_SHIFT                   12
#define MAD_STATUS_RW_COUNT_MASK         (MAD_STATUS_READ_COUNT_MASK | MAD_STATUS_WRITE_MASK)
#define MAD_STATUS_DEAD_DEVICE_MASK      (~MAD_STATUS_RW_COUNT_MASK)
//
#define MAD_CONTROL_CACHE_XFER_BIT       0x00000001 //Control: Buffered I/O through the R/W cache 
#define MAD_CONTROL_IOSIZE_BYTES_BIT     0x00000002 //Control: Io count(Size) is in Bytes not sectors
#define MAD_CONTROL_BUFRD_GO_BIT         0x00000008 //Control: BUFRD Go bit
//
#define MAD_CONTROL_CHAINED_DMA_BIT      0x00000040 //Control: DMA is chained
#define MAD_CONTROL_DMA_GO_BIT           0x00000080 //Control: DMA Go bit
#define MAD_CONTROL_RESET_STATE          0x00000000
//
#define MAD_CONTROL_IO_OFFSET_MASK       0x00000F00
#define MAD_CONTROL_IO_OFFSET_SHIFT               8
//
#define MAD_CONTROL_IO_COUNT_MASK        0x0000F000
#define MAD_CONTROL_IO_COUNT_SHIFT               12
//
#define REGISTER_MASK_ALL_BITS_HIGH      0xFFFFFFFF
//
// The generic mask bit defines for the Simulator-UI 
//
#define MADMASK_BIT0             0x0001
#define MADMASK_BIT1             0x0002
#define MADMASK_BIT2             0x0004
#define MADMASK_BIT3             0x0008
#define MADMASK_BIT4             0x0010
#define MADMASK_BIT5             0x0020
#define MADMASK_BIT6             0x0040
#define MADMASK_BIT7             0x0080
//
#define MADMASK_BIT8             0x0100
#define MADMASK_BIT9             0x0200
#define MADMASK_BIT10            0x0400
#define MADMASK_BIT11            0x0800
#define MADMASK_BIT12            0x1000
#define MADMASK_BIT13            0x2000
#define MADMASK_BIT14            0x4000
#define MADMASK_BIT15            0x8000
