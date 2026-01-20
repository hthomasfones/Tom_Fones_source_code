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
/* $Id: ucrash.h, v 1.0 2023/01/01 00:00:00 htf $                              */
/*                                                                             */
/*******************************************************************************/

#ifdef UserMode
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

#include <linux/sched.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <pthread.h>
#endif

enum Crash_Funxns {eDivZero, eNullPntr, eInvalidPntr, eReuseFree, eBadFree, eBadPage,
                  eXecMem, eHardRun, eDeadLock, eWatchdog, eLeakMem, eNoMem, eStkOvr, eStkTrash, eUndef=-1}; 
                  
#define Crash_List "z0nrfpxhdwlmvt\0"

#define KRASH_UNUSED_LINUX_IOC_MAGIC_NUMBER  0x9A
#define KRASH_DEV_IOC_MAGIC        KRASH_UNUSED_LINUX_IOC_MAGIC_NUMBER
#define KRASH_DEV_IOC_INDEX_BASE   0
//
#define KRASH_DEV_IOC_KRASH_NUM   _IOW(KRASH_DEV_IOC_MAGIC, \
                                       KRASH_DEV_IOC_INDEX_BASE+1, \
                                       unsigned long)

#ifdef UserMode
typedef struct crash_private_data
{
	int nMinor;

	char buff[1024];

	//struct cdev cdev;

	//struct timer_list ktimer1;

	//struct tasklet_struct krash_tasklet;
	//spinlock_t spinlock1;
	//spinlock_t spinlock2;
    pthread_mutex_t *pmtx;
    //wait_queue_head_t  IO_q;
    int arg;

	//struct device *krash_device;
} crash_private;
#endif
