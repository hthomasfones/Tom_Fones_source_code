/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2021 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by Dual/GPL license to the Linux open source   */ 
/* community                                                                   */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe files   : madbus.ko, madsimui.exe                                      */ 
/*                                                                             */
/*  Module NAME : madbusioctls.h                                               */
/*                                                                             */
/*  DESCRIPTION : Ioctl definitions for the MAD bus driver                     */
/*                                                                             */
/*  MODULE_AUTHOR("HTF Consulting");                                           */
/*  MODULE_LICENSE("Dual/GPL");                                                */
/*                                                                             */
/* The source code in this file can be freely used, adapted, and redistributed */
/* in source or binary form, so long as an acknowledgment appears in derived   */
/* source files.  The citation should state that the source code comes from a  */
/* a set of source files developed by HTF Consulting                           */
/* http://www.htfconsulting.com                                                */
/*                                                                             */
/* No warranty is attached.                                                    */
/* HTF Consulting assumes no responsibility for errors or fitness of use       */
/*                                                                             */
/*                                                                             */
/* $Id: madbusioctls.h, v 1.0 2021/01/01 00:00:00 htf $                        */
/*                                                                             */
/*******************************************************************************/

#ifndef  _MADBUSIOCTLS_H_
#define  _MADBUSIOCTLS_H_
/*
 * Ioctl definitions
 */

struct _MADBUS_CTL_PARMS
{
	int      Parm;
	int      Val;
	MADREGS  MadRegs;
};
typedef struct _MADBUS_CTL_PARMS MADBUSCTLPARMS, *PMADBUSCTLPARMS;

#define MADBUS_IOCTL_MAGIC        (MAD_UNUSED_LINUX_IOC_MAGIC_NUMBER+1)
#define MADBUS_IOCTL_INDEX_BASE   0
//
#define MADBUS_IOCTL_RESET        _IO(MADBUS_IOCTL_MAGIC, MADBUS_IOCTL_INDEX_BASE)
#define MADBUS_IOCTL_EXPIRE       _IO(MADBUS_IOCTL_MAGIC, MADBUS_IOCTL_INDEX_BASE+1)
#define MADBUS_IOCTL_GET_DEVICE   _IOR(MADBUS_IOCTL_MAGIC, MADBUS_IOCTL_INDEX_BASE+2, MADBUSCTLPARMS)
//
#define MADBUS_IOCTL_SET_MSI      _IOW(MADBUS_IOCTL_MAGIC, MADBUS_IOCTL_INDEX_BASE+3, ulong)
#define MADBUS_IOCTL_SET_STATUS   _IOW(MADBUS_IOCTL_MAGIC, MADBUS_IOCTL_INDEX_BASE+4, ulong)
#define MADBUS_IOCTL_SET_INTID    _IOW(MADBUS_IOCTL_MAGIC, MADBUS_IOCTL_INDEX_BASE+5, ulong)
//
#define MADBUS_IOCTL_HOT_PLUG     _IOW(MADBUS_IOCTL_MAGIC, MADBUS_IOCTL_INDEX_BASE+6, ulong)
#define MADBUS_IOCTL_HOT_UNPLUG   _IOW(MADBUS_IOCTL_MAGIC, MADBUS_IOCTL_INDEX_BASE+7, ulong)
#define MADBUS_IOCTL_FLUSH_WRITE_CACHE  _IOW(MADBUS_IOCTL_MAGIC, MADBUS_IOCTL_INDEX_BASE+8)
//
#define MADBUS_IOCTL_MAX_NBR 9
//
#endif  //_MADBUSIOCTLS_H_
