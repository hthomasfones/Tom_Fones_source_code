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
/*  Exe file ID  : MadDevice.sys                                               */
/*                                                                             */
/*  Module  NAME : MadDevWMI.cpp                                               */
/*                                                                             */
/*  DESCRIPTION  : This module handles WMI support for the MadDevice driver.   */
/*                 Derived from WDK-Toaster\func\WMI.c                         */
/*                                                                             */
/*******************************************************************************/

#include "MadDevice.h"

#ifdef WPP_TRACING
    #include "trace.h"
    #include "MadDevWmi.tmh"
#endif

#include <wmilib.h>
#include <wmistr.h>
#include "MadDeviceMof.h"

static NTSTATUS GetDeviceFriendlyName(__in  WDFDEVICE hDevice, __out WDFMEMORY* DeviceName);


extern "C" {
static ULONG MadDeviceHelperFunction1(__in ULONG InData);

static ULONG MadDeviceHelperFunction2(__in ULONG InData1, __in ULONG InData2);

static VOID MadDeviceHelperFunction3(__in  ULONG InData1, __in  ULONG InData2, __out PULONG OutData1, __out PULONG OutData2);
} //End extern "C"

#define MadDevWmiInfo_SIZE FIELD_OFFSET(MadDevWmiInfo, VariableData)

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MadDevWmiInfo, MadDeviceWmiGetData)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MadDevWmiControl, MadDevWmiGetControlData)


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MadDeviceWmiRegistration)
#pragma alloc_text(PAGE, MadDevWmiEvtQueryInfoInstanceData)
#pragma alloc_text(PAGE, MadDevWmiEvtSetInfoInstanceData)
#pragma alloc_text(PAGE, MadDevWmiEvtSetInfoDataItem)
#pragma alloc_text(PAGE, MadDevWmiEvtInstanceQueryDevControl)
#pragma alloc_text(PAGE, MadDevWmiEvtInstance_DevCntlExecMethod)
#pragma alloc_text(PAGE, MadDevWmiEvtInstanceSetDevControl)
#pragma alloc_text(PAGE, MadDeviceHelperFunction1)
#pragma alloc_text(PAGE, MadDeviceHelperFunction2)
#pragma alloc_text(PAGE, MadDeviceHelperFunction3)
#endif

// Global debug error level
//
extern ULONG DebugLevel;


/**************************************************************************//**
 * MadDeviceWmiRegistration
 *
 * DESCRIPTION:
 *    This function registers with WMI as a data provider for this instance
*      of the device.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice   handle to our device. This device object will be
 *                           the parent object of the new WMI instance objects.
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
NTSTATUS MadDeviceWmiRegistration(__in WDFDEVICE hDevice)

{
DECLARE_CONST_UNICODE_STRING(mofResourceName, MOFRESOURCENAME);
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);
PMadDevWmiInfo pMadDevWmiInfo; //Data;
PMadDevWmiControl pCntlData;
NTSTATUS NtStatus;
WDFWMIINSTANCE hWmiObject;
WDF_OBJECT_ATTRIBUTES WdfObjAttrs;
WDF_WMI_PROVIDER_CONFIG providerConfig;
WDF_WMI_INSTANCE_CONFIG instanceConfig;

    PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			    "MadDeviceWmiRegistration...SerialNo=%d, hDevice=%p\n",
				pFdoData->SerialNo, hDevice);

    // Register the MOF resource names of any customized WMI data providers that are not defined in wmicore.mof.
    //
    NtStatus = WdfDeviceAssignMofResourceName(hDevice, &mofResourceName);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDeviceWmiRegistration:WdfDeviceAssignMofResourceName failed! ... Status=0x%X, SerialNo=%d\n",
					NtStatus, pFdoData->SerialNo);
        return NtStatus;
        }

    // Initialize the config structures for the Provider and the Instance and
    // define event callback functions that support a WMI client's request to
    // access the driver's WMI data blocks.
    //
    WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &MadDevWmiInfo_GUID);

    // Specify minimum expected buffer size for query and set instance requests.
    // Since the query block size is different than the set block size, set it
    // to zero and manually check for the buffer size for each operation.
    //
    providerConfig.MinInstanceBufferSize = 0;

    // The WDFWMIPROVIDER handle is needed if multiple instances for the provider
    // has to be created or if the instances have to be created sometime after
    // the provider is created. In case below, the provider handle is not needed
    // because only one instance is needed and can be created when the provider
    // is created.
    //
    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);

    // Create the Provider object as part of the Instance creation call by setting
    // the Register value in the Instance Config to TRUE. This eliminates the
    // need to call WdfWmiProviderRegister.
    //
    instanceConfig.Register = TRUE;
    instanceConfig.EvtWmiInstanceQueryInstance = MadDevWmiEvtQueryInfoInstanceData;
    instanceConfig.EvtWmiInstanceSetInstance   = MadDevWmiEvtSetInfoInstanceData;
    instanceConfig.EvtWmiInstanceSetItem       = MadDevWmiEvtSetInfoDataItem;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&WdfObjAttrs, MadDevWmiInfo);

    // Create the WMI instance object for this data block.
    //
    NtStatus = WdfWmiInstanceCreate(hDevice, &instanceConfig, &WdfObjAttrs, &hWmiObject);
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDeviceWmiRegistration:WdfWmiInstanceCreate for Info data block returned...Status=0x%X, SerialNo=%d\n",
		        NtStatus, pFdoData->SerialNo);
    if (!NT_SUCCESS(NtStatus)) 
         return NtStatus;
  
	pMadDevWmiInfo = MadDeviceWmiGetData(hWmiObject);
	pMadDevWmiInfo->ConnectorType = MADBUS_MODEL_ONE;
    pMadDevWmiInfo->ErrorCount   = 0;
	pMadDevWmiInfo->ServiceTime  = 0; //Only meaningful when the MAD_DEVICE_CONTROL_POWER_MNGT timer is active
	pMadDevWmiInfo->IoCountKb    = 0;
    pMadDevWmiInfo->PowerUsed_mW = 0; //Only meaningful when the MAD_DEVICE_CONTROL_POWER_MNGT timer is active
	//
	RtlCopyMemory(&pFdoData->MadDevWmiData.InfoClassData, pMadDevWmiInfo, sizeof(MadDevWmiInfo));

	WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &MadDeviceNotifyDeviceArrival_GUID /*MADDEVICE_NOTIFY_DEVICE_ARRIVAL_EVENT*/);
    providerConfig.Flags = WdfWmiProviderEventOnly;

    // Specify minimum expected buffer size for query and set instance requests.
    // Since the query block size is different than the set block size, set it
    // to zero and manually check for the buffer size for each operation.
    //
    providerConfig.MinInstanceBufferSize = 0;

    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);
    instanceConfig.Register = TRUE;

    // Create the WMI instance object for this data block.
    //
    NtStatus = WdfWmiInstanceCreate(hDevice, &instanceConfig, WDF_NO_OBJECT_ATTRIBUTES, &pFdoData->hMadDevWmiArrivalEvent);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadDeviceWmiRegistration:WdfWmiInstanceCreate for Arrival Event failed!... Status=0x%X, SerialNo=%d\n",
					NtStatus, pFdoData->SerialNo);
        return NtStatus;
        }

    // Register the MadDevice Control class.
    // Initialize the config structures for the Provider and the Instance and
    // define event callback functions that support a WMI client's request to
    // access the driver's WMI data blocks.
    //
    WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &MadDevWmiControl_GUID);

    // Specify minimum expected buffer size for query and set instance requests.
    //
    providerConfig.MinInstanceBufferSize = MadDevWmiControl_SIZE;

    // The WDFWMIPROVIDER handle is needed if multiple instances for the provider
    // has to be created or if the instances have to be created sometime after
    // the provider is created. In case below, the provider handle is not needed
    // because only one instance is needed and can be created when the provider
    // is created.
    //
    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);

    // Create the Provider object as part of the Instance creation call by setting
    // the Register value in the Instance Config to TRUE. This eliminates the
    // need to call WdfWmiProviderRegister.
    //
    instanceConfig.Register = TRUE;

    instanceConfig.EvtWmiInstanceQueryInstance = MadDevWmiEvtInstanceQueryDevControl;
    //instanceConfig.EvtWmiInstanceSetInstance   = MadDevWmiEvtInstanceSetDevControl;
    //instanceConfig.EvtWmiInstanceSetItem       = MadDevWmiEvtInstanceSetDevControlItem;
    instanceConfig.EvtWmiInstanceExecuteMethod = MadDevWmiEvtInstance_DevCntlExecMethod;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&WdfObjAttrs, MadDevWmiControl);

    // Create the WMI instance object for this data block.
    //
    NtStatus = WdfWmiInstanceCreate(hDevice, &instanceConfig, &WdfObjAttrs, &hWmiObject);
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDeviceWmiRegistration:WdfWmiInstanceCreate for Control data Block returned...Status=0x%X, SerialNo=%d\n",
		        NtStatus, pFdoData->SerialNo);

    if (!NT_SUCCESS(NtStatus))
    	return NtStatus;

    pCntlData = MadDevWmiGetControlData(hWmiObject);
    pCntlData->ControlValue = 0;

    return NtStatus;
}


// WMI System Call back functions
//
/**************************************************************************//**
 * MadDevWmiEvtQueryInfoInstanceData
 *
 * DESCRIPTION:
 *    This function copies a WMI provider's instance data into a buffer for
 *    delivery to a WMI client.
 *    
 * PARAMETERS: 
 *     @param[in]  hWmiObject handle to our I/O Wdf WMI instance
 *     @param[in]  OutBufrSize  size of the passed output buffer
 *     @param[in]  pOutBufr     pointer to the return output buffer 
 *     @param[in]  pBufrUsed    pointer to where the required length is returned
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
#pragma warning(suppress: 28253)
#pragma warning(suppress: 28252)
NTSTATUS MadDevWmiEvtQueryInfoInstanceData(__in  WDFWMIINSTANCE hWmiObject, 
                                           __in  ULONG OutBufrSize, __in  PVOID pOutBufr, 
										   __out PULONG pBufrUsed)
{
PMadDevWmiInfo  pMadDevWmiInfo = MadDeviceWmiGetData(hWmiObject);
WDFDEVICE       hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DATA       pFdoData = MadDeviceFdoGetData(hDevice);
PUCHAR          pBuf;
ULONG           size;
//UNICODE_STRING  string;
NTSTATUS        NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevWmiEvtQueryInfoInstanceData...SerialNo=%d, OutBufrSize=%d\n",
				pFdoData->SerialNo, OutBufrSize);

	//string.Buffer = L"xxxxxxxx\0\0";
    //string.Length = (USHORT) (wcslen(string.Buffer) + 1) * sizeof(WCHAR);
    //string.MaximumLength = string.Length + sizeof(UNICODE_NULL);

    // A USHORT value is needed to contain the length of the buffer in the data
    //
	size = sizeof(MadDevWmiInfo);  //MadDevWmiInfo_SIZE + string.Length + sizeof(USHORT);

    *pBufrUsed = size;

    if (OutBufrSize < size)
        return STATUS_BUFFER_TOO_SMALL;

    pBuf = (PUCHAR)pOutBufr;

    // Copy the structure information
    //
	RtlCopyMemory(pMadDevWmiInfo, &pFdoData->MadDevWmiData.InfoClassData, sizeof(MadDevWmiInfo));
	RtlCopyMemory(pBuf, pMadDevWmiInfo, /*MadDevWmiInfo_SIZE*/sizeof(MadDevWmiInfo));

    //pBuf += MadDevWmiInfo_SIZE;

    // Copy the string. Put length of string ahead of string.
    //
    //NtStatus = WDF_WMI_BUFFER_APPEND_STRING(pBuf, (size - MadDevWmiInfo_SIZE), &string, &size);

    return NtStatus;
}


/************************************************************************//**
 * MadDevWmiEvtSetInfoInstanceData
 *
 * DESCRIPTION:
 *    This function sets ALL of a our WMI data provider's instance data to
 *    values that a WMI client supplies.
 *    
 * PARAMETERS: 
 *     @param[in]  hWmiObject handle to our I/O queue for this device.
 *     @param[in]  InBufrSize   the size of the input buffer 
 *     @param[in]  pInBufr      pointer to the input buffer  
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
#pragma warning(suppress: 28252)
NTSTATUS MadDevWmiEvtSetInfoInstanceData(__in WDFWMIINSTANCE hWmiObject, 
										 __in ULONG InBufrSize, __in PVOID pInBufr)
{
PMadDevWmiInfo pMadDevWmiInfo = MadDeviceWmiGetData(hWmiObject);
WDFDEVICE      hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DATA      pFdoData = MadDeviceFdoGetData(hDevice);

    PAGED_CODE();
	UNREFERENCED_PARAMETER(pInBufr);

	//Report passed values ...
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevWmiEvtSetInfoInstanceData... SerialNo=%d, InBufrSize=%d, ErrCount=%d, ServiceTime=%d, KbytesXfer'd=%d, PowerUsed=%d\n",
				pFdoData->SerialNo, InBufrSize,
				pMadDevWmiInfo->ErrorCount, pMadDevWmiInfo->ServiceTime, pMadDevWmiInfo->IoCountKb, (ULONG)pMadDevWmiInfo->PowerUsed_mW);

    if (InBufrSize < MadDevWmiInfo_SIZE) 
        return STATUS_BUFFER_TOO_SMALL;

	//... And then set all values to zero
	pMadDevWmiInfo->ErrorCount   = 0;
	pMadDevWmiInfo->ServiceTime  = 0;
	pMadDevWmiInfo->IoCountKb    = 0;
	pMadDevWmiInfo->PowerUsed_mW = 0;

	RtlCopyMemory(&pFdoData->MadDevWmiData.InfoClassData, pMadDevWmiInfo, sizeof(MadDevWmiInfo));

    return STATUS_SUCCESS;
}


/************************************************************************//**
 * MadDevWmiEvtSetInfoDataItem 
 *
 * DESCRIPTION:
 *    This function sets a single item of a WMI data provider's instance data
 *     to the value that the WMI client supplies.
 *    
 * PARAMETERS: 
 *     @param[in]  hWmiObject handle to our I/O queue for this device.
 *     @param[in]  InBufrSize   the size of the input buffer 
 *     @param[in]  pInBufr      pointer to the input buffer  
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
#pragma warning(suppress: 28252)
NTSTATUS MadDevWmiEvtSetInfoDataItem(__in WDFWMIINSTANCE hWmiObject, __in ULONG DataItemId,
									 __in ULONG InBufrSize, __in PVOID pInBufr)
{
//PMadDevWmiInfo pMadDevWmiInfo = MadDeviceWmiGetData(hWmiObject);
WDFDEVICE      hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DATA      pFdoData = MadDeviceFdoGetData(hDevice);
//
NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevWmiEvtSetInfoDataItem... SerialNo=%d, InBufrSize=%d, DataItemId=%d\n",
				pFdoData->SerialNo, InBufrSize, DataItemId);

	switch (DataItemId)
	    {
	    case MadDevWmiInfo_ErrorCount_ID:
	    	pFdoData->MadDevWmiData.InfoClassData.ErrorCount = *(PULONG)pInBufr;
	    	break;

	    case MadDevWmiInfo_ServiceTime_ID:
	    	pFdoData->MadDevWmiData.InfoClassData.ServiceTime = *(PULONG)pInBufr;
		    break;

	    case MadDevWmiInfo_IoCountKb_ID:
	    	pFdoData->MadDevWmiData.InfoClassData.IoCountKb = *(PULONG)pInBufr;
		    break;

		case MadDevWmiInfo_PowerUsed_mW_ID:
			pFdoData->MadDevWmiData.InfoClassData.PowerUsed_mW = *(PULONG)pInBufr;
			break;

	    default: // All other fields are read only
		    NtStatus = STATUS_WMI_READ_ONLY;
	    };

	return NtStatus;
}


/**************************************************************************//**
 *  MadDevWmiFireArrivalEvent 
 *
 * DESCRIPTION:
 *    This function creates an event node for WMI & then calls IoWMIWriteEvent
 *    to deliver the arrival event to all of the user-mode WMI components
 *    (data consumers registered for this event) .
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
NTSTATUS  MadDevWmiFireArrivalEvent(__in WDFDEVICE hDevice)
{
PFDO_DATA      pFdoData = MadDeviceFdoGetData(hDevice);
WDFMEMORY hMemory;
PWNODE_SINGLE_INSTANCE  pWmiNode;
ULONG                   wnodeSize;
ULONG                   wnodeDataBlockSize;
ULONG                   wnodeInstanceNameSize;
ULONG                   size;
ULONG                   length;
UNICODE_STRING          deviceName;
UNICODE_STRING          modelName;
NTSTATUS                NtStatus;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
             	"MadDevWmiFireArrivalEvent ... SerialNo=%d\n", pFdoData->SerialNo);

    // *NOTE*
    // WdfWmiFireEvent only fires single instance events at the moment so continue to use this method of firing events
    //
    RtlInitUnicodeString(&modelName, L"MadDeviceWMI\0\0");
    
    // Get the device name.
    //
    NtStatus = GetDeviceFriendlyName(hDevice, &hMemory);
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;

    RtlInitUnicodeString(&deviceName, (PWSTR)WdfMemoryGetBuffer(hMemory, NULL));

    // Determine the amount of wnode information we need.
    //
    wnodeSize = sizeof(WNODE_SINGLE_INSTANCE);
    wnodeInstanceNameSize = deviceName.Length + sizeof(USHORT);
    wnodeDataBlockSize = modelName.Length + sizeof(USHORT);

    size = wnodeSize + wnodeInstanceNameSize + wnodeDataBlockSize;

    // Allocate memory for the WNODE from NonPagedPoolNx
    //
    pWmiNode = 
	(PWNODE_SINGLE_INSTANCE)ExAllocatePoolWithTag(NonPagedPoolNx, size, MAD_DEV_POOL_TAG);
    if (NULL == pWmiNode) 
		{
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
	else
	    {
        RtlZeroMemory(pWmiNode, size);
		PDEVICE_OBJECT pDevObj = WdfDeviceWdmGetDeviceObject(hDevice);
        pWmiNode->WnodeHeader.BufferSize = size;
        pWmiNode->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(pDevObj);
        pWmiNode->WnodeHeader.Version    = 1;
        //KeQuerySystemTime(&pWmiNode->WnodeHeader.TimeStamp);

        RtlCopyMemory(&pWmiNode->WnodeHeader.Guid,
			          &MadDeviceNotifyDeviceArrival_GUID /*&MADDEVICE_NOTIFY_DEVICE_ARRIVAL_EVENT*/, sizeof(GUID));

        // *NOTE*
        // WdfWmiFireEvent only fires single instance events at the moment so continue to use this method of firing events
        // *NOTE*
        //
        // Set flags to indicate that you are creating dynamic instance names.
        // The reason we chose to do dynamic instance is because we can fire the events anytime.
        // If we do static instance names, we can only fire events after WMI queries for IRP_MN_REGINFO,
        // which happens after the device has been started.
        // Another point to note is that if you are firing an event after the device is started, you should
        // check whether the event is enabled, because that indicates that someone is interested in receiving the event.
        // Why fire an event when nobody is interested and waste system resources?
        //
        pWmiNode->WnodeHeader.Flags = WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_SINGLE_INSTANCE;

        pWmiNode->OffsetInstanceName = wnodeSize;
        pWmiNode->DataBlockOffset = wnodeSize + wnodeInstanceNameSize;
        pWmiNode->SizeDataBlock = wnodeDataBlockSize;

        // Write the instance name
        //
        size -= wnodeSize;
        NtStatus = 
		WDF_WMI_BUFFER_APPEND_STRING(WDF_PTR_ADD_OFFSET(pWmiNode, pWmiNode->OffsetInstanceName),
                                     size, &deviceName, &length);
        ASSERT(NT_SUCCESS(NtStatus));  // Size was precomputed, this should never fail

        // Write the data, which is the model name as a string
        //
        size -= wnodeInstanceNameSize;
        NtStatus = 
		WDF_WMI_BUFFER_APPEND_STRING(WDF_PTR_ADD_OFFSET(pWmiNode,  pWmiNode->DataBlockOffset),
                                     size, &modelName, &length);
        ASSERT(NT_SUCCESS(NtStatus)); // Size was precomputed, this should never fail

        // Indicate the event to WMI. WMI will take care of freeing the WMI struct back to pool.
        //
        NtStatus = IoWMIWriteEvent(pWmiNode);
        if (!NT_SUCCESS(NtStatus))
		    {
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        " MadDevWmiFireArrivalEvent:IoWMIWriteEvent failed!...status=0x%X, SerialNo=%d\n",
						NtStatus, pFdoData->SerialNo);
		    ExFreePool(pWmiNode);
			}
        }
 
    // Free the memory allocated by GetDeviceFriendlyName function.
    //
    WdfObjectDelete(hMemory);

    return NtStatus;
}


/**************************************************************************//**
 * GetDeviceFriendlyName
 *
 * DESCRIPTION:
 *    This function returns the friendly name associated with the device object
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     @param[out] pDeviceName pointer to the returned device name 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
NTSTATUS GetDeviceFriendlyName(__in WDFDEVICE hDevice, __out WDFMEMORY* pDeviceName)

{
NTSTATUS NtStatus;

    // First get the length of the string.
    // If the FriendlyName is not there then get the length of device description.
    //
    NtStatus = WdfDeviceAllocAndQueryProperty(hDevice,
                                              DevicePropertyFriendlyName,
                                              NonPagedPoolNx,
                                              WDF_NO_OBJECT_ATTRIBUTES,
                                              pDeviceName);

    if (!NT_SUCCESS(NtStatus) && NtStatus != STATUS_INSUFFICIENT_RESOURCES) 
        NtStatus = WdfDeviceAllocAndQueryProperty(hDevice,
                                                  DevicePropertyDeviceDescription,
                                                  NonPagedPoolNx,
                                                  WDF_NO_OBJECT_ATTRIBUTES,
                                                  pDeviceName);

    if (!NT_SUCCESS(NtStatus)) 
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "WdfDeviceQueryProperty returned %x\n", NtStatus);
	    }

    return NtStatus;
}


/************************************************************************//**
 * MadDevWmiEvtInstanceQueryDevControl 
 *
 * DESCRIPTION:
 *    This function is the callback routine for the WMI Query irp on the 
 *    instance representing the MadDevWmiControl Class. This method copies
 *    the MadDevWmiControl Class data into the given buffer for delivery to
 *    a WMI client.
 *    
 * PARAMETERS: 
 *     @param[in]  hWmiObject  the handle to the WMI instance object 
 *     @param[in]  OutBufrSize   the size (in bytes) of the output buffer into
 *                               which the instance data is to be copied.
 *     @param[in]  pOutBufr      pointer to the output buffer. 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
#pragma warning(suppress: 28253)
#pragma warning(suppress: 28252)
NTSTATUS MadDevWmiEvtInstanceQueryDevControl(__in  WDFWMIINSTANCE hWmiObject, __in  ULONG OutBufrSize,
                                             __in  PVOID pOutBufr, __out PULONG pBufrUsed)

{
PMadDevWmiControl pCntlData = MadDevWmiGetControlData(hWmiObject);
WDFDEVICE         hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DATA         pFdoData = MadDeviceFdoGetData(hDevice);

    PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevWmiEvtInstanceQueryDevControl...SerialNo=%d, OutBufrSize=%d\n",
				pFdoData->SerialNo, OutBufrSize);

    // Since the minimum buffer size required for querying the instance data was already 
	// specified during the WMI instance setup, the Framework will make sure that the 
	// incoming buffer size is large enough for this instance query. There is no need to check
	// the size of the given buffer again.
    // The instance information can be copied directly to the given buffer.
    //
	RtlCopyMemory(pOutBufr, pCntlData, MadDevWmiControl_SIZE);
    *pBufrUsed = MadDevWmiControl_SIZE;

	pCntlData->ControlValue++;

    return STATUS_SUCCESS;
}


/************************************************************************//**
 * MadDevWmiEvtInstanceSetDevControl
 *
 * DESCRIPTION:
 *    This function is the callback routine for the WMI Set instance information
 *    irp on the instance representing the MadDevWmiControl Class. This method 
 *    reads the data from the input buffer and updates the writable elements
 *    of the Instance.
 *    
 * PARAMETERS: 
 *     @param[in]  hWmiObject  the handle to the WMI instance object 
 *     @param[in]  InBufrSize    the size (in bytes) of the output buffer into
 *                               which the instance data is to be copied.
 *     @param[in]  pInBufr       pointer to the output buffer. 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
#pragma warning(suppress: 28252)
NTSTATUS MadDevWmiEvtInstanceSetDevControl(__in WDFWMIINSTANCE hWmiObject, 
										   __in ULONG InBufrSize, __in PVOID pInBufr)

{
//PMadDevWmiControl pCntlData = MadDevWmiGetControlData(hWmiObject);
WDFDEVICE         hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DATA         pFdoData   = MadDeviceFdoGetData(hDevice);

	UNREFERENCED_PARAMETER(pInBufr);
    PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevWmiEvtInstanceSetDevControl ... SerialNo=%d, InBufrSize=%d\n",
				pFdoData->SerialNo, InBufrSize);

    // Since the minimum buffer size required for setting the instance data was already
	// specified during the WMI instance setup, the Framework will make sure that the
	// incoming buffer size is large enough for this set request.
    // There is no need to check the size of the given buffer again. 
    // The instance information can be copied directly from the given buffer.
    // Also, note that only the wriable elements need to be copied.
    //
    //pMadDevWmiInfo->ControlValue = ((PMadDevWmiControl)pInBufr)->ControlValue;

	return STATUS_NOT_IMPLEMENTED; //STATUS_SUCCESS;
}


/************************************************************************//**
 * MadDevWmiEvtInstanceSetDevControlItem
 *
 * DESCRIPTION:
 *    This function initiates a read to the Bob device.
 *    
 * PARAMETERS: 
 *     @param[in]  hWmiObject  the handle to the WMI instance object 
 *     @param[in]  InBufrSize    the size (in bytes) of the input buffer into
 *                               which the instance data is to be copied.
 *     @param[in]  pInBufr       pointer to the input buffer. 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
#pragma warning(suppress: 28252)
NTSTATUS MadDevWmiEvtInstanceSetDevControlItem(__in WDFWMIINSTANCE hWmiObject, __in ULONG DataItemId, 
                                            __in ULONG InBufrSize, __in PVOID pInBufr)
{
//PMadDevWmiControl pCntlData = MadDevWmiGetControlData(hWmiObject);
WDFDEVICE         hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DATA         pFdoData = MadDeviceFdoGetData(hDevice);

	UNREFERENCED_PARAMETER(pInBufr);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevWmiEvtInstanceSetDevControlItem ... SerialNo=%d, InBufrSize=%d, DataItemId=%d\n",
				pFdoData->SerialNo, InBufrSize, DataItemId);

	return STATUS_NOT_IMPLEMENTED; //STATUS_SUCCESS;

    /*if (DataItemId == MadDevWmiControl_ControlValue_ID)
        {
        if (InBufrSize < sizeof(ULONG)) 
            return STATUS_BUFFER_TOO_SMALL;

        //pMadDevWmiInfo->ControlValue = *((PULONG)pInBufr);
        return STATUS_SUCCESS;
        } 
    else 
        {
        return STATUS_WMI_READ_ONLY;
        }*/
}


/**************************************************************************//**
 * MadDevWmiEvtInstance_DevCntlExecMethod
 *
 * DESCRIPTION:
 *    This function handles the Execute Method WMI request to the MadDevice.
 *    
 * PARAMETERS: 
 *     @param[in]  hWmiObject   the handle to the WMI instance object.
 *     @param[in]  MethodId       the method id of the method in MadDevWmiControl
 *                                class to execute
 *     @param[in]  InBufrSize     the size (in bytes) of the input buffer from
 *                                which the input parameter values are read. 
 *     @param[in]  OutBufrSize    the size (in bytes) of the output buffer into
 *                                which the instance data is to be copied
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
#pragma warning(suppress: 28252)
#pragma warning(suppress: 28253)
NTSTATUS MadDevWmiEvtInstance_DevCntlExecMethod( __in    WDFWMIINSTANCE hWmiObject,
                                                 __in    ULONG MethodId,
                                                 __in    ULONG InBufrSize,
                                                 __in    ULONG OutBufrSize,
                                                 __in    PVOID pBuffer,
                                                 __out   PULONG pBufrUsed)

{
//PMadDevWmiControl pCntlData = MadDevWmiGetControlData(hWmiObject);
WDFDEVICE         hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DATA         pFdoData = MadDeviceFdoGetData(hDevice);

NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevWmiEvtInstance_DevCntlExecMethod - just verifying the plumbing :) ...SerialNo=%d, MethodId=%d\n",
				pFdoData->SerialNo, MethodId);

    switch (MethodId)
         {
        case MadDevWmiControl1:
            {
            PMadDevWmiControl1_IN  pInBufr  = (PMadDevWmiControl1_IN)pBuffer;
            PMadDevWmiControl1_OUT pOutBufr = (PMadDevWmiControl1_OUT)pBuffer;

            // Check the size of the input and output buffers.
            //
            if (InBufrSize < MadDevWmiControl1_IN_SIZE)
                {
                NtStatus = STATUS_INVALID_PARAMETER_MIX;
                break;
                }

            if (OutBufrSize < MadDevWmiControl1_OUT_SIZE)
                {
                NtStatus = STATUS_BUFFER_TOO_SMALL;
                break;
                }

            // Call the helper function for the MadDevice Method 1.
            //
            pOutBufr->OutData = MadDeviceHelperFunction1(pInBufr->InData);

            // Update the size of the output buffer used.
            //
            *pBufrUsed = MadDevWmiControl1_OUT_SIZE;
            }
            break;

        case MadDevWmiControl2:
            {
            PMadDevWmiControl2_IN  pInBufr  = (PMadDevWmiControl2_IN) pBuffer;
            PMadDevWmiControl2_OUT pOutBufr = (PMadDevWmiControl2_OUT)pBuffer;

            // Check the size of the input and output buffers.
            //
            if (InBufrSize < MadDevWmiControl2_IN_SIZE) 
                {
                NtStatus = STATUS_INVALID_PARAMETER_MIX;
                break;
                }

            if (OutBufrSize < MadDevWmiControl2_OUT_SIZE)
                {
                NtStatus = STATUS_BUFFER_TOO_SMALL;
                break;
                }

            // Call the helper function for the MadDevice Method 2.
            //
            pOutBufr->OutData =
			MadDeviceHelperFunction2(pInBufr->InData1, pInBufr->InData2);

             // Update the size of the output buffer used.
            //
            *pBufrUsed = MadDevWmiControl2_OUT_SIZE;
            }
            break;

        case MadDevWmiControl3:
            {
            PMadDevWmiControl3_IN  pInBufr  = (PMadDevWmiControl3_IN)pBuffer;
            PMadDevWmiControl3_OUT pOutBufr = (PMadDevWmiControl3_OUT)pBuffer;

            // Check the size of the input and output buffers.
            //
            if (InBufrSize < MadDevWmiControl3_IN_SIZE)
                {
                NtStatus = STATUS_INVALID_PARAMETER_MIX;
                break;
                }

            if (OutBufrSize < MadDevWmiControl3_OUT_SIZE)
                {
                NtStatus = STATUS_BUFFER_TOO_SMALL;
                break;
                }

            // Call the helper function for the MadDevice Method 3.
            //
            MadDeviceHelperFunction3(pInBufr->InData1,
                                     pInBufr->InData2,
                                     &(pOutBufr->OutData1),
                                     &(pOutBufr->OutData2));

            // Update the size of the output buffer used.
            //
            *pBufrUsed = MadDevWmiControl3_OUT_SIZE;
            }
            break;

        default:
            NtStatus = STATUS_WMI_ITEMID_NOT_FOUND;
            break;
        }

    return NtStatus;
}


/************************************************************************//**
 * MadDeviceHelperFunction1
 *
 * DESCRIPTION:
 *    This function is a helper to the WMI execute method function 
 *    
 * PARAMETERS: 
 *     
 * RETURNS:
 *    @return      ULONG       a general result (arithmentic)
 * 
 ***************************************************************************/
ULONG MadDeviceHelperFunction1(__in ULONG InData)
{
    PAGED_CODE();

    return (InData + 1);
}


/************************************************************************//**
 * MadDeviceHelperFunction2
 *
 * DESCRIPTION:
 *    This function is a helper to the WMI execute method function 
 *    
 * PARAMETERS: 
 *     
 * RETURNS:
 *    @return      ULONG       a general result (arithmentic)
 * 
 ***************************************************************************/
ULONG MadDeviceHelperFunction2(__in ULONG InData1, __in ULONG InData2)
{
    PAGED_CODE();

    return (InData1 + InData2);
}


/************************************************************************//**
 * MadDeviceHelperFunction3
 *
 * DESCRIPTION:
 *    This function is a helper to the WMI execute method function 
 *    
 * PARAMETERS: 
 *     
 * RETURNS:
 *    @return      ULONG       a general result (arithmentic)
 * 
 ***************************************************************************/
VOID MadDeviceHelperFunction3(__in  ULONG InData1, __in  ULONG InData2, __out PULONG OutData1, __out PULONG OutData2)
{
    PAGED_CODE();

    *OutData1 = InData1 + 1;
    *OutData2 = InData2 + 1;

    return;
}

