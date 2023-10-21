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
/*  Module  NAME : MadPower.cpp                                                */
/*                                                                             */
/*  DESCRIPTION  : Implements callbacks to manager power transition, wait-wake */ 
/*                 and selective suspend.                                      */
/*                 Derived from WDK-Toaster\func\power.c                       */
/*                                                                             */
/*******************************************************************************/

#include "../inc/MadDevice.h"
#include "../Includes/PowerNotify.h"

#ifdef WPP_TRACING
    #include "trace.h"
    #include "MadPower.tmh"
#endif

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, MadEvtD0Exit)
#pragma alloc_text(PAGE, MadEvtArmWakeFromS0)
#pragma alloc_text(PAGE, MadEvtArmWakeFromSx)
#pragma alloc_text(PAGE, MadEvtWakeFromS0Triggered)
#pragma alloc_text(PAGE, DbgDevicePowerString)
#endif // ALLOC_PRAGMA

extern LONG gPowrChekTimrMillisecs;

/************************************************************************//**
 * MadEvtD0Entry
 *
 * DESCRIPTION:
 *    This function is called to program the device to go into D0, which is 
 *    the working state. The framework calls the driver's EvtDeviceD0Entry
 *    callback when the Power manager sends an IRP_MN_SET_POWER-DevicePower 
 *    request to the driver stack. The Power manager sends this request when
 *    the power policy manager of this device stack (probaby the FDO) requests
 *    a change in D-state by calling PoRequestPowerIrp.
 *
 *    This function is not marked pageable because this function is in the
 *    device power up path. When a function is marked pagable and the code
 *    section is paged out, it will generate a page fault which could impact
 *    the fast resume behavior because the client driver will have to wait
 *    until the system drivers can service this page fault.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice        handle to our device 
 *     @param[in]  PrevPowerState the power state we are returning from
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadEvtD0Entry(IN WDFDEVICE hDevice, IN WDF_POWER_DEVICE_STATE PrevPowerState)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    UNREFERENCED_PARAMETER(PrevPowerState);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtD0Entry...coming from %s, SerialNo=%d\n",
		        DbgDevicePowerString(PrevPowerState), pFdoData->SerialNo);
 
	// Program the device to go into D0
    #ifdef POWER_MANAGEMENT_ENABLED
    MadDev_IssuePowerNotice(pFdoData, PowerDeviceD0, PowerActionNone);
    #endif

    #ifdef _MAD_SIMULATION_MODE_ //======================================================================
	//Activate the Simulator's interrupt thread at power-up
     ULONG Signald = 
  	KeSetEvent(&pFdoData->pMadSimIntParms->evDevPowerUp, (KPRIORITY)0, FALSE);
    //ASSERT(Signald == 0); //Wasn't already signalled
	UNREFERENCED_PARAMETER(Signald);

    //Because we haven't set up a proper Wdf interrupt ...
    //the framework won't invoke our interrupt-enable callback function
    //so we call it directly
	KIRQL BaseIRQL;
    MAD_ACQUIRE_LOCK_RAISE_IRQL(pFdoData->hIntSpinlock, pFdoData->Irql, &BaseIRQL);
	//
    NTSTATUS NtStatus = MadEvtIntEnable(pFdoData->hInterrupt, hDevice);
	//
	MAD_LOWER_IRQL_RELEASE_LOCK(BaseIRQL, pFdoData->hIntSpinlock);
	ASSERT(NtStatus == STATUS_SUCCESS);
	UNREFERENCED_PARAMETER(NtStatus);
    #endif //_MAD_SIMULATION_MODE_ .......................................................................

    return STATUS_SUCCESS;
}

/************************************************************************//**
 * MadEvtD0Exit
 *
 * DESCRIPTION:
 *    This function is called to program the device to go into D1, D2 or D3,
 *    which are the low-power states. The framework calls the  driver's
 *    EvtDeviceD0Exit callback when the Power Mngr sends an IRP_MN_SET_POWER
 *    -DevicePower request to the driver stack. The Power Mngr sends this 
 *    request when the power policy manager of this device stack requests a
 *    change in D-state by calling PoRequestPowerIrp.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  PowerState  the power state we are going to
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadEvtD0Exit(IN WDFDEVICE hDevice, IN WDF_POWER_DEVICE_STATE  PowerState)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtD0Exit...going to %s, SerialNo=%d\n",
		        DbgDevicePowerString(PowerState), pFdoData->SerialNo);

    #ifdef _MAD_SIMULATION_MODE_ //=====================================================================================
    //Because we haven't set up a proper Wdf interrupt ...
    //the framework won't invoke our interrupt-disable callback function
    //so we call it directly
    //
	KIRQL BaseIRQL;
	MAD_ACQUIRE_LOCK_RAISE_IRQL(pFdoData->hIntSpinlock, pFdoData->Irql, &BaseIRQL);
	//
    NTSTATUS NtStatus = MadEvtIntDisable(pFdoData->hInterrupt, hDevice);
	//
 	MAD_LOWER_IRQL_RELEASE_LOCK(BaseIRQL, pFdoData->hIntSpinlock);
    ASSERT(NtStatus == STATUS_SUCCESS);
	UNREFERENCED_PARAMETER(NtStatus);

	ULONG Signald; 
	if (PowerState == WdfPowerDeviceD3Final) //They're calling the mortician. Let's shut down the interrupt thread
	    {
        //We signal the exit event 1st because the interrupt thread won't wait for it 
        Signald = 
        KeSetEvent(pFdoData->pMadSimIntParms->pEvIntThreadExit, (KPRIORITY)0, FALSE);
        ASSERT(Signald == 0); //Wasn't already signalled

		//Now when we signal the power down event it will also see the thread-exit event signalled
		//...
	    }

    //Signal the simulator's interrupt thread power-down event (Deactivate the thread)
    //
    Signald = 
    KeSetEvent(&pFdoData->pMadSimIntParms->evDevPowerDown, (KPRIORITY)0, FALSE);
    ASSERT(Signald == 0); //Wasn't already signalled
    #endif //_MAD_SIMULATION_MODE_ ......................................................................................

    #ifdef POWER_MANAGEMENT_ENABLED //Program the device to go into power state Dx (1, 2 or 3)
	MadDev_IssuePowerNotice(pFdoData, (DEVICE_POWER_STATE)PowerState, PowerActionNone);
    #endif

    return STATUS_SUCCESS;
}


/************************************************************************//**
 * MadEvtArmWakeFromS0
 *
 * DESCRIPTION:
 *    This function is called when the Framework arms the device for wake
 *    from S0.  If there is any device-specific initialization that needs to 
 *    be done to arm internal wake signals, or to route internal interrupt 
 *    signals to the wake logic, it should be done here.  The device will be 
 *    moved out of the D0 state soon after this callback is invoked.
 *
 *    This function is pageable and it will run at PASSIVE_LEVEL.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadEvtArmWakeFromS0(IN WDFDEVICE hDevice)

{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    PAGED_CODE();
	UNREFERENCED_PARAMETER(pFdoData);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtArmWakeFromS0...SerialNo=%d\n", pFdoData->SerialNo);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "<-- MadEvtArmWakeFromS0\n");

    return STATUS_SUCCESS;
}


/************************************************************************//**
 * MadEvtArmWakeFromSx
 *
 * DESCRIPTION:
 *    This function is called when the Framework arms the device for wake 
 *    from Sx.  If there is any device-specific initialization that needs to 
 *    be done to arm internal wake signals, or to route internal interrupt
 *    signals to the wake logic, it should be done here.  The device will be 
 *    moved out of the D0 state soon after this callback is invoked.
 *
 *   This function is pageable and it will run at PASSIVE_LEVEL.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadEvtArmWakeFromSx(IN WDFDEVICE hDevice)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    PAGED_CODE();
	UNREFERENCED_PARAMETER(pFdoData);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtArmWakeFromSx...SerialNo=%d\n", pFdoData->SerialNo);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "<-- MadEvtArmWakeFromSx\n");

    return STATUS_SUCCESS;
}


/**************************************************************************//**
 * MadEvtDisarmWakeFromS0
 *
 * DESCRIPTION:
 *    This function reverses anything done in EvtDeviceArmWakeFromS0 - above.
 *    This function is not marked pageable because this function is in the
 *    device power up path. When a function is marked pagable and the code
 *    section is paged out, it will generate a page fault which could impact
 *    the fast resume behavior because the client driver will have to wait
 *    until the system drivers can service this page fault.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 *****************************************************************************/
VOID MadEvtDisarmWakeFromS0(IN WDFDEVICE hDevice)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    UNREFERENCED_PARAMETER(pFdoData);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtDisarmWakeFromS0...SerialNo=%d\n", pFdoData->SerialNo);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "<-- MadEvtDisarmWakeFromS0\n");

    return;
}


/************************************************************************//**
 * MadEvtDisarmWakeFromSx
 *
 * DESCRIPTION:
 *    This function reverses anything done in EvtDeviceArmWakeFromSx.
 *
 *    This function will run at PASSIVE_LEVEL.
 *
 *   This function is not marked pageable because this function is in the
 *   device power up path. When a function is marked pagable and the code
 *   section is paged out, it will generate a page fault which could impact
 *   the fast resume behavior because the client driver will have to wait
 *   until the system drivers can service this page fault.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtDisarmWakeFromSx(IN WDFDEVICE hDevice)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    UNREFERENCED_PARAMETER(pFdoData);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtDisarmWakeFromSx...SerialNo=%d\n", pFdoData->SerialNo);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "<-- MadEvtDisarmWakeFromSx\n");

    return;
}


/************************************************************************//**
 * MadEvtWakeFromS0Triggered
 *
 * DESCRIPTION:
 *    This function will be called whenever the device triggers its wake 
 *    signal after being armed for wake.
 *    This function is pageable and runs at PASSIVE_LEVEL.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtWakeFromS0Triggered(IN WDFDEVICE hDevice)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    PAGED_CODE();
	UNREFERENCED_PARAMETER(pFdoData);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtWakeFromS0Triggered...SerialNo=%d\n", pFdoData->SerialNo);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "<-- MadEvtWakeFromS0Triggered\n");

	return;
}


/************************************************************************//**
 * MadEvtWakeFromSxTriggered
 *
 * DESCRIPTION:
 *    This function will be called whenever the device triggers its wake 
 *    signal after being armed for wake. This function runs at PASSIVE_LEVEL.
 *
 *   This function is not marked pageable because this function is in the
 *   device power up path. When a function is marked pagable and the code
 *   section is paged out, it will generate a page fault which could impact
 *   the fast resume behavior because the client driver will have to wait
 *   until the system drivers can service this page fault.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtWakeFromSxTriggered(IN WDFDEVICE hDevice)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    UNREFERENCED_PARAMETER(pFdoData);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		        " MadEvtWakeFromSxTriggered...SerialNo=%d\n", pFdoData->SerialNo);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "<-- MadEvtWakeFromSxTriggered\n");

    return;
}


#ifdef MAD_DEVICE_CONTROL_POWER_MNGT //==============================================================
//
/************************************************************************//**
 * MadEvtPowerCheckTimer
 *
 * DESCRIPTION:
 *    This function is invoked when the power-check timer is trigered.
 *    We will issue a read of config space to see if there is a change of
 *    power state for the device. Then we re-satrt the timer
 *    
 * PARAMETERS: 
 *     @param[in]  hTimer      handle to the triggered timer 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtPowerCheckTimer(WDFTIMER hTimer)

{
NTSTATUS                 NtStatus;
WDFDEVICE hDevice  = (WDFDEVICE)WdfTimerGetParentObject(hTimer);
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);
PDEVICE_OBJECT      pDevObj  = WdfDeviceWdmGetDeviceObject(hDevice);
PDEVICE_OBJECT      pLowrDev = WdfDeviceWdmGetAttachedDevice(hDevice);
MADBUS_DEVICE_CONFIG_DATA  MadDevCnfg;
DEVICE_POWER_STATE   DevPowerState;
POWER_STATE          PowerState;
BOOLEAN              bRC;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtPowerCheckTimer...Power check timer fired for device# %d: ",
                pFdoData->SerialNo);

	pFdoData->MadDevWmiData.InfoClassData.PowerUsed_mW += 
		(gPowrChekTimrMillisecs / 1000) * MAD_MILLIWATTS_PER_IDLE_SECOND;

    RtlFillMemory(&MadDevCnfg, sizeof(MADBUS_DEVICE_CONFIG_DATA), 0x00);
    NtStatus = MadDev_GetCnfgDataFromPdo(pLowrDev, pDevObj, &MadDevCnfg);
    if (!NT_SUCCESS(NtStatus))
        {
        //TBD: log something
        return;
        }

    DevPowerState = MadDevCnfg.VndrSpecData.DevPowerState;
    if (DevPowerState != pFdoData->CurrDevPowerState)
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtPowerCheckTimer...Power check timer function detects power state change for device# %d, DevPowerState=%d\n",
                    pFdoData->SerialNo, DevPowerState);

		if (DevPowerState <= PowerDeviceD3) //Else it has been unplugged - don't touch it
            {
            PowerState.DeviceState = DevPowerState;
            PowerState = PoSetPowerState(pDevObj, DevicePowerState, PowerState);

            MadDev_IssuePowerNotice(pFdoData, DevPowerState, PowerActionNone);
            }
	    }

	if (DevPowerState <= PowerDeviceD3) //Else the device is unplugged - don't restart the timer
	    {
        bRC = WdfTimerStart(pFdoData->hPowrChekTimr, pFdoData->PowrChekTimrPeriod); 
        ASSERT(!bRC); //Because our timer should not still be in the system timer queue after firing when we set period=0 
	    }

    return;
}
#endif //MAD_DEVICE_CONTROL_POWER_MNGT ......................................................................

#ifdef POWER_MANAGEMENT_ENABLED 
/************************************************************************//**
 * MadDev_IssuePowerNotice
 *
 * DESCRIPTION:
 *    This function issues a power notice for any change of power state.
  *   This function is kind of hokey but we don't have real hardware.
 *    
 * PARAMETERS: 
 *     @param[in]  pFdoData      pointer to the framework device extension
 *     @param[in]  DevPowerState the new device power state  
 *     @param[in]  SysPowerAxn   the relevant system power action if any
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
void MadDev_IssuePowerNotice(PFDO_DATA pFdoData,
						     DEVICE_POWER_STATE DevPowerState, POWER_ACTION SysPowerAxn)

{
static POWERNOTIFY_TYPE PowerNotify = INIT_POWERAXN; 
static  WCHAR wcNotifyFile[] = POWERAXNFILE_WC;

NTSTATUS      NtStatus;
BOOLEAN       bRC;
ULONG         PowerNotice;
ULONG         PowerState;
LARGE_INTEGER liDelay;    

    ASSERT(VALID_KERNEL_ADDRESS(pFdoData));

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDev_IssuePowerNotice: pFdoData=%p, SerialNo=%d, PowerState=%d, SysPowerAxn=%d\n",
			    pFdoData, pFdoData->SerialNo, (DevPowerState-1), SysPowerAxn);

	//* Support for Simulator display of Power State
	//* Enum 1 -> State 0, Enum 2 -> State 1,  etc.
	pFdoData->CurrDevPowerState = DevPowerState;
	PowerState = (ULONG)((LONG)(pFdoData->CurrSysPowerState << 4) +
		         (LONG)(pFdoData->CurrDevPowerState - 1));

//First - program the hardware for the power state change. This is trivial in the prototype
//
	MadInterruptAcquireLock(pFdoData->hInterrupt);
	WRITE_REGISTER_ULONG(&pFdoData->pMadRegs->PowerState, PowerState);
	MadInterruptReleaseLock(pFdoData->hInterrupt);

//* We will communicate this power activity to the Simulator & the Monitor program
//*
//* Let's write a message to a disk file about this Power Mngt activity
//* to be parsed by the Monitor program
//
    PowerNotice = ((UCHAR)(SysPowerAxn << 8)) | (UCHAR)PowerState;
    PowerNotify.PowerNotices[pFdoData->SerialNo] = PowerNotice;
    bRC = Write_Power_Notice(wcNotifyFile, (PVOID)&PowerNotify, sizeof(POWERNOTIFY_TYPE));

//* *IF* The system is powering down then ...
//* let's introduce a delay so that the monitor program can sneak a peak at the 
//* disk message.
//*	This seems like the right place to do it.
//* Just before completion of system shutdown caused by Hibernate
//*
    if (SysPowerAxn != PowerActionNone) //* Sleep_1 ... Shutdown
   	    {
        // .500 sec. delay
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDev_IssuePowerNotice Inserting a .500 second delay during shutdown\n"); 

        liDelay.LowPart = (ULONG)(-1 * 10 * 1000 * 500); 
        NtStatus = KeDelayExecutionThread(KernelMode, FALSE, &liDelay);     
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDev_IssuePowerNotice KeDelayExThread returns %08X\n", NtStatus);
        }

    return;
}


/************************************************************************//**
 * Write_Power_Notice
 *
 * DESCRIPTION:
 *    This function writes a record to the specified file.
 *    
 * PARAMETERS: 
 *     @param[in]  wcNotifyFile   wide-character format filename.
 *     @param[in]  pPowerNotify   pointerto the data to be written
 *     @param[in]  Len            Length of data  
 *     
 * RETURNS:
 *    @return      BOOLEAN       success/fail
 * 
 ***************************************************************************/

BOOLEAN Write_Power_Notice(WCHAR wcNotifyFile[], PVOID pPowerNotify, ULONG Len)

{
NTSTATUS NtStatus, NtStatus_wr = 0xC0000001;
BOOLEAN bRC;
HANDLE hZwFile;
IO_STATUS_BLOCK IoStatus;
UNICODE_STRING PowerNotify;
OBJECT_ATTRIBUTES lObjAttrs;

    RtlInitUnicodeString(&PowerNotify, wcNotifyFile);
    InitializeObjectAttributes(&lObjAttrs, &PowerNotify,
		                       (OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE),
		                        NULL, (PSECURITY_DESCRIPTOR)NULL/*&SecDesc*/);
    NtStatus =
    ZwCreateFile(&hZwFile, (FILE_WRITE_DATA|SYNCHRONIZE),
	             &lObjAttrs, &IoStatus, 0L,
		         FILE_ATTRIBUTE_NORMAL, FILE_SHARE_DELETE, FILE_SUPERSEDE, 
                 (FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT),
		         NULL, 0L);
    if (NtStatus == STATUS_SUCCESS)
	    {
        NtStatus_wr = 
		ZwWriteFile(hZwFile, NULL, NULL, NULL, 
    	            &IoStatus, pPowerNotify, Len , NULL, 0L);

        NtStatus = ZwClose(hZwFile);
 	    }

    bRC = (NtStatus == STATUS_SUCCESS) && (NtStatus_wr == STATUS_SUCCESS);

    return bRC;
}
#endif POWER_MANAGEMENT_ENABLED 


/*++
New Routine Description:
    DbgDevicePowerString converts the device power state code of a power IRP to a
    text string that is helpful when tracing the execution of power IRPs.

Parameters Description:
    Type
    Type specifies the device power state code of a power IRP.

Return Value Description:
    DbgDevicePowerString returns a pointer to a string that represents the
    text description of the incoming device power state code.
--*/
PCHAR DbgDevicePowerString(IN WDF_POWER_DEVICE_STATE Type)
{
    PAGED_CODE();

    switch (Type)
    {
    case WdfPowerDeviceInvalid:
        return "WdfPowerDeviceInvalid";
    case WdfPowerDeviceD0:
        return "WdfPowerDeviceD0";
    case PowerDeviceD1:
        return "WdfPowerDeviceD1";
    case WdfPowerDeviceD2:
        return "WdfPowerDeviceD2";
    case WdfPowerDeviceD3:
        return "WdfPowerDeviceD3";
    case WdfPowerDeviceD3Final:
        return "WdfPowerDeviceD3Final";
    case WdfPowerDevicePrepareForHibernation:
        return "WdfPowerDevicePrepareForHibernation";
    case WdfPowerDeviceMaximum:
        return "PowerDeviceMaximum";
    default:
		GENERIC_SWITCH_DEFAULTCASE_ASSERT;
        return "UnKnown Device Power State";
    }
}


PCHAR PowerMinorFunctionString(UCHAR MinorFunction)

{
    switch (MinorFunction)
        {
        case IRP_MN_SET_POWER:
            return "IRP_MN_SET_POWER";
        case IRP_MN_QUERY_POWER:
            return "IRP_MN_QUERY_POWER";
        case IRP_MN_POWER_SEQUENCE:
            return "IRP_MN_POWER_SEQUENCE";
        case IRP_MN_WAIT_WAKE:
            return "IRP_MN_WAIT_WAKE";

        default:
			GENERIC_SWITCH_DEFAULTCASE_ASSERT;
            return "IRP_MN_?????";
        }
}

