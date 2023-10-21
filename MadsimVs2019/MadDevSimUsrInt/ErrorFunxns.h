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
/*  Module  NAME : ErrorFunxns.h                                               */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes for the error injection functions       */
/*                                                                             */
/*******************************************************************************/

//*
//* ErrorFunxns.h
		BOOL			InjectGenlErr();
	    BOOL			InjectOvrUnd();
	    BOOL			InjectDevBusy();
		VOID            InjectAnyErrs(PMADREGS pMadRegs);
	    BOOL			Probability(CButton& ctl, DWORD p);

//   afx_msg LRESULT	OnUpdateProbabilities(WPARAM, LPARAM);
