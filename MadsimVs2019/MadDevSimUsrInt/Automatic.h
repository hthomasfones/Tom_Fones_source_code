/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2022 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* XYZ Company                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadSimUI.exe                                                */
/*                                                                             */
/*  Module  NAME : Automatic.h                                                 */
/*                                                                             */
/*  DESCRIPTION  : Definition of the Automatic class                           */
/*                                                                             */
/*******************************************************************************/


#ifndef AUTOMATIC_H      //* Indicates this file is included 
    #define AUTOMATIC_H  //* Prevents redundancy
#endif

//#define OUTPUTFILE "DToutput.tst"
//
#define MAD_AUTO_POLL_INTERVAL 50  //millisecs

typedef enum {eManual = 0, eAutoInput, eAutoOutput, eAutoDuplex} etRunMode; 

typedef enum {eInitIn = 0, eGenerate, eLoadSendWin, eDataXfer, eWait4Xfer,
              ePacketEnd, eDelayIn} etAutoInState; 


//* Function prototypes: Definitions found in Automatic.cpp 
//*
afx_msg LRESULT	  OnPoll(WPARAM, LPARAM);
        void	  PollInput();
	    void   	  PollOutput();
void Set_Random_Err(PUSHORT puNewStat);