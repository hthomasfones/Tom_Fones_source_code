/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Krash                                                       */
/*  COPYRIGHT    : (c) 2023 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by Dual/GPL license to the Linux open source   */ 
/* community                                                                   */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe files   : ucrash.o                                                     */ 
/*                                                                             */
/*  Module NAME : ucrash.h                                                     */
/*                                                                             */
/*  DESCRIPTION : Properties & Definitions for the user-mode crash program     */
/*                                                                             */
/*  MODULE_AUTHOR("HTF Consulting");                                           */
/*  MODULE_LICENSE("Dual/GPL");                                                */
/*                                                                             */
/* The source code in this file can be freely used, adapted, and redistributed */
/* in source or binary form, so long as an acknowledgment appears in derived   */
/* source files.  The citation should state that the source code comes from    */
/* a set of source files developed by HTF Consulting                           */
/* http://www.htfconsulting.com                                                */
/*                                                                             */
/* No warranty is attached.                                                    */
/* HTF Consulting takes no responsibility for errors or fitness of use         */
/*                                                                             */
/*                                                                             */
/* $Id: ucrash.h, v 1.0 2023/01/01 00:00:00 htf $                             */
/*                                                                             */
/*******************************************************************************/

#include <chrono>
#include <ostream>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define KRASH_UNUSED_LINUX_IOC_MAGIC_NUMBER  0x9A
#define KRASH_DEV_IOC_MAGIC        KRASH_UNUSED_LINUX_IOC_MAGIC_NUMBER
#define KRASH_DEV_IOC_INDEX_BASE   0
//
#define KRASH_DEV_IOC_KRASH_NUM   _IOW(KRASH_DEV_IOC_MAGIC, \
                                       KRASH_DEV_IOC_INDEX_BASE+1, \
                                       unsigned long)

