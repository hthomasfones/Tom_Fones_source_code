/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2018 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by Dual/GPL license to the Linux open source   */ 
/* community                                                                   */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe files   : maddevc.ko, maddevb.ko madtest.exe                           */ 
/*                                                                             */
/*  Module NAME : madioctls.h                                                  */
/*                                                                             */
/*  DESCRIPTION : Ioctl definitions for the MAD device driver(s)               */
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
/* $Id: madioctls.h, v 1.0 2021/01/01 00:00:00 htf $                           */
/*                                                                             */
/*******************************************************************************/

#ifndef  _MADIOCTLS_H_
#define  _MADIOCTLS_H_

/*
 * Ioctl definitions
 */

#define MADDEVOBJ_IOC_MAGIC        MAD_UNUSED_LINUX_IOC_MAGIC_NUMBER
#define MADDEVOBJ_IOC_INDEX_BASE   0

#define MADDEVOBJ_IOC_INIT     _IO(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE)
#define MADDEVOBJ_IOC_RESET    _IO(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+1)
//#define MADDEVOBJ_IOC_SEEK     _IO(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+2, unsigned long))
//
#define MADDEVOBJ_IOC_GET_DEVICE        _IOR(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+2, void*)
#define MADDEVOBJ_IOC_GET_ENABLE        _IOR(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+3, unsigned long*)
#define MADDEVOBJ_IOC_GET_CONTROL       _IOR(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+4, unsigned long*)
//
#define MADDEVOBJ_IOC_SET_ENABLE        _IOW(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+5, unsigned long)
#define MADDEVOBJ_IOC_SET_CONTROL       _IOW(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+6, unsigned long)
//
#define MADDEVOBJ_IOC_SET_READ_INDX     _IOW(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+7, unsigned long)
#define MADDEVOBJ_IOC_SET_WRITE_INDX    _IOW(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+8, unsigned long)

#define MADDEVOBJ_IOC_PULL_READ_CACHE    _IOR(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+9, void*)
#define MADDEVOBJ_IOC_PUSH_WRITE_CACHE   _IOW(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+10, void*)
//
#define MADDEVOBJ_IOC_ALIGN_READ_CACHE   _IOW(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+11, unsigned long)
#define MADDEVOBJ_IOC_ALIGN_WRITE_CACHE  _IOW(MADDEVOBJ_IOC_MAGIC, MADDEVOBJ_IOC_INDEX_BASE+12, unsigned long)
//
#define MADDEVOBJ_IOCTL_MAX_NBR 13

typedef struct _MADCTLPARMS
{
	int         Parm;
	int         Val;
	MADREGS     MadRegs;
    u8 databufr[MAD_SECTOR_SIZE];
} MADCTLPARMS, *PMADCTLPARMS;
//
#endif  //_MADIOCTLS_H_
