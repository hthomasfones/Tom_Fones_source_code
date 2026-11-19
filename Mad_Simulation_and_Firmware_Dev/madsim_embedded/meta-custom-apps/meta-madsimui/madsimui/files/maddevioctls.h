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
#ifndef NUM_BASE_REGS
    #define NUM_BASE_REGS 22
#endif
struct _MADCTLPARMS
{
	U32         Parm;
	U32         Val;
	uint32_t    MadRegs[NUM_BASE_MADREGS];
};
typedef struct _MADCTLPARMS  MADCTLPARMS, *PMADCTLPARMS;

 struct _CACHE_TRANSFER
{
	U32         length;
    u8          databufr[MAD_CACHE_SIZE_BYTES];
};
typedef struct _CACHE_TRANSFER CACHE_TRANSFER, *PCACHE_TRANSFER;

#define MADDEV_IOCTL_MAGIC        MAD_UNUSED_LINUX_IOC_MAGIC_NUMBER
#define MADDEV_IOCTL_INDEX_BASE   0

#define MADDEV_IOCTL_INIT     _IO(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE)
#define MADDEV_IOCTL_RESET    _IO(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+1)
//#define MADDEV_IOCTL_SEEK     _IO(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+2, unsigned long))
//
#define MADDEV_IOCTL_GET_DEVICE        _IOR(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+2, struct _MADCTLPARMS)
#define MADDEV_IOCTL_GET_ENABLE        _IOR(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+3, unsigned long*)
#define MADDEV_IOCTL_GET_CONTROL       _IOR(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+4, unsigned long*)
//
#define MADDEV_IOCTL_SET_ENABLE        _IOW(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+5, unsigned long)
#define MADDEV_IOCTL_SET_CONTROL       _IOW(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+6, unsigned long)
//
#define MADDEV_IOCTL_SET_READ_INDX     _IOW(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+7, unsigned long)
#define MADDEV_IOCTL_SET_WRITE_INDX    _IOW(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+8, unsigned long)

#define MADDEV_IOCTL_PULL_READ_CACHE    _IOR(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+9, struct _CACHE_TRANSFER)
#define MADDEV_IOCTL_PUSH_WRITE_CACHE   _IOW(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+10, struct _CACHE_TRANSFER)
//
#define MADDEV_IOCTL_ALIGN_READ_CACHE   _IOW(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+11, unsigned long)
#define MADDEV_IOCTL_ALIGN_WRITE_CACHE  _IOW(MADDEV_IOCTL_MAGIC, MADDEV_IOCTL_INDEX_BASE+12, unsigned long)
//
#define MADDEV_IOCTL_MAX_NBR 13

//
#endif  //_MADIOCTLS_H_
