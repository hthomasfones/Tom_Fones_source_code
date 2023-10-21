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
/* Exe file ID  : MadBus.sys, MadDevice.sys                                    */ 
/*                                                                             */
/*  Module  NAME : MadBusDdiQi.h                                               */
/*                                                                             */
/*  DESCRIPTION  : Device-Driver-Interface definitions for the query_interface */
/*                 irp                                                         */
/*                 Drived from WDK-Toaster\bus\driver.h                        */
/*                                                                             */
/*******************************************************************************/

#ifndef _MADBUS_DDI_QUERY_H_
#define _MADBUS_DDI_QUERY_H_

typedef enum _PDO_QUERY_INTF_TYPE  
             {eUndefined=-1, eNullIntfQ, eBusIntfQ, ePciBusIntfQ, ePciBusIntfQ2, 
              eArbtrIntfQ, eXlateIntfQ, eAcpiIntfQ, eIntRouteIntfQ, 
              ePcmciaBusIntfQ, eAcpiRegsIntfQ, eLegacyDevIntfQ, ePciDevIntfQ, 
              eMfEnumIntfQ, eRenumSelfIntfQ, ePnpLocIntfQ, eXAddrIntfQ, 
              eIommuBusIntfQ, eMaxIntfQ} PDO_QUERY_INTF_TYPE;

#ifndef GUID_NULL
    DEFINE_GUID(GUID_NULL, 0L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

#ifdef PDO_DEVICE_DATA_DEF
NTSTATUS MadBusProcessQueryInterfaceRequest(PPDO_DEVICE_DATA  pPdoData,
                                            PDO_QUERY_INTF_TYPE eIntfQ,
                                            _Inout_ PINTERFACE pExposedInterface,
                                            _Inout_opt_ PVOID ExposedInterfaceSpecificData);
#endif //PDO_DEVICE_DATA_DEF
 
// Define Interface reference/dereference routines for
//  Interfaces exported by IRP_MN_QUERY_INTERFACE
typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);

typedef
BOOLEAN
(*PMADDEVICE_GET_POWER_LEVEL)(IN PVOID Context, OUT  PUCHAR Level);

typedef
BOOLEAN
(*PMADDEVICE_SET_POWER_LEVEL)(IN   PVOID Context, OUT  UCHAR Level);

typedef
BOOLEAN
(*PMADDEVICE_IS_CHILD_PROTECTED)(IN PVOID Context);

// Interface for getting and setting power level etc.,
typedef struct _MADDEVICE_INTERFACE_STANDARD
{
    INTERFACE                     InterfaceHeader;
    PMADDEVICE_GET_POWER_LEVEL    GetPowerLevel;
    PMADDEVICE_SET_POWER_LEVEL    SetPowerLevel;
    PMADDEVICE_IS_CHILD_PROTECTED IsSafetyLockEnabled; //):
} MADDEVICE_INTERFACE_STANDARD, *PMADDEVICE_INTERFACE_STANDARD;

inline PDO_QUERY_INTF_TYPE SetEnumIntfType(LPGUID pIntfGuid, GUID GuidTable[])
{
    ULONG j;
    PDO_QUERY_INTF_TYPE eIntfQ = eUndefined;

    for (j = 0; j < eMaxIntfQ; j++)
        if (*pIntfGuid == GuidTable[j])
            {
            eIntfQ = (PDO_QUERY_INTF_TYPE)j;
            break;
            }

    return eIntfQ;
}
#endif //_MADBUS_DDI_QUERY_H_

