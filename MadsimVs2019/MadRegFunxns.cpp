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
/*  Exe file ID  : MadBus.sys, MadDevice.sys                                   */
/*                                                                             */
/*  Module  NAME : MadRegFunxns.cpp                                            */
/*                                                                             */
/*  DESCRIPTION  : Definitions of registry query functions for the drivers     */
/*                                                                             */
/*******************************************************************************/

#include ".\MadBus\MadBus.h"
//#include <ntddk.h>
//#include <wdf.h>

#define BYTE UCHAR
#define REG_POOL_TAG ' ptR'

extern "C" {
#include "Includes\MadDefinition.h"
#include "Includes\MadRegFunxns.h" 

#ifdef WPP_TRACING
    #ifdef MADDEVICE
    #include "MadDevice\trace.h"
    #else
    #include "Madbus\trace.h"
    #endif
	#include "MadRegFunxns.tmh"
#endif
}
 
/**************************************************************************//**
 * MadInitSubkeyPath
 *
 * DESCRIPTION:
 *    This function initializes a unicode string to the parameters subkey 
 *    from input components.
 *    
 * PARAMETERS: 
 *     @param[in]  pDriverObj   pointer to the driver object for the driver
 *     @param[in]  pInPath     pointer to the unicode string containing the 
 *                             service key
 *     @param[in]  pSubkey     pointer to unicode string containing for the 
 *                             subkey 
 *     @param[in]  pSubkeyPath pointer to the unicode string containing the 
 *                             returned assembled path
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
NTSTATUS MadInitSubkeyPath(PDRIVER_OBJECT   pDriverObj,
                           PUNICODE_STRING  pInPath,     
                           PUNICODE_STRING  pSubkey,
                           PUNICODE_STRING  pSubkeyPath)
{
NTSTATUS  ret = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pDriverObj);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadInitSubkeyPath...enter\n");

    if (pInPath == NULL) 
        return STATUS_INVALID_PARAMETER; // parmx ?
    if (pSubkey == NULL) 
        return STATUS_INVALID_PARAMETER; // parmx ?
    if (pSubkeyPath == NULL) 
        return STATUS_INVALID_PARAMETER; // parmx ?

// We want  a UNICODE String long enough for the Registry path length with the Parameters key
//
    pSubkeyPath->MaximumLength = (USHORT)(pInPath->Length + pSubkey->Length + sizeof(WCHAR)) ;
    pSubkeyPath->Buffer = 
    (PWSTR)ExAllocatePoolWithTag(NonPagedPoolNx, 
                                 (SIZE_T)pSubkeyPath->MaximumLength + 4,
                                 REG_POOL_TAG);
    if (pSubkeyPath->Buffer == NULL) 
        return STATUS_INSUFFICIENT_RESOURCES;

// ?? TO DO ?? add checking on return pointer here
//
// Since the registry path parameter is a "counted" UNICODE string, it
// might not be zero terminated.  For a very short time allocate memory
// to hold the Registry path zero-terminated so that we can use it to
// delve into the Registry.
//
    RtlCopyUnicodeString(pSubkeyPath, pInPath);
    RtlAppendUnicodeStringToString(pSubkeyPath, pSubkey);

//  Null terminate for some uses (but don't increase the 'length' or
//  the UNICODE_STRING version will have a 0 on the end.
//
    pSubkeyPath->Buffer[pSubkeyPath->Length / sizeof(UNICODE_NULL)] = UNICODE_NULL;

    return ret;
}

/************************************************************************//**
 * MadSaveUnicodeString 
 *
 * DESCRIPTION:
 *    This function copies one unicode string to another (the main use being
 *    making a permanent copy of the driver's service key).
 *    
 * PARAMETERS: 
 *     @param[in]  pDest       pointer to the destination unicode string
 *     @param[in]  pSrc        pointer to the source unicode string
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadSaveUnicodeString(OUT PUNICODE_STRING pDest, IN PUNICODE_STRING pSrc)
{
// We want  a UNICODE String long enough for the Registry path length with the Parameters key
//
    pDest->MaximumLength = MAX_REGISTRY_PATH_LEN;
    pDest->Length        = pSrc->Length + sizeof(WCHAR);

    pDest->Buffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPoolNx,
		                                         MAX_REGISTRY_PATH_LEN, REG_POOL_TAG);
    if (pDest->Buffer == NULL)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadSaveUnicodeString...can't get a buffer!\n");
        return STATUS_NO_MEMORY;
        }

    RtlCopyUnicodeString(pDest, pSrc);

    return STATUS_SUCCESS;
}

/************************************************************************//**
 * MadRegReadChunk 
 *
 * DESCRIPTION:
 *    This function reads a chunk of data from the registry 
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  pRegPath    pointer to the registry path
 *     @param[in]  pValueName  pointer to the value name
 *     @param[in]  Len         length of the chunk to read
 *     @param[in]  pReturnData pointer to the returned data
 *     @param[in]  pLenQueried pointer to the returned length queried
 *     @param[in]  pValueType  pointer to the returned value type of the data
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadRegReadChunk(IN WDFDEVICE  hDevice,   // Driver object
                         IN PUNICODE_STRING pRegPath,    // base path to keys
                         IN PUNICODE_STRING pValueName,
                         IN ULONG  Len,        
                         OUT PUCHAR pReturnData,
                         OUT OPTIONAL PULONG  pLenQueried,
                         OUT OPTIONAL PULONG  pValueType)     
{
static WDFKEY NoParentKey = (WDFKEY)NULL;
NTSTATUS NtStatus = STATUS_SUCCESS;   // assume success
WDFKEY hKey;

    UNREFERENCED_PARAMETER(hDevice);

    NtStatus = 
    WdfRegistryOpenKey(NoParentKey, pRegPath, KEY_QUERY_VALUE, WDF_NO_OBJECT_ATTRIBUTES, &hKey);  
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadRegReadChunk:WdfRegistryOpenKey returns...0x%X\n", NtStatus);
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;

    NtStatus = 
    WdfRegistryQueryValue(hKey, pValueName, Len, pReturnData, pLenQueried, pValueType);
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadRegReadChunk:WdfRegistryQueryValue returned...0x%X\n", NtStatus);

    return (NtStatus);
}

/************************************************************************//**
 * MadRegWriteChunk 
 *
 * DESCRIPTION:
 *    This function writes a chunk of data to the registry 
 *    
 * PARAMETERS: 
 *     @param[in]  pDriverObj   pointer to the driver object 
 *     @param[in]  pRegPath    pointer to the registry path
 *     @param[in]  pValueName  pointer to the value name
 *     @param[in]  Len         length of the chunk to read
 *     @param[in]  pBuffer     pointer to the data to write 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadRegWriteChunk(IN PDRIVER_OBJECT pDriverObj,   
                          IN PUNICODE_STRING pRegPath,    
                          IN PWCHAR pValueName,
                          IN ULONG  Len,        
                          OUT PUCHAR pBuffer)    
{
RTL_QUERY_REGISTRY_TABLE rqrTable[2]; 
NTSTATUS NtStatus = STATUS_SUCCESS;   // assume success

// We set these values to their defaults in case there are any failures
HANDLE  hKey;
OBJECT_ATTRIBUTES  ObjAttrs;

    UNREFERENCED_PARAMETER(pDriverObj);

    RtlZeroMemory(&rqrTable, sizeof(rqrTable)); // mandatory
    rqrTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    rqrTable[0].Name          = pValueName;
    rqrTable[0].EntryContext  = pBuffer;
    rqrTable[0].DefaultType   = REG_NONE;
    rqrTable[0].DefaultData   = NULL;
    rqrTable[0].DefaultLength = Len;  
  
    InitializeObjectAttributes(&ObjAttrs, pRegPath,
                               OBJ_CASE_INSENSITIVE, NULL, (PSECURITY_DESCRIPTOR)NULL);
    NtStatus = ZwOpenKey(&hKey, KEY_QUERY_VALUE, &ObjAttrs);
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadWriteRegChunk:ZwOpenKey NtStatus = 0x%x\n", NtStatus);

    NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                      pRegPath->Buffer, rqrTable, NULL, NULL);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadWriteRegChunk:RtlQueryRegistryValues failed...0x%X\n", NtStatus);
        }

     return (NtStatus);
}


#if 0 //Unused
/************************************************************************//**
 * MadInitDevicePath
 *
 * DESCRIPTION:
 *    This function initializes a unicode string containing a registry path
 *    
 * PARAMETERS: 
 *     @param[in]  pDriverObj   pointer to the driver object
 *     @param[in]  pInPath     pointer to the unicode string containing 
 *                             the base (service key) path  
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  pFdoData    pointer to the framework device extension
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
NTSTATUS MadInitDevicePath(IN PDRIVER_OBJECT pDriverObj,
                           IN PUNICODE_STRING  pInPath,    // zero terminated UNICODE_STRING with parameters path
                           UNICODE_STRING* pParmRegPath,
                           UNICODE_STRING* pRegPathName)
{
NTSTATUS  ret = STATUS_SUCCESS;

UNREFERENCED_PARAMETER(pDriverObj);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadInitDevicePath...enter\n");

    if (pInPath == NULL) 
        return STATUS_INVALID_PARAMETER; // parmx ?
    if (pParmRegPath == NULL) 
        return STATUS_INVALID_PARAMETER; // parmx ?
    if (pRegPathName == NULL) 
        return STATUS_INVALID_PARAMETER; // parmx ?

// We want  a UNICODE String long enough for the Registry path length with the Parameters key
//
    pParmRegPath->MaximumLength = (USHORT)(pInPath->Length + 40 + sizeof(WCHAR)) ;
    pRegPathName->MaximumLength = (USHORT)(pInPath->Length + sizeof(WCHAR)) ;

    pParmRegPath->Buffer = 
    (PWSTR)ExAllocatePoolWithTag(NonPagedPoolNx, 
                                 pParmRegPath->MaximumLength + 4, REG_POOL_TAG);
    pRegPathName->Buffer = 
    (PWSTR)ExAllocatePoolWithTag(NonPagedPoolNx, pRegPathName->MaximumLength + 4, REG_POOL_TAG);
    if ((pParmRegPath->Buffer == NULL) || (pRegPathName->Buffer == NULL)) 
        return STATUS_INSUFFICIENT_RESOURCES;

// ?? TO DO ?? add checking on return pointer here
//
// Since the registry path parameter is a "counted" UNICODE string, it
// might not be zero terminated.  For a very short time allocate memory
// to hold the Registry path zero-terminated so that we can use it to
// delve into the Registry.
//
    RtlCopyUnicodeString(pParmRegPath, pInPath);
    RtlCopyUnicodeString(pRegPathName, pInPath);

    RtlAppendUnicodeToString(pParmRegPath, PARMS_SUBKEY);

//  Null terminate for some uses (but don't increase the 'length' or
//  the UNICODE_STRING version will have a 0 on the end.
//
    pParmRegPath->Buffer[pParmRegPath->Length / sizeof(UNICODE_NULL)] = UNICODE_NULL;
    pRegPathName->Buffer[pRegPathName->Length / sizeof(UNICODE_NULL)] = UNICODE_NULL;

    return ret;
}

/************************************************************************//**
 * Mad.xxxxxxxxxxxxxxxxxxxxxxx
 *
 * DESCRIPTION:
 *    This function initiates a read to the Bob device.
 *    
 * PARAMETERS: 
 *     @param[in]  hQueue      handle to our I/O queue for this device.
 *     @param[in]  hRequest    handle to this I/O request
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  pFdoData    pointer to the framework device extension
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
NTSTATUS MadReadRegistry(IN PDRIVER_OBJECT  pDriverObj, // Driver object
                         IN PUNICODE_STRING pRegPath,          // base path to keys
                          OUT PULONG debugMask,             // 32-bit binary debug mask
                          OUT PULONG eventLog,              // Boolean: do we log events?
                          OUT PULONG shouldBreak,           // Boolean: break in DriverEntry?
                          OUT PULONG Timeout) 
                               
{
RTL_QUERY_REGISTRY_TABLE RegParmTable[7];  // Parameter table -- parameters key
ULONG zero = 0;                          // default value 0
NTSTATUS NtStatus = STATUS_SUCCESS;   // assume success
ULONG xxxx = 0x41;

// We set these values to their defaults in case there are any failures
HANDLE  hKey;
OBJECT_ATTRIBUTES  ObjAttrs;

    UNREFERENCED_PARAMETER(pDriverObj);

    ASSERT(pRegPath != NULL);
 
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadReadRegistry...RegPath = %S\n", pRegPath->Buffer));

// We set these values to their defaults in case there are any failures
    *debugMask = 0;
    *shouldBreak = 0;
    *eventLog = 0;

    RtlZeroMemory(&RegParmTable[0], sizeof(RegParmTable)); // mandatory

    RegParmTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[0].Name = REG_BREAK;
    RegParmTable[0].EntryContext = shouldBreak;
    RegParmTable[0].DefaultType = REG_DWORD;
    RegParmTable[0].DefaultData = &zero;
    RegParmTable[0].DefaultLength = sizeof(ULONG);

    RegParmTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[1].Name = REG_DBG;
    RegParmTable[1].EntryContext = debugMask;
    RegParmTable[1].DefaultType = REG_DWORD;
    RegParmTable[1].DefaultData = &zero;
    RegParmTable[1].DefaultLength = sizeof(ULONG);

    RegParmTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[2].Name = REG_EVENT;
    RegParmTable[2].EntryContext = eventLog;
    RegParmTable[2].DefaultType = REG_DWORD;
    RegParmTable[2].DefaultData = &xxxx;
    RegParmTable[2].DefaultLength = sizeof(ULONG);

    RegParmTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[3].Name = REG_TIMEOUT;
    RegParmTable[3].EntryContext = Timeout;
    RegParmTable[3].DefaultType = REG_DWORD;
    RegParmTable[3].DefaultData = Timeout;
    RegParmTable[3].DefaultLength = sizeof(ULONG);


    //NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
    //                                pPath->Buffer, &RegParmTable[0], NULL, NULL);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadReadRegistry:RtlQueryRegistryValues failed...(%x)\n", NtStatus));
        *shouldBreak = 0;
        *debugMask = 0;
        NtStatus = STATUS_UNSUCCESSFUL;
        return (NtStatus);
        }

    InitializeObjectAttributes(&ObjAttrs, pRegPath,
                               OBJ_CASE_INSENSITIVE, NULL, (PSECURITY_DESCRIPTOR)NULL);
    NtStatus = ZwOpenKey(&hKey, KEY_QUERY_VALUE, &ObjAttrs);
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadReadRegistry:ZwOpenKey NtStatus = 0x%x\n", NtStatus));

    NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                    pRegPath->Buffer, &RegParmTable[0], NULL, NULL);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadReadRegistry:RtlQueryRegistryValues failed (%x)\n", NtStatus));
        return (NtStatus);
        }

     TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		         "MadReadRegistry - debugMask   = 0x%08x\n", *debugMask));
     TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		         "MadReadRegistry - eventLog    = 0x%08x\n", *eventLog));
     TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		         "MadReadRegistry - shouldBreak = 0x%08x\n", *shouldBreak));
     TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		         "MadReadRegistry - Timeout     = 0x%08x\n", *Timeout));

     return (NtStatus);
}
#endif // if 0
