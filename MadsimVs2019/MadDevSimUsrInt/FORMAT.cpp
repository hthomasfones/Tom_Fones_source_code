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
/*  Module  NAME : Format.cpp                                                  */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "format.h"

/****************************************************************************
*   						 formatError
* Inputs:
*   	DWORD err: Error message
* Result: CString
*   	Display string for error
****************************************************************************/

CString formatError(DWORD err)

{
	LPTSTR msg;

	if (::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			err,
			0,
			(LPTSTR) & msg,
			0,
			NULL))
		{
		/* formatted */
		LPTSTR p = _tcschr(msg, _T('\r'));
		if (p != NULL)
			*p = _T('\0');
		CString s(msg);
		LocalFree(msg);
		return s;
		} /* formatted */
	else
		{
		/* can't format */
		CString s;
		int severity = (err >> 30) & 0x3;
		int system = (err >> 29) & 0x1;
		int value = (err & 0x07FFFFFF);
		s.Format(_T("? %d:%d:%06d ?"), severity, system, value);
		return s;
		} /* can't format */
}
