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
/*  Exe files   : madtest.exe                                                  */ 
/*                                                                             */
/*  Module NAME : madtest.h                                                    */
/*                                                                             */
/*  DESCRIPTION : Properties & Definitions for the MAD test app                */
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
/* $Id: madtest.h, v 1.0 2021/01/01 00:00:00 htf $                             */
/*                                                                             */
/*******************************************************************************/

#ifndef  _MADTEST_H_
#define  _MADTEST_H_
//
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <cstdint>
#include <aio.h>
#include <libaio.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
//
#define dma_addr_t  U64
#define phys_addr_t U64
#include "maddefs.h"
#include "madapplib.h"
#include "madkonsts.h"
#include "maddevioctls.h"

#ifdef BIO //Block-mode device
    //#define  MADDEVNAME         "raw/rawX"
    //#define  MADDEVNUMDX        7 //....^
    #define  MADDEVNAME         "maddevb_objX"
    #define  MADDEVNUMDX        11 //.......^
#else //Char-mode device
    #define  MADDEVNAME      "maddevc_objX"
    #define  MADDEVNUMDX     11 //.......^
#endif

typedef PMADREGS* PPMADREGS; //just an env test

#define nudef kSA
//

#define kPRC  15
#define kPWC  16
#define kARC  17
#define kAWC  18
//
#define kMGT  19
#define kPR   20
#define kPW   21
#define kRB   22
#define kWB   23
#define kRBA  24
#define kWBA  25
#define kRBQ  26
#define kWBQ  27
#define kRDI  28
#define kWDI  29
#define kRDR  30
#define kWRR  31

bool Parse_Cmd(int argc, char **argv,
   		int* devnum, int* op, long* val, long *offset, void* *parm);

int Process_Cmd(int fd, int op, long val, long offset, void* parm);

void display_error_w_help(char* pChar);
void display_help();
int GetBufr(char** ppBufr, size_t IoSize);
void InitData(char* pBufr, size_t Len);
void DisplayData(char* pBufr, size_t Len);
int MapDeviceRegs(PMADREGS *ppMadRegs, int fd);
void DisplayDevRegs(PMADREGS pMadRegs);
ssize_t Async_Io(int fd, u8* pBufr, size_t DataLen, size_t offset, u8 bWrite);
ssize_t Queued_Io(int fd, u8* pBufr, size_t DataLen, u8 bWrite);
//
#endif
