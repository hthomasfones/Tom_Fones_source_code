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
/*  Exe files   : madsimui.exe                                                 */ 
/*                                                                             */
/*  Module NAME : madsimui.h                                                   */
/*                                                                             */
/*  DESCRIPTION : Properties & Definitions for the MAD simulation UI app       */
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
/* $Id: madsimui.h, v 1.0 2018/01/01 00:00:00 htf $                            */
/*                                                                             */
/*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
//
#include "../../include/maddefs.h"
#include "../../include/madapplib.h"
#include "../../include/madkonsts.h"
#include "../../include/madbusioctls.h"
//
#define  MADBUSOBJNAME   "madbusobjX"
#define  MBDEVNUMDX      9 //......^
#define  MAD_DATA_FILE_PREFIX "maddev"
//
#define nudef kSA
//
#define kEXP  11
//
#define kCBR  21
#define kCBW  22
#define kLRC  23
#define kFWC  24
#define kARC  25
#define kAWC  26
#define kIDD  27
#define kSAV  28
#define kRES  29
#define kHPL  30
#define kHUN  31
//
#define u32 unsigned long

bool Parse_Cmd(int argc, char **argv,
		       int* devnum, int* op, int* parm, int* val);

int Process_Cmd(int fd, int op, int parm, int val);
int Init_Device_Data(int b_s, u8* pDevData);
int Save_Device_Data(u8* pDevData, char* file_name, int devnum, size_t dataLen);
int Load_Device_Data(u8* pDevData, char* file_name, int devnum, size_t dataLen);
int Xfer_Device_Data(u8* pDevData, char* file_prefx, int devnum, size_t dataLen, u8 bWrite);

void display_error_w_help(char* pChar);
void display_help();
void display_regs(PMADREGS pmadregs);
//int read_regs(int fd, PMADREGS pmadregs);
//int write_regs(int fd, PMADREGS pmadregs);
int MapDeviceRegs(PMADREGS pmadregs, int fd);
   
