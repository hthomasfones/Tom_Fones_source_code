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
/*  Exe file ID  : MadBus.sys, MadDevice.sys, MadSimUI.exe, MadTestApp.exe,    */ 
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : MadDefinition.h                                             */
/*                                                                             */
/*  DESCRIPTION  : Properties & Definitions for the Model-Abstract Device      */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/

#ifndef _MADDEFS_

#define _MAD_SIMULATION_MODE_  //Comment this out when your hardware arrives 
#ifndef _MAD_SIMULATION_MODE_
    #define _MAD_REAL_HARDWARE_MODE_
#endif //_MAD_SIMULATION_MODE_

//PCI VendorId:DeviceID defines
#define MAD_VENDOR_ID         0x9808 //Write it out on paper & then rotate it upside down
#define MAD_DEVICE_ID_GENERIC 0x1001
#define MAD_DEVICE_ID_DISK    0x2001
#define MAD_SUBVENDOR_ID      MAD_VENDOR_ID
#define MAD_SUBSYSTEM_ID      MAD_DEVICE_ID_GENERIC

//Configuration-Property definitions ............................................
typedef enum  _MAD_DEV_INT_MODE 
              {ePOLLED=0, eLEGACY, eLineBased=eLEGACY, eLevelSensitive=eLEGACY,
               eMSI, eLATCHED=eMSI}  MAD_DEV_INT_MODE;
//
#define MADDEV_INT_MODE eLEGACY //Not really used - unless the device driver needs to support both 

typedef enum  _MADDEV_IO_TYPE 
              {eNoIO=0, eBufrdRead, eBufrdWrite, eCachedRead, eCachedWrite, 
			   eReadAlign, eWriteAlign, eDmaRead, eDmaWrite, 
			   eSgDmaRead, eSgDmaWrite, eMltplIO, eInvalidIO = eMltplIO}
              MADDEV_IO_TYPE;

//Names .........................................................................
#define MAD_DFLT_LOCALE_ID           0x0409 //USA 
#define MAD_DEVICE_NAME_PREFIX       "MadDevice"
#define MAD_OBJECTNAME_UNITNUM_WSTR  L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_\0"

//* PnP enumeration commands:
#define    MadPlugInCmd         "MadEnum.exe -p "
#define    MadUnplugCmd         "MadEnum.exe -u "
#define    MadEjectCmd          "MadEnum.exe -e "
#define    MadPowerCmd          "MadEnum.exe "

// Other commands
#define    MadTestAppCmd        "MadTestApp.exe "
#define    MadMonCmd            "MadMonitor.exe "
#define    MadBusWmiCmd         "MadBusWmi.cmd"
#define    MadDevWmiCmd         "MadDevWmi.cmd"
#define    MadRegDefsViewCmd    "Notepad.exe  MadRegDefs.txt"
#define    MadViewReadmeCmd     "Notepad.exe  Readme.Mad.txt"
#define    MadViewDfdCmd        "MSPaint.exe  MadSimDFDiagram.bmp "
#define    MadPnpStressName     "PnpStress.cmd"
#define    MadAutoTestName      "MadTestRWI.lst"

#define    MadTestAppLog        "MadTestApp.log"

//General MACRO definitions .................................................................
#define BRKPNT DbgBreakPoint();

#define GENERIC_SWITCH_DEFAULTCASE_ASSERT \
        ASSERTMSG("This switch statement received an invalid case value! ", FALSE);
       //Always asserts; should present filename:line#    

#define MAD_PNP_MINOR_DV_INVALID 0xFF  //Driver verifier invalid minor pnp function

#define Trace_Guid(guid) \
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, \
                    "Guid = {%08lX - %04hX - %04hX - %02hhX %02hhX - %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX}\n", \
                    guid.Data1, guid.Data2, guid.Data3, \
                    guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], \
                    guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

//*** This macro is used in the assert macro Ex: ASSERT(VALID_KERNEL_ADDRESS(pX));
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

//Operation-function definitions ..............................................
#define MAD_DEVICE_TYPE_GENERIC      FILE_DEVICE_UNKNOWN
#define MAD_BUS_INTERFACE_TYPE       PCIBus
#define MAD_GUID_BUS_TYPE            GUID_BUS_TYPE_PCI
#define FUNCTION_TYPE_FROM_CTL_CODE(CtlCode) ((CtlCode & 0x00003FFC) >> 2)

#ifdef _MAD_SIMULATION_MODE_
    //#ifdef _AMD64_
    //Always in simulation mode unless we add 32-bit (x86) builds
    //Because the kernel won't create the dma adapter for a non-dma-capable bus in 64-bit mode
    #define MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
    //#endif

    #define SIMULATION_ASSERT(x) ASSERT(x) //Used to verify things known about the simulation

    #ifndef _WINDOWS //* We're included by a WDK Driver - not a Windows App 
        // Simulator & simulation_mode functions to replace WdfInterruptAcquireLock & WdfInterruptReleaseLock
         #define MAD_ACQUIRE_LOCK_RAISE_IRQL(hLock, HiIRQL, pSvIRQL)  \
                    WdfSpinLockAcquire(hLock); KeRaiseIrql(HiIRQL, pSvIRQL);

         #define MAD_LOWER_IRQL_RELEASE_LOCK(LoIRQL, hLock)           \
	                 KeLowerIrql(LoIRQL); WdfSpinLockRelease(hLock); 
    #endif //!_WINDOWS

    #define MAD_VIRTUAL_MINIPORT
    #ifdef  MAD_VIRTUAL_MINIPORT
        #define MAD_DEVICE_TYPE_DISK         FILE_DEVICE_VIRTUAL_DISK
        #define MAD_BUS_INTERFACE_TYPE_SCSI  Internal 
        #define MAD_GUID_BUS_TYPE_SCSI       GUID_BUS_TYPE_INTERNAL
    #else
        #define MAD_DEVICE_TYPE_DISK         FILE_DEVICE_DISK 
        #define MAD_BUS_INTERFACE_TYPE_SCSI  PCIBus  
        #define MAD_GUID_BUS_TYPE_SCSI       GUID_BUS_TYPE_PCI 
    #endif

#else //_MAD_REAL_HARDWARE_MODE_  
    #define MAD_DEVICE_TYPE_DISK         FILE_DEVICE_DISK
    #define MAD_BUS_INTERFACE_TYPE_SCSI  PCIBus 
    #define MAD_GUID_BUS_TYPE_SCSI       GUID_BUS_TYPE_PCI

    #define MAD_ACQUIRE_LOCK_RAISE_IRQL(...)
    #define MAD_LOWER_IRQL_RELEASE_LOCK(...) 
    #define SIMULATION_ASSERT(x)
#endif //_MAD_SIMULATION_MODE_

//Logging definitions ............................................................
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
    #define WPP_TRACING //
    #pragma message("WPP_TRACING defined!")
#endif

//#define WPP_TRACING
#ifndef WPP_TRACING   //Revert to KdPrint for WinDbg-over-Firewire / DbgView.exe / etc
#ifdef _MADDISK_H_
    #define TRACE_LEVEL_NONE        0   // Tracing is not on
    #define TRACE_LEVEL_FATAL       1   // Abnormal exit or termination
    #define TRACE_LEVEL_ERROR       2   // Severe errors that need logging
    #define TRACE_LEVEL_WARNING     3   // Warnings such as allocation failure
    #define TRACE_LEVEL_INFORMATION 4   // Includes non-error cases(e.g.,Entry-Exit)
    #define TRACE_LEVEL_VERBOSE     5   // Deta
    #define TraceEvents(X,Y, ...)   StorPortDebugPrint(X, __VA_ARGS__)
#else
    #define TraceEvents(X,Y, ...)   KdPrint((__VA_ARGS__))
#endif
#endif

//Power management profile ......................................
#undef POWER_MANAGEMENT_ENABLED
//#define MAD_HOST_CONTROL_POWER_MNGT
//#define MAD_DEVICE_CONTROL_POWER_MNGT
#ifdef MAD_HOST_CONTROL_POWER_MNGT
    #ifdef MAD_DEVICE_CONTROL_POWER_MNGT
	//Cause the compilation to abort here 
    #error Define error. MAD_HOST_CONTROL_POWER_MNGT & MAD_DEVICE_CONTROL_POWER_MNGT are mutually exclusive *!*
    #endif 

    #define MAD_IDLE_TIMEOUT    180000; // 3 minute idle timeout in millisecs
#define POWER_MANAGEMENT_ENABLED
#endif //MAD_HOST_CONTROL_POWER_MNGT

#ifdef MAD_DEVICE_CONTROL_POWER_MNGT
    #define MAD_DFLT_POWER_CHECK_DELAY  3000 //Millisecs
    #define POWER_MANAGEMENT_ENABLED   
#endif //MAD_DEVICE_CONTROL_POWER_MNGT

#define    MAD_XFER_UNIT_SIZE               1024
#define	   MAD_MILLIWATTS_PER_XFER_UNIT      250
#define    MAD_DMA_MILLIWATTS_PER_XFER_UNIT  500 // Size independent
#define    MAD_MILLIWATTS_PER_IDLE_SECOND    100

//#define IO_TIMER
#ifdef IO_TIMER
    #define READTIMEOUT       30 //* Seconds
    #define WRITETIMEOUT      30 //* Seconds
    #define PULSETIMEOUT      30 //* Seconds
#endif

//*** Device property definitions ................................................................
//WDF_DMA_PROFILE
#define  MAD_DMA_ALIGN_REQ   FILE_QUAD_ALIGNMENT
#define  MAD_DMA_PROFILE     WdfDmaProfileScatterGather   //Or WdfDmaProfileScatterGather64
#define  MAD_MEM_CACHE_TYPE  MmNonCached 
#define  MAD_MEM_MAPIO_FLAGS (PAGE_READWRITE | PAGE_NOCACHE)

//Assuming the host must eXclusive-OR Status & IntID register bits on real hardware
//That is the host can't set these bits to one - bits are reset to zero by writing one (X-ORing)
#define XOR_REGBITS_SET_BY_DEVICE 

//Configuration Mngt parameters
#define MAD_DFLT_DEVICE_IRQL        9
#define MAD_DFLT_IDT_BASE_INDX    0x40 
#define MAD_DFLT_AFFINITY_ON        1 
#define MAD_DFLT_NUM_FILTERS        0
#define MAD_DFLT_DMA_ENABLED        FALSE 
#define MADDEV_LOW_PHYS_ADDR     {0, 0}
#define MADDEV_HIGH_PHYS_ADDR    {0xFFFF0000, 0} //Highest 32-bit page
#define MAD_MAX_DEVICES             9  // 1..9 = SerialNo: (One-digit name suffix may need enhancement)
#define MAD_NUM_RESRC_LISTS        1  //Unless it's a multi-funxn device 
#define MADDEV_TOTAL_CM_RESOURCES    4  // Device registers memory(IoPort), Int Parms, PIO read memory, PIO write memory  
#define MADDEV_NUM_MEMORY_RANGES    (MADDEV_TOTAL_CM_RESOURCES - 1)

//* Device Memory extents & configuration 
#define MAD_SIZEOF_REGISTER_BLOCK    sizeof(MADREGS)
#define MAD_SECTOR_SIZE              512
#define MAD_BLOCK_SIZE               MAD_SECTOR_SIZE  
#define MAD_DEVICE_MAX_SECTORS       4096  // A power of two please
#define MAD_DEFAULT_DATA_EXTENT      (MAD_SECTOR_SIZE * MAD_DEVICE_MAX_SECTORS)
#define MAD_MIN_DATA_EXTENT          (MAD_SECTOR_SIZE * 16)
#define MAD_MAX_DATA_EXTENT          (MAD_SECTOR_SIZE * 2048) //1 MB *?*
#define MAD_CACHE_NUM_SECTORS         1
#define MAD_CACHE_SIZE_BYTES         (MAD_CACHE_NUM_SECTORS * MAD_SECTOR_SIZE)

//MAPD as in mapped
#define MAD_REGISTER_BLOCK_SIZE      MAD_SECTOR_SIZE 
#define MAD_MAPD_READ_OFFSET         MAD_REGISTER_BLOCK_SIZE
#define MAD_MAPD_READ_SIZE           (MAD_SECTOR_SIZE * 1)  // * N sectors if useful 
#define MAD_MAPD_WRITE_OFFSET        (MAD_MAPD_READ_OFFSET + MAD_MAPD_READ_SIZE) 
#define MAD_MAPD_WRITE_SIZE           MAD_MAPD_READ_SIZE  

#define MAD_CACHE_READ_OFFSET        MAD_MAPD_READ_OFFSET
#define MAD_CACHE_WRITE_OFFSET       MAD_MAPD_WRITE_OFFSET

#define MAD_DEVICE_DATA_OFFSET       (MAD_MAPD_WRITE_OFFSET + MAD_MAPD_WRITE_SIZE)
#define MAD_INTER_DEVICE_MARGIN      MAD_SECTOR_SIZE  

#define MAD_DEVICE_MEM_SIZE_NODATA  \
        (MAD_REGISTER_BLOCK_SIZE + MAD_MAPD_READ_SIZE + MAD_MAPD_WRITE_SIZE)

#define MAD_DEVICE_MAP_MEM_SIZE         \
        (MAD_DEVICE_MEM_SIZE_NODATA + MAD_DEFAULT_DATA_EXTENT)

#define MAD_64KB_ALIGNMENT            0x10000
#define MAD_64KB_ALIGN_MASK           0xFFFFFFFF0000

//Device I-O count & size defines  
#define MAD_DMA_MAX_SECTORS          16 
#define MAD_DMA_MAX_SGLIST_SIZE      16 
#define MAD_DMA_MAX_BYTES            (MAD_DMA_MAX_SGLIST_SIZE * MAD_SECTOR_SIZE)

#define MAD_UNITIO_SIZE_BYTES        16
#define MAD_BYTE_COUNT_MULT          MAD_UNITIO_SIZE_BYTES

// The larger our device extent the coarser the granularity of the cache alignment/indexing
// given that we have N bits (4) for offseting/indexing
#define MAD_CACHE_OFFSET_BITS        4
#define MAD_CACHE_OFFSET_RANGE       (1 << MAD_CACHE_OFFSET_BITS) // 2 ^ CACHE_OFFSET_BITS
#define MAD_CACHE_ALIGN_MULTIPLE     (MAD_DEVICE_MAX_SECTORS / MAD_CACHE_OFFSET_RANGE)

//*** Device registers & register mask values ..........................................................
// Using the Universe II - Tundra controller chip for the VmeBus as the model for our DMA controller
typedef struct _MAD_DMA_CHAIN_ELEMENT
        {
		ULONG64   HostAddr;     //* Physical address of the host data
		ULONG64   DevLoclAddr;  //* Device-relative address for data xfers
        ULONG     DmaCntl;      //* DMA Cntl
        ULONG     DTBC;         //* DMA Total Byte Count
        ULONG64   CDPP;         //* Chained-DMA Packet Pointer to the next element
        }  MAD_DMA_CHAIN_ELEMENT,  *PMAD_DMA_CHAIN_ELEMENT; 

#ifdef __cplusplus //Because C won't allow this
typedef struct _EMPTY_STRUCT {} EmptyStruct;
#endif

//DMA Controller register mask defines
#define MAD_DMA_CNTL_INIT        0x00440044 //Address space & address alignment bits (arbitrary - not really used)
#define MAD_DMA_CNTL_H2D         0x80000000 //Indicates host to device (write)
//
#define MAD_DMA_CDPP_END         0x00000001 //Chained-DMA End-of-list

//*** Define the device registers in memory  
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
	//#ifdef __cplusplus
	//EmptyStruct      OneBlockDmaRegs; //This has the same offset as the following registers
    //#endif
	//
    //ULONG64          HostAddr;        //* Physical address of the host data
    //ULONG64          DevLoclAddr;     //* Local (system/pci) adddress
    //ULONG            DmaCntl;         //* DMA Control-Status
    //ULONG            DTBC;            //* DMA Xfer byte count
    //ULONG64          BCDPP;           //* Base Chained-DMA Pkt Pntr
    MAD_DMA_CHAIN_ELEMENT DmaChainItem0;
    } MADREGS, *PMADREGS;

// Register offsets 	
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

#define MADREG_PIO_READ_LEN    offsetof(MADREGS, PioCacheReadLen)
#define MADREG_PIO_READ_LEN1   offsetof(MADREGS, PioCacheReadLen)
#define MADREG_PIO_READ_LEN2   (offsetof(MADREGS, PioCacheReadLen)+1)
#define MADREG_PIO_WRITE_LEN   offsetof(MADREGS, PioCacheWriteLen)
#define MADREG_PIO_WRITE_LEN1   offsetof(MADREGS, PioCacheWriteLen)
#define MADREG_PIO_WRITE_LEN2  (offsetof(MADREGS, PioCacheWriteLen)+1)
#define MADREG_POWER_STATE     offsetof(MADREGS, PowerState)

//* REGISTER MASK BITS 
#define MAD_INT_BUFRD_INPUT_BIT      0x00000001  //Enable-indicate buffered input
#define MAD_INT_DMA_INPUT_BIT        0x00000002  //Enable-indicate DMA input
#define MAD_INT_ALIGN_INPUT_BIT      0x00000008  //Enable-indicate read cache alignment
#define MAD_INT_INPUT_MASK        \
        (MAD_INT_BUFRD_INPUT_BIT | MAD_INT_DMA_INPUT_BIT | MAD_INT_ALIGN_INPUT_BIT) 

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
#define MAD_ALL_INTS_DISABLED        0x00000000

#define MAD_STATUS_NO_ERROR_MASK         0x00000000  //Status
#define MAD_STATUS_GENERAL_ERR_BIT       0x00000001  //Status  
#define MAD_STATUS_OVER_UNDER_ERR_BIT    0x00000002  //Status 
#define MAD_STATUS_DEVICE_BUSY_BIT       0x00000004  //Status 
#define MAD_STATUS_DEVICE_FAILURE_BIT    0x00000008  //Status
#define MAD_STATUS_IO_ERROR_MASK         0x0000000F
#define MAD_STATUS_INVALID_IO_BIT        0x00000100  //Status ... not visible in the UI
#define MAD_STATUS_RESOURCE_ERROR_BIT    0x00000200  //Status ... not visible in the UI
#define MAD_STATUS_BUS_ERROR_BIT         MAD_STATUS_RESOURCE_ERROR_BIT  
#define MAD_STATUS_TIMEOUT_ERROR_BIT     0x00010000  //Status ... not visible in the UI

#define MAD_STATUS_READ_COUNT_MASK       0x000000F0  //Count of completed input - up to 16
#define MAD_STATUS_READ_COUNT_SHIFT               4

#define MAD_STATUS_WRITE_MASK      0x0000F000  //Count of completed output - up to 16
#define MAD_STATUS_WRITE_SHIFT             12
#define MAD_STATUS_RW_COUNT_MASK         (MAD_STATUS_READ_COUNT_MASK | MAD_STATUS_WRITE_MASK)
#define MAD_STATUS_DEAD_DEVICE_MASK      (~MAD_STATUS_RW_COUNT_MASK)

#define MAD_CONTROL_CACHE_XFER_BIT       0x00000001 //Control: Buffered I/O through the R/W cache 
#define MAD_CONTROL_IOSIZE_BYTES_BIT     0x00000002 //Control: Io count(Size) is in Bytes not sectors
#define MAD_CONTROL_BUFRD_GO_BIT         0x00000008 //Control: BUFRD Go bit

#define MAD_CONTROL_CHAINED_DMA_BIT      0x00000040 //Control: DMA is chained
#define MAD_CONTROL_DMA_GO_BIT           0x00000080 //Control: DMA Go bit
#define MAD_CONTROL_RESET_STATE          0x00000000

#define MAD_CONTROL_IO_OFFSET_MASK       0x00000F00
#define MAD_CONTROL_IO_OFFSET_SHIFT               8

#define MAD_CONTROL_IO_COUNT_MASK        0x0000F000
#define MAD_CONTROL_IO_COUNT_SHIFT               12

#define GO_BITS_MASK                    \
        (MAD_CONTROL_DMA_GO_BIT | MAD_CONTROL_BUFRD_GO_BIT)
#define REGISTER_MASK_ALL_BITS_HIGH  0xFFFFFFFF

// The generic mask bit defines for the Simulator-UI 
#define MADMASK_BIT0             0x0001
#define MADMASK_BIT1             0x0002
#define MADMASK_BIT2             0x0004
#define MADMASK_BIT3             0x0008
#define MADMASK_BIT4             0x0010
#define MADMASK_BIT5             0x0020
#define MADMASK_BIT6             0x0040
#define MADMASK_BIT7             0x0080

#define MADMASK_BIT8             0x0100
#define MADMASK_BIT9             0x0200
#define MADMASK_BIT10            0x0400
#define MADMASK_BIT11            0x0800
#define MADMASK_BIT12            0x1000
#define MADMASK_BIT13            0x2000
#define MADMASK_BIT14            0x4000
#define MADMASK_BIT15            0x8000

#define _MADDEFS_
#endif  //_MADDEFS_