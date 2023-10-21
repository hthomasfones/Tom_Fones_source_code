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
/*  Module  NAME : Registry.h                                                  */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes, structures, classes, etc.              */
/*                                                                             */
/*******************************************************************************/

BOOL GetRegistryString(HKEY root, const CString& var, CString& val);
BOOL SetRegistryString(HKEY root, const CString& var, const CString& val);

BOOL GetRegistryInt(HKEY root, const CString& var, DWORD& val);
BOOL SetRegistryInt(HKEY root, const CString& var, DWORD val);

CStringArray* EnumRegistryKeys(HKEY root, const CString& group);
CStringArray* EnumRegistryValues(HKEY root, const CString& group);

CDWordArray* GetRegistryDWordArray(HKEY root, const CString& var);
BOOL SetRegistryDWordArray(HKEY root, const CString& var, CDWordArray& data);
