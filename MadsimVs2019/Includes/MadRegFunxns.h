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
/*  Exe file ID  : MadBus.sys, MadDevice.sys,                                  */
/*                                                                             */
/*  Module  NAME : MadRegFunxns.h                                              */
/*                                                                             */
/*  DESCRIPTION  : Registry function prototypes & name definitions             */
/*                                                                             */
/*******************************************************************************/

//
#define REG_DBG                  (L"DebugMask")
#define REG_EVENT                (L"LogEvents")
#define REG_BREAK                (L"BreakOnEntry")
#define REG_TIMEOUT              (L"Timeout")
//#define REG_INTLINE              (L"InterruptLine")
//#define REG_DEFAULT_INTLINE      (L"DefaultInterruptLine")
#define REG_DMA_ENABLED          (L"DmaEnabled")
#define REG_DEVIRQL              (L"DeviceIRQL")
#define REG_IDT_BASE             (L"IDTBASE")
#define REG_AFFINITY_ON          (L"AffinityOn")
#define REG_NUM_FILTERS          (L"NumFilters")
#define REG_MAX_DEVICES          (L"MaxDevices")
#define REG_MAD_DATA_EXTENT      (L"DeviceDataExtent")
#define REG_BUF_LOG_ADDRESS      (L"BufferLogicalAddress")
#define REG_BUF_PHYS_ADDRESS_LO  (L"BufferPhysicalAddressLow")
#define REG_BUF_PHYS_ADDRESS_HI  (L"BufferPhysicalAddressHigh")
#define PNP_STARTDEV_IOSTACKLOC  (L"StartDevIoStackLoc")

#define REG_PATHNAME             (L"PathName")
    
#define PARMS_SUBKEY             (L"\\Parameters")
//#define EPHEMERAL_SUBKEY         (L"\\Ephemeral")
#define DEVICES_SUBKEY           (L"Devices")
#define CONFIG_DATA_SUBKEY       (L"\\ConfigData")
#define CONFIG_DATA_VALUE_NAME   (L"ConfigData\0")
#define MAX_REGISTRY_PATH_LEN    240 //?

NTSTATUS MadReadRegistry(IN PDRIVER_OBJECT DriverObject,   // Driver object
                          IN PUNICODE_STRING path,    // base path to keys
                          OUT PULONG debugMask,       // 32-bit binary debug mask
                          OUT PULONG eventLog,        // Boolean: do we log events?
                          OUT PULONG shouldBreak,     // Boolean: break in DriverEntry?
                          OUT PULONG Timeout); 
 
NTSTATUS MadRegReadChunk(IN WDFDEVICE  hDevice,        // Driver object
                         IN PUNICODE_STRING path,    // base path to keys
                         IN PUNICODE_STRING pValueName,
                         IN ULONG  Len,        
                         OUT PUCHAR pReturnData,
                         OUT OPTIONAL PULONG  LenQueried,
                         OUT OPTIONAL PULONG  ValueType);  
                             
NTSTATUS MadRegWriteChunk(IN PDRIVER_OBJECT DriverObject,   // Driver object
                          IN PUNICODE_STRING path,    // base path to keys
                          IN PWCHAR pValueName,
                          IN ULONG  Len,        
                          OUT PUCHAR pBuffer);
                                       
NTSTATUS MadDevSimReadRegistry(IN PDRIVER_OBJECT DriverObject,   // Driver object
                                IN ULONG ulSerialNo,
                                IN PUNICODE_STRING path,    // base path to keys
                                OUT PULONG interruptLine,
                                OUT PULONG interruptIDT,
                                OUT PUCHAR* ppDevRegsAddr,  //* Yes a pointer 2 a pointer
                                OUT PHYSICAL_ADDRESS *registerPhysicalAddress); //,
                                //OUT PIO_STACK_LOCATION* ppIoStackLoc);

NTSTATUS MadInitDevicePath(IN PDRIVER_OBJECT DriverObject,
                            IN PUNICODE_STRING  inPath,    // zero terminated UNICODE_STRING with parameters path 
                            /*UNICODE_STRING* ephemeralRegistryPath,*/
                            UNICODE_STRING* parameterRegistryPath,
                            UNICODE_STRING* registryPathName);

NTSTATUS  MadInitSubkeyPath(PDRIVER_OBJECT DriverObject,
                            PUNICODE_STRING  pInPath,    
                            PUNICODE_STRING pSubkey,
                            PUNICODE_STRING pSubkeyPath);

NTSTATUS MadSaveUnicodeString(OUT PUNICODE_STRING  pDest, IN PUNICODE_STRING pSrc);
