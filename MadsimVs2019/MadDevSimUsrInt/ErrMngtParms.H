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
/*  Module  NAME : ErrMngtParms.h                                              */
/*                                                                             */
/*  DESCRIPTION  : Definitons of error probabilities                           */
/*                                                                             */
/*******************************************************************************/


//typedef enum {eNoIntErrs = 0, eLoseInts, eXtraInts, eBothErrs} etIntErrStyle; 

#define DefltProb    10

#define PROB_ERR	  0
#define PROB_LOST     1
#define PROB_SPURIOUS 2
#define PROB_OVRUND   3
#define PROB_DEVBUSY  4