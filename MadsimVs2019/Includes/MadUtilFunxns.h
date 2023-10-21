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
/*  Exe file ID  : MadBus.sys, MadDevice.sys                                   */
/*                                                                             */
/*  Module  NAME : MadUtilFunxns.h                                             */
/*                                                                             */
/*  DESCRIPTION  : Utility functions for the above kernel drivers              */
/*                                                                             */
/*******************************************************************************/

NTSTATUS       Mad_MapSysAddr2UsrAddr(IN PVOID pKrnlAddr, IN /*ULONG*/ size_t LengthToMap,
                                      OUT PHYSICAL_ADDRESS* ppPhysAddr, OUT PVOID* ppMapdMemUsrMode);
NTSTATUS       Mad_UnmapUsrAddr(IN PVOID pMapdMemUsr);

MADDEV_IO_TYPE MadDevAnalyzeReqType(PMADREGS pMadRegs);

void MadWriteEventLogMesg(PDRIVER_OBJECT pDriverObj, NTSTATUS NtStatMC, 
                          ULONG NumStrings, ULONG LenParms, PWSTR pwsInfoStr);

NTSTATUS MadDev_AllocIrpSend2Parent(PVOID /*PDEVICE_OBJECT*/ pPrntDev,
                                    PVOID /*PDEVICE_OBJECT*/ pDevObj,
                                    PVOID /*PIO_STACK_LOCATION*/ pIoStackLoc);
NTSTATUS 
MadDev_CnfgCmpltnRtn(PVOID /*PDEVICE_OBJECT*/ pDevObj, PVOID pIRP, PVOID pContext);

PVOID MadDevMakeDevCtlIrp(ULONG IoCtlCode);