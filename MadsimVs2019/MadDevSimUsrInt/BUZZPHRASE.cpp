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
/*  Module  NAME : BuzzPhrase.cpp                                              */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s) - generates a buzzphrase             */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "BuzzPhrase.h"

TCHAR* words0[] = {
	_T("integrated"),	// 0
	_T("total"),		// 1
	_T("systematized"),	// 2
	_T("parallel"),		// 3
	_T("functional"),	// 4
	_T("responsive"),	// 5
	_T("optional"),		// 6
	_T("synchronized"),	// 7
	_T("compatible"),	// 8
	_T("balanced")      // 9
};	

TCHAR* words1[] = {
	_T("management"),	// 0
	_T("organizational"),	// 1
	_T("monitored"),	// 2
	_T("reciprocal"),	// 3
	_T("digital"),		// 4
	_T("logistical"),	// 5
	_T("transitional"),	// 6
	_T("incremental"),	// 7
	_T("5th-generation"),	// 8
	_T("policy")     	// 9
};	

TCHAR* words2[] = {
	_T("options"),		// 0
	_T("flexibility"),	// 1
	_T("capability"),	// 2
	_T("mobility"),		// 3
	_T("programming"),	// 4
	_T("concept"),		// 5
	_T("paradigm"),		// 6
	_T("projection"),	// 7
	_T("hardware"),		// 8
	_T("interface")     // 9
};	


void BuzzPhrase(CString& csPhrase, BOOL bGenRndmPkts = TRUE)

{
static int nStatDx2 = 0, nStatDx1 = 0, nStatDx0 = 0;
int nDx2, nDx1, nDx0; 
//int n = rand() % 10;

    if (bGenRndmPkts) //* Randomize
		{
        nDx0 = rand() % 10;
        nDx1 = rand() % 10;
        nDx2 = rand() % 10;
		}
    else             //* Cycle through the buzzphrase set predictably
		{
        nDx0 = nStatDx0;
        nDx1 = nStatDx1;
        nDx2 = nStatDx2;

		nStatDx2 = (nStatDx2 + 1) % 10;
		if (nStatDx2 == 0)
			{
			nStatDx1 = (nStatDx1 + 1) % 10;
			if (nStatDx1 == 0)
				nStatDx0 = (nStatDx0 + 1) % 10;
			}
		}

//* Build the buzzphrase by concatenating the chosen buzzwords
//*
	csPhrase += words0[nDx0];
	csPhrase += _T(" ");

	csPhrase += words1[nDx1];
	csPhrase += _T(" ");

	csPhrase += words2[nDx2];

	return;
}
