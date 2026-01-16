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
/*  Exe files   : madbus.ko, maddevc.ko, maddevb.ko, madsimui.exe, madtest.exe */ 
/*                                                                             */
/*  Module NAME : madkonsts.h                                                  */
/*                                                                             */
/*  DESCRIPTION : General definition of constants for the MAD software         */
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
/* $Id: madkonsts.h, v 1.0 2018/01/01 00:00:00 htf $                           */
/*                                                                             */
/*******************************************************************************/


#define MAD_UNUSED_LINUX_IOC_MAGIC_NUMBER  0x9A /* ? */

//These konstants entered as little-endian
//
#define kHelp    0x682D  // "-h"
#define kQuiet   0x712D  // "-q"
#define kDevice  0x782D  // "-x"
#define kIoSize  0x7A2D  // -z

#define kBR      0x7262
#define kDR      0x7264
#define kRC      0x6372
#define kBW      0x7862
#define kDW      0x7864
#define kWC      0x6378
#define kSA      0x6173
#define kMS      0x736D  //"ms"

//These konstants entered as little-endian
//
#define kGE      0x6567  // "ge"
#define kOU      0x756f  // ou
#define kDB      0x6264  //DB
#define kDF      0x6664
//
//#define kCTL     0
#define kINI     1
#define kRST     2
#define kMAP     3
#define kGET     4
#define kMGET    5
#define kSET     6
//
#define kNOP     99

#define kMSI  0 //"ms"
#define kCTL  1
#define kSTS  2
#define kIEN  3
#define kIID  4


#define MAD_NBR_DEVS 4

#ifndef MADDEVOBJ_MAJOR
#define MADDEVOBJ_MAJOR 0   /* dynamic major by default */
#endif

//#define MADBUSOBJ_MAJOR       1 //memory

//#define __CONSOLE_DEBUG
//
#ifdef __CONSOLE_DEBUG
#undef KERN_DEBUG
#undef KERN_INFO
#undef KERN_NOTICE
#undef KERN_WARNING
#undef KERN_ERR
#undef KERN_CRIT
#undef KERN_ALERT
//#undef KERN_EMERG
//
#define KERN_DEBUG     KERN_EMERG
#define KERN_INFO      KERN_EMERG
#define KERN_NOTICE    KERN_EMERG
#define KERN_WARNING   KERN_EMERG
#define KERN_ERR       KERN_EMERG
#define KERN_CRIT      KERN_EMERG
#define KERN_ALERT     KERN_EMERG
//
#endif //__CONSOLE_DEBUG


