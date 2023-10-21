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
/*  Exe file ID  : MadBus.sys                                                  */
/*                                                                             */
/*  Module  NAME : MadBusWmi.cpp                                               */
/*                                                                             */
/*  DESCRIPTION  : WMI functions for the simulator-bus driver                  */
/*                 Derived from WDK-Toaster\bus\wmi.c                          */
/*                                                                             */
/*******************************************************************************/

#include "MadBus.h"
#ifdef WPP_TRACING
    #include "trace.h"
	#include "MadBusWmi.tmh"
#endif

#include <wmilib.h>
#include <wmistr.h>
#include "MadBusMof.h"

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MadBusWmiInfo, MadBusWmiGetData)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MadBusWmiControl, MadBusWmiGetControlData)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MadBus_WmiRegistration)
#pragma alloc_text(PAGE,MadBusWmiEvtSetInfoInstanceDataItem)
#pragma alloc_text(PAGE,MadBusWmiEvtSetInfoInstanceData)
#pragma alloc_text(PAGE,MadBusWmiEvtQueryInfoInstanceData)
#pragma alloc_text(PAGE,MadBusWmiEvtInstanceQueryDevControl)
#pragma alloc_text(PAGE,MadBusWmiEvtInstance_DevCntlExecMethod)
#endif


/************************************************************************//**
 * MadBus_WmiRegistration
 *
 *
 * DESCRIPTION:
 *    This function registers with WMI as a data provider for this instance
 *    of the device.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice   handle to our device. This device object will be
 *                           the parent object of the new WMI instance objects.
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
 
NTSTATUS MadBus_WmiRegistration(WDFDEVICE hDevice)

{
//DECLARE_GLOBAL_CONST_UNICODE_STRING(BusResrcName, BUSRESOURCENAME);
UNICODE_STRING(BusResrcName);
PFDO_DEVICE_DATA pFdoData = MadBusFdoGetData(hDevice);
WDF_WMI_PROVIDER_CONFIG providerConfig;
WDF_WMI_INSTANCE_CONFIG instanceConfig;
NTSTATUS NtStatus;
WDFWMIINSTANCE hWmiObject;
WDF_OBJECT_ATTRIBUTES WdfObjAttrs;

    PAGED_CODE(); 
	RtlInitUnicodeString(&BusResrcName, BUSRESOURCENAME);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus_WmiRegistration...SerialNo=%d, hDevice=%p\n",
		         pFdoData->SerialNo, hDevice);

    // Register WMI classes.
    // First specify the resource name which contain the binary mof resource.
    //
    NtStatus = WdfDeviceAssignMofResourceName(hDevice, &BusResrcName);
	if (!NT_SUCCESS(NtStatus))
	    { 
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBus_WmiRegistration:WdfDeviceAssignMofResourceName failed! ... Status=0x%X, SerialNo=%d\n",
			        NtStatus, pFdoData->SerialNo);
		return NtStatus;
	    }

	WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &MadBusWmiInfo_GUID);
	providerConfig.MinInstanceBufferSize = sizeof(_MadBusWmiInfo);

    // You would want to create a WDFWMIPROVIDER handle separately if you are going to dynamically
	// create instances on the provider. Since we are statically creating one instance, 
	// there is no need to create the provider handle.
    //
    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);

    // By setting Regsiter to TRUE, we tell the framework to create a provider as part of the 
	// Instance creation call. This eliminates the need to call WdfWmiProviderRegister.
    //
    instanceConfig.Register = TRUE;
    instanceConfig.EvtWmiInstanceQueryInstance = MadBusWmiEvtQueryInfoInstanceData;
    instanceConfig.EvtWmiInstanceSetInstance = MadBusWmiEvtSetInfoInstanceData;
    instanceConfig.EvtWmiInstanceSetItem = MadBusWmiEvtSetInfoInstanceDataItem;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&WdfObjAttrs, MadBusWmiInfo);

	// Create the WMI instance object for this data block.
	//
	NtStatus = WdfWmiInstanceCreate(hDevice, &instanceConfig, &WdfObjAttrs, &hWmiObject);
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus_WmiRegistration:WdfWmiInstanceCreate for Info data block returned...Status=0x%X, SerialNo=%d\n",
		        NtStatus, pFdoData->SerialNo);
	if (NT_SUCCESS(NtStatus))
	    {
		pFdoData->MadBusWmiData.InfoClassData.ErrorCount = 0;
		pFdoData->MadBusWmiData.InfoClassData.CurrDevCount = 0;
		pFdoData->MadBusWmiData.InfoClassData.TotlDevCount = 0;

		PMadBusWmiInfo pMadBusWmiInfo = MadBusWmiGetData(hWmiObject);
		RtlCopyMemory(pMadBusWmiInfo, &pFdoData->MadBusWmiData.InfoClassData, sizeof(MadBusWmiInfo));
	    }

	//No device arrival wmi instance created here
	//

	// Register the MadBus Control class.
	// Initialize the config structures for the Provider and the Instance and
	// define event callback functions that support a WMI client's request to
	// access the driver's WMI data blocks.
	//
	WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &MadBusWmiControl_GUID);

	// Specify minimum expected buffer size for query and set instance requests.
	//
	providerConfig.MinInstanceBufferSize = MadBusWmiControl_SIZE;

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
	instanceConfig.EvtWmiInstanceQueryInstance = MadBusWmiEvtInstanceQueryDevControl;
	//instanceConfig.EvtWmiInstanceSetInstance = MadBusWmiEvtInstanceSetDevControl;
	//instanceConfig.EvtWmiInstanceSetItem = MadBusWmiEvtInstanceSetDevControlItem;
	instanceConfig.EvtWmiInstanceExecuteMethod = MadBusWmiEvtInstance_DevCntlExecMethod; 
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&WdfObjAttrs, MadBusWmiControl);

	// Create the WMI instance object for this data block.
	//
	NtStatus = WdfWmiInstanceCreate(hDevice, &instanceConfig, &WdfObjAttrs, &hWmiObject);
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus_WmiRegistration:WdfWmiInstanceCreate for Control data block returned...Status=0x%X, SerialNo=%d\n",
		        NtStatus, pFdoData->SerialNo);
	if (NT_SUCCESS(NtStatus))
	    {
		PMadBusWmiControl pCntlData = MadBusWmiGetControlData(hWmiObject);
		pCntlData->ControlValue = 0;
	    }

    return NtStatus;
}


// WMI System Call back functions ***********************************************
//
/************************************************************************//**
* MadBusWmiEvtQueryInfoInstanceData
*
* DESCRIPTION:
*    This function is a callback into the driver to query the contents of
*    a wmi data block instance.
*
* PARAMETERS:
*     @param[in]   hWmiObject   is the instance being set
*     @param[in]   OutBufrSize    has the size available to write the data
*     @param[in]   pOutBufr       pointer to the buffer filled with the returned data block
*     @param[in]   pBufrUsed      pointer to the variable to contain the
*                                 required buffer size
* RETURNS:
*    @return      NtStatus    indicates success or reason for the failure
*
***************************************************************************/
NTSTATUS MadBusWmiEvtQueryInfoInstanceData(IN WDFWMIINSTANCE hWmiObject,
	                                   IN ULONG OutBufrSize, IN PVOID pOutBufr, OUT PULONG pBufrUsed)
{
WDFDEVICE           hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DEVICE_DATA    pFdoData = MadBusFdoGetData(hDevice);

    *pBufrUsed = sizeof(MadBusWmiInfo);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusWmiEvtQueryInfoInstanceData...SerialNo=%d, OutBufrSize=%d, Bufr used=%d\n",
				pFdoData->SerialNo, OutBufrSize, *pBufrUsed);
	PAGED_CODE();

	RtlCopyMemory(pOutBufr, &pFdoData->MadBusWmiData.InfoClassData, sizeof(MadBusWmiInfo));

	return STATUS_SUCCESS;
}


/************************************************************************//**
 * MadBusWmiEvtSetInfoInstanceData
 *
 * DESCRIPTION:
 *   This function is a callback into the driver to set the contents of
*    a wmi data block instance.
 *    
 * PARAMETERS: 
 *     @param[in]  hWmiObject is the instance being set
 *     @param[in]  InBufrSize    has the size of the data item passed 
 *     @param[in]  pInBufr       pointer to the new values for the data item
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBusWmiEvtSetInfoInstanceData(IN  WDFWMIINSTANCE hWmiObject,
                                     IN  ULONG InBufrSize,
                                     IN  PVOID pInBufr)
{
WDFDEVICE           hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DEVICE_DATA    pFdoData = MadBusFdoGetData(hDevice);
PMadBusWmiInfo      pMadBusWmiInfo = (PMadBusWmiInfo)pInBufr;

    PAGED_CODE();

	//Report the values passed in ...
	//
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusWmiEvtSetInfoInstanceData...SerialNo=%d, InBufrSize=%d, ErrCount=%d, CurrDevCount=%d, TotlDevCount=%d\n",
				pFdoData->SerialNo, InBufrSize, 
				pMadBusWmiInfo->ErrorCount, pMadBusWmiInfo->CurrDevCount, pMadBusWmiInfo->TotlDevCount);
 
	// ... and then shamelessly reset all counters to zero
	//
	pMadBusWmiInfo->ErrorCount = 0;
	pMadBusWmiInfo->CurrDevCount = 0;
	pMadBusWmiInfo->TotlDevCount = 0;

	RtlCopyMemory(&pFdoData->MadBusWmiData.InfoClassData, pMadBusWmiInfo, sizeof(MadBusWmiInfo));

    return STATUS_SUCCESS;
}

/************************************************************************//**
* MadBusWmiEvtSetInfoInstanceDataItem
*
* DESCRIPTION:
*    This function initiates a read to the Bob device.
*
* PARAMETERS:
*     @param[in]  hWmiObject  a handle to is the instance being set
*     @param[in]  DataItemId    has the id of the data item being set
*     @param[in]  InBufrSize    has the size of the data item passed
*     @param[in]  pInBufr       pointer to the new values for the data item
*
* RETURNS:
*    @return      NtStatus    indicates success or reason for the failure
*
***************************************************************************/
NTSTATUS MadBusWmiEvtSetInfoInstanceDataItem(IN  WDFWMIINSTANCE hWmiObject,
	                                     IN  ULONG DataItemId, IN  ULONG InBufrSize, IN  PVOID pInBufr)
{
WDFDEVICE           hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DEVICE_DATA    pFdoData = MadBusFdoGetData(hDevice);
//
NTSTATUS NtStatus = STATUS_SUCCESS;

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusWmiEvtSetInfoInstanceDataItem...SerialNo=%d, DataItemId=%d, InBufrSize=%d\n",
		        pFdoData->SerialNo, DataItemId, InBufrSize);

	switch (DataItemId)
	    {
	    case MadBusWmiInfo_ErrorCount_ID:
	    	pFdoData->MadBusWmiData.InfoClassData.ErrorCount = *(PULONG)pInBufr;
	    	break;

	    case MadBusWmiInfo_CurrDevCount_ID:
	    	pFdoData->MadBusWmiData.InfoClassData.CurrDevCount = *(PULONG)pInBufr;
	    	break;

	    case MadBusWmiInfo_TotlDevCount_ID:
	    	pFdoData->MadBusWmiData.InfoClassData.TotlDevCount = *(PULONG)pInBufr;
		    break;

	    default: // All other fields are read only
		    NtStatus = STATUS_WMI_READ_ONLY;
	    };

	return NtStatus;
}


/************************************************************************//**
* MadBusWmiEvtInstanceQueryDevControl
*
* DESCRIPTION:
*    This function is the callback routine for the WMI Query irp on the
*    instance representing the MadBusWmiControl Class. This method copies
*    the MadBusWmiControl Class data into the given buffer for delivery to
*    a WMI client.
*
* PARAMETERS:
*     @param[in]  hWmiObject  the handle to the WMI instance object
*     @param[in]  OutBufrSize   the size (in bytes) of the output buffer into
*                                which the instance data is to be copied.
*     @param[in]  pOutBufr      pointer to the output buffer.
*
* RETURNS:
*    @return      NtStatus    indicates success or reason for the failure
*
***************************************************************************/
#pragma warning(suppress: 28253)
#pragma warning(suppress: 28252)
NTSTATUS MadBusWmiEvtInstanceQueryDevControl(__in  WDFWMIINSTANCE hWmiObject, __in  ULONG OutBufrSize,
	                                         __in  PVOID pOutBufr, __out PULONG pBufrUsed)

{
PMadBusWmiControl pCntlData = MadBusWmiGetControlData(hWmiObject);
WDFDEVICE         hDevice   = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DEVICE_DATA  pFdoData  = MadBusFdoGetData(hDevice);

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusWmiEvtInstanceQueryDevControl...SerialNo=%d, OutBufrSize=%d, ControlValue=%d\n",
				pFdoData->SerialNo, OutBufrSize, pCntlData->ControlValue);

	// Since the minimum buffer size required for querying the instance data was already 
	// specified during the WMI instance setup, the Framework will make sure that the 
	// incoming buffer size is large enough for this instance query. There is no need to check
	// the size of the given buffer again.
	// The instance information can be copied directly to the given buffer.
	//
	RtlCopyMemory(pOutBufr, pCntlData, MadBusWmiControl_SIZE);
	*pBufrUsed = MadBusWmiControl_SIZE;

	return STATUS_SUCCESS;
}


/**************************************************************************//**
* MadBusWmiEvtInstance_DevCntlExecMethod
*
* DESCRIPTION:
*    This function handles the Execute Method WMI request to the MadBus.
*     We just increment the variable in the WMI object and log  
*
* PARAMETERS:
*     @param[in]  hWmiObject   the handle to the WMI instance object.
*     @param[in]  MethodId       the method id of the method in MadBusWmiControl
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
NTSTATUS MadBusWmiEvtInstance_DevCntlExecMethod(__in WDFWMIINSTANCE hWmiObject, __in ULONG MethodId,
	                                            __in ULONG InBufrSize, __in ULONG OutBufrSize, __in PVOID pBuffer, __out PULONG pBufrUsed)

{
WDFDEVICE         hDevice = WdfWmiInstanceGetDevice(hWmiObject);
PFDO_DEVICE_DATA  pFdoData = MadBusFdoGetData(hDevice);
PMadBusWmiControl pCntlData = MadBusWmiGetControlData(hWmiObject);
//
NTSTATUS NtStatus = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(pBufrUsed);

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadBusWmiEvtInstance_DevCntlExecMethod just verifying the plumbing :)...SerialNo=%d, ControlValue=%d, MethodId=%d, InBufrSize=%d, OutBufrSize=%d\n",
	            pFdoData->SerialNo, pCntlData->ControlValue, MethodId, InBufrSize, OutBufrSize);

	pCntlData->ControlValue++; //For next time
	pBufrUsed = &pCntlData->ControlValue;
	OutBufrSize = sizeof(ULONG);

	return NtStatus;
}
