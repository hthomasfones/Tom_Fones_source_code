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
/*  Module  NAME : Registry.cpp                                                */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "registry.h"
#include "resource.h"

/****************************************************************************
*   								makeKey
* Inputs:
*	HKEY root: Root of key
*   	const CString & path: Path of the form
*			a\b\c\d...
*	HKEY * key: Result of opening key
* Result: LONG
*   	The result of ::RegOpenKey
* Effect: 
*   	If the path cannot be opened, tries to back off creating the key
*	one level at a time
****************************************************************************/

static LONG makeKey(HKEY root, const CString& path, HKEY* key)

{
	LONG result = ::RegOpenKey(root, path, key);
	if (result == ERROR_SUCCESS)
		return result;

	// We have a path of the form a\b\c\d  
	// But apparently a/b/c doesn't exist

	CString s;
	int i = path.ReverseFind(_T('\\'));
	if (i == -1)
		return result;  // well, we lose

	HKEY newkey;
	s = path.Left(i);
	result = makeKey(root, s, & newkey);
	if (result != ERROR_SUCCESS)
		return result;

	// OK, we now have created a\b\c
	CString v;
	v = path.Right(path.GetLength() - i - 1);
	DWORD disp;

	result = ::RegCreateKeyEx(newkey,
			 	v,
			 	0,
			 	NULL,
			 	REG_OPTION_NON_VOLATILE,
			 	KEY_ALL_ACCESS,
			 	NULL,
			 	key,
			 	& disp);
	return result;
}

/****************************************************************************
*   							   makePath
* Inputs:
*   	CString & path: Existing path
*	const CString & var: Value or subpath
*	CString & name: Place to put name
* Result: void
*   	
* Effect: 
*   	Takes a path of the form
*		\this\that
*	and a variable name 'var' of the form
*		whatever\substring
*	and updates 'path' so that it is
*		\this\that\whatever
*	and updates 'name' so that it is
*		substring
****************************************************************************/

static void makePath(CString& path, const CString& var, CString& name)

{
	// locate the rightmost \ of the 'var'.  If there isn't one,
	// we simply copy var to name...

	int i = var.ReverseFind(_T('\\'));
	if (i == -1)
		{
		/* no path */
		name = var;
		return;
		} /* no path */

	// append the prefix of the var to the path, leaving only the name
	if (path[path.GetLength() - 1] != _T('\\'))
		path += _T("\\");
	path += var.Left(i);
	name = var.Right(var.GetLength() - i - 1);
}

/****************************************************************************
*   						   GetRegistryString
* Inputs:
*	HKEY root: HKEY_CURRENT_USER, etc.
*   	const CString &var: Name of variable
*	CString &value: place to put value
* Result: BOOL
*   	TRUE if registry key found, &val is updated
*	FALSE if registry key not found, &val is not modified
* Effect: 
*   	Retrieves the key based on 
*	HKEY_CURRENT_USER\IDS_PROGRAM_ROOT\var
*	HKEY_LOCAL_MACHINE\IDS_PROGRAM_ROOT\var
* Notes:
*	This presumes the value is a text string (SZ_TEXT)
****************************************************************************/

BOOL GetRegistryString(HKEY root, const CString& var, CString& val)

{
	CString path;
#pragma warning(suppress: 6031)
	path.LoadString(IDS_PROGRAM_ROOT);

	CString name;
	makePath(path, var, name);

	HKEY key;
	LONG result = makeKey(root, path, & key);
	if (result != ERROR_SUCCESS)
		return FALSE;
	TCHAR buffer[256];
	DWORD len = sizeof(buffer) / sizeof(TCHAR);
	DWORD type;

	result = ::RegQueryValueEx(key, name, 0, & type, (LPBYTE) buffer, & len);
	::RegCloseKey(key);

	if (result != ERROR_SUCCESS)
		return FALSE;

	if (type != REG_SZ)
		return FALSE;

	val = buffer;

	return TRUE;
}

/****************************************************************************
*   							GetRegistryInt
* Inputs:
*	HKEY  root: root of path
*   	const CString &var: Name of variable
*	LONG &val: Place to put value
* Result: BOOL
*   	TRUE if registry key found, &val is updated
*	FALSE if registry key not found, &val is not modified
* Effect: 
*   	Retrieves the key based on 
*	HKEY_CURRENT_USER\IDS_PROGRAM_ROOT\var
*	HKEY_LOCAL_MACHINE\IDS_PROGRAM_ROOT\var
* Notes:
*	This presumes the value is a 32-bit value
****************************************************************************/

BOOL GetRegistryInt(HKEY root, const CString& var, DWORD& val)

{
	CString path;
#pragma warning(suppress: 6031)
	path.LoadString(IDS_PROGRAM_ROOT);

	CString name;
	makePath(path, var, name);

	HKEY key;
	LONG result = makeKey(root, path, & key);
	if (result != ERROR_SUCCESS)
		return FALSE;

	DWORD buffer;
	DWORD len = sizeof(buffer);
	DWORD type;

	result = ::RegQueryValueEx(key, name, 0, & type, (LPBYTE) & buffer, & len);
	::RegCloseKey(key);

	if (result != ERROR_SUCCESS)
		return FALSE;

	if (type != REG_DWORD)
		return FALSE;

	val = buffer;

	return TRUE;
}

/****************************************************************************
*   						 GetRegistryDWordArray
* Inputs:
*	HKEY  root: root of path
*   	const CString &var: Name of variable
* Result: CDWordArray
*   	The array of data
*	NULL if there is an error
* Effect: 
*   	Allocates a DWORD array.  
* Notes:
*	The caller is responsible for deleting the result
****************************************************************************/

CDWordArray* GetRegistryDWordArray(HKEY root, const CString& var)

{
	CString path;
	DWORD len;
	DWORD type;

#pragma warning(suppress: 6031)
	path.LoadString(IDS_PROGRAM_ROOT);
	CString name;
	makePath(path, var, name);

	HKEY key;
	LONG result = makeKey(root, path, & key);
	if (result != ERROR_SUCCESS)
		return NULL;

	result = ::RegQueryValueEx(key, name, 0, & type, NULL, & len);

	if (result != ERROR_SUCCESS)
		{
		/* failed */
		::RegCloseKey(key);
		return NULL;
		} /* failed */

	CDWordArray* data = new CDWordArray;
	DWORD count = len / sizeof(DWORD);

	data->SetSize(count); // preallocate the array data

	result = ::RegQueryValueEx(key,
			 	name,
			 	0,
			 	& type,
			 	(LPBYTE) & (*data)[0],
			 	& len);

	if (result != ERROR_SUCCESS)
		{
		/* failed */
		::RegCloseKey(key);
		delete data;
		return NULL;
		} /* failed */

	::RegCloseKey(key);

	return data;
}

/****************************************************************************
*   						 SetRegistryDWordArray
* Inputs:
*	HKEY  root: root of path
*   	const CString &var: Name of variable
*	CDWordArray & data: Data to write
* Result: BOOL
*   	TRUE if successful
*	FALSE if error
* Effect: 
*   	Writes the data for the key
****************************************************************************/

BOOL SetRegistryDWordArray(HKEY root, const CString& var, CDWordArray& data)

{
	CString path;

#pragma warning(suppress: 6031)
	path.LoadString(IDS_PROGRAM_ROOT);
	CString name;
	makePath(path, var, name);

	HKEY key;
	LONG result = makeKey(root, path, & key);
	if (result != ERROR_SUCCESS)
		return FALSE;

	result = ::RegSetValueEx(key,
			 	name,
			 	0,
			 	REG_BINARY,
			 	(LPBYTE) & (data[0]),
			 	(DWORD)data.GetSize() * sizeof(DWORD));
	::RegCloseKey(key);
	return result == ERROR_SUCCESS;
}

/****************************************************************************
*   						   SetRegistryString
* Inputs:
*	HKEY root: root of search
*   	const CString & var: Name of variable
*	CString & val: Value to write
* Result: BOOL
*   	TRUE if registry key set
*	FALSE if registry key not set
* Effect: 
*   	Retrieves the key based on 
*	HKEY_CURRENT_USER\IDS_PROGRAM_ROOT\var
*	HKEY_LOCAL_MACHINE\IDS_PROGRAM_ROOT\var
* Notes:
*	This presumes the value is a string
****************************************************************************/

BOOL SetRegistryString(HKEY root, const CString& var, const CString& val)

{
	CString path;
#pragma warning(suppress: 6031)
	path.LoadString(IDS_PROGRAM_ROOT);
	CString name;
	makePath(path, var, name);

	HKEY key;
	DWORD disp;
	LONG result = ::RegCreateKeyEx(root,
				  	path,
				  	0,
				  	NULL,
				  	REG_OPTION_NON_VOLATILE,
				  	KEY_ALL_ACCESS,
				  	NULL,
				  	& key,
				  	& disp);
	if (result != ERROR_SUCCESS)
		return FALSE;

	result = ::RegSetValueEx(key,
			 	name,
			 	0,
			 	REG_SZ,
			 	(LPBYTE) (LPCTSTR) val,
			 	lstrlen(val));
	::RegCloseKey(key);

	if (result != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

/****************************************************************************
*   						   SetRegistryInt
*	HKEY root : root of search
*   	const CString var: Name of variable, including possibly path info
*	DWORD val: Place to put value
* Result: BOOL
*   	TRUE if registry key set
*	FALSE if registry key not set
* Effect: 
*   	Retrieves the key based on 
*	HKEY_CURRENT_USER\IDS_PROGRAM_ROOT\var
*	HKEY_LOCAL_MACHINE\IDS_PROGRAM_ROOT\var
* Notes:
*	This presumes the value is a string
****************************************************************************/

BOOL SetRegistryInt(HKEY root, const CString& var, DWORD val)

{
	CString path;
#pragma warning(suppress: 6031)
	path.LoadString(IDS_PROGRAM_ROOT);
	CString name;

	makePath(path, var, name);

	HKEY key;
	DWORD disp;
	LONG result = ::RegCreateKeyEx(root,
				  	path,
				  	0,
				  	NULL,
				  	REG_OPTION_NON_VOLATILE,
				  	KEY_ALL_ACCESS,
				  	NULL,
				  	& key,
				  	& disp);
	if (result != ERROR_SUCCESS)
		return FALSE;

	result = ::RegSetValueEx(key,
			 	name,
			 	0,
			 	REG_DWORD,
			 	(LPBYTE) & val,
			 	sizeof(DWORD));
	::RegCloseKey(key);

	if (result != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

/****************************************************************************
*   						   EnumRegistryKeys
* Inputs:
*   	HKEY root: Root of search
*	const CString & group: Name of group key
* Result: CStringArray *
*   	Array of key names, NULL if no keys found
* Effect: 
*   	Enumerates the keys
****************************************************************************/

CStringArray* EnumRegistryKeys(HKEY root, const CString& group)

{
	CString path;

	CStringArray* keys;
	TCHAR itemName[512];

#pragma warning(suppress: 6031)
	path.LoadString(IDS_PROGRAM_ROOT);

	path += _T("\\");
	path += group;


	HKEY key;

	LONG result = makeKey(root, path, & key);
	if (result != ERROR_SUCCESS)
		return NULL;

	keys = new CStringArray;
	DWORD i = 0;
	while (TRUE)
		{
		/* loop */
		LONG result = ::RegEnumKey(key,
					  	i,
					  	itemName,
					  	sizeof(itemName) / sizeof(TCHAR));
		if (result != ERROR_SUCCESS)
			break;
		// we have a valid key name
		keys->SetAtGrow(i, itemName);
		i++;
		} /* loop */

	::RegCloseKey(key);
	return keys;
}

/****************************************************************************
*   						   EnumRegistryValues
* Inputs:
*   	HKEY root: Root of search
*	const CString & group: Name of value group key
* Result: CStringArray *
*   	Array of value names, NULL if no keys found
* Effect: 
*   	Enumerates the keys
****************************************************************************/

CStringArray* EnumRegistryValues(HKEY root, const CString& group)

{
	CString path;

	CStringArray* keys;
	TCHAR itemName[512];

#pragma warning(suppress: 6031)
	path.LoadString(IDS_PROGRAM_ROOT);

	path += _T("\\");
	path += group;


	HKEY key;

	LONG result = makeKey(root, path, & key);
	if (result != ERROR_SUCCESS)
		return NULL;

	keys = new CStringArray;
	DWORD i = 0;
	while (TRUE)
		{
		/* loop */
		DWORD length = sizeof(itemName) / sizeof(TCHAR);
		LONG result = ::RegEnumValue(key, // key selection
					 				  i,   // which key
									  itemName, // place to put value name
				 & length,  // in: length of buffer
									  			// out: length of name
									  NULL, 	// reserved, NULL
									  NULL, 	// place to put type
									  NULL, 	// place to put value
									  NULL);	// place to put value length
		if (result != ERROR_SUCCESS)
			break;
		// we have a valid key name
		keys->SetAtGrow(i, itemName);
		i++;
		} /* loop */

	::RegCloseKey(key);
	return keys;
}
