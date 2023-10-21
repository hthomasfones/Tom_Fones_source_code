/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : RAS Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2015 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* MAD Limited                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadBus.sys, MadDevice.sys, MadDisk.sys                      */
/*                                                                             */
/*                                                                             */
/*  Module  NAME : MadSusSysID.h                                               */
/*                                                                             */
/*  DESCRIPTION  : Definition of the the structure Subsystem-Id in the         */
/*                 (PCI) configuration space for a disk device                 */
/*                                                                             */
/*******************************************************************************/

typedef union _MadSubSysID {
	struct {
		USHORT Features : 7;
		USHORT SubType : 3;
		USHORT AdapterType : 2;
		USHORT ConfigType : 3;
		USHORT ExcludeFlag : 1;
	} BitFields;

	USHORT ssid;
} MadSubSysId_U, *pMadSubSysId_U;
