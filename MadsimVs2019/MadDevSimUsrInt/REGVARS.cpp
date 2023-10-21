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
/*  Module  NAME : RegVars.cpp                                                 */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

#include "stdafx.h"
#include "registry.h"
#include "regvars.h"

/****************************************************************************
*   						  RegistryVar::getInt
* Inputs:
*   	UINT id: ID value
*	int def: Default value (= 0)
*	HKEY root: default HKEY_CURRENT_USER
* Result: int
*   	
* Effect: 
*   	Retrieves the integer value
****************************************************************************/

int RegistryVar::getInt(UINT id, int def, HKEY root)

{
	DWORD value;
	CString pathname;
#pragma warning(suppress: 6031)
	pathname.LoadString(id);
	if (!GetRegistryInt(root, pathname, value))
		return def;
	return (int) value;
}


/****************************************************************************
*   						  RegistryVar::setInt
* Inputs:
*   	UINT id: ID to set
*	int value: Value to set
*	HKEY root: default HKEY_CURRENT_USER
* Result: void
*   	
* Effect: 
*   	Sets the registry key
****************************************************************************/

void RegistryVar::setInt(UINT id, int value, HKEY root)

{
	CString pathname;

#pragma warning(suppress: 6031)
	pathname.LoadString(id);
	SetRegistryInt(root, pathname, value);
}

/****************************************************************************
*   						RegistryVar::setString
* Inputs:
*   	UINT id: ID of variable in 
*	LPCTSTR value: String value to save
*	HKEY root: default HKEY_CURRENT_USER
* Result: void
*   	
* Effect: 
*   	Stores the string value in the registry
****************************************************************************/

void RegistryVar::setString(UINT id, LPCTSTR value, HKEY root)

{
	CString pathname;

#pragma warning(suppress: 6031)
	pathname.LoadString(id);

	SetRegistryString(root, pathname, value);
}

/****************************************************************************
*   						RegistryVar::getString
* Inputs:
*   	UINT id: ID of subkey
*	LPCTSTR def: Default value, or NULL
*	HKEY root: default HKEY_CURRENT_USER
* Result: CString
*   	The string value loaded
* Effect: 
*   	Loads the string from the registry
****************************************************************************/

CString RegistryVar::getString(UINT id, LPCTSTR def, HKEY root)

{
	CString pathname;
	CString value;

#pragma warning(suppress: 6031)
	pathname.LoadString(id);

	if (!GetRegistryString(root, pathname, value))
		{
		/* failed */
		if (def != NULL)
			value = def;
		else
			value = _T("");
		} /* failed */
	return value;
}

/****************************************************************************
*   						   RegistryInt::load
* Result: BOOL
*   	TRUE if loaded successfully
*	FALSE if no load occurred
* Effect: 
*   	Loads the 'value'
****************************************************************************/

int RegistryInt::load(int def)

{
	value = getInt(id, def, root);
	return value;
}

/****************************************************************************
*   						  RegistryInt::store
* Result: void
*   	
* Effect: 
*   	Stores the 'value'
****************************************************************************/

void RegistryInt::store()

{
	setInt(id, value, root);
}

/****************************************************************************
*   						 RegistryString::load
* Inputs:
*   	LPCTSTR def: Default value, or NULL if no default
* Result: CString
*   	The string that was loaded
* Effect: 
*   	Loads the string
****************************************************************************/

CString RegistryString::load(LPCTSTR def)

{
	value = getString(id, def, root);
	return value;
}

/****************************************************************************
*   						 RegistryString::store
* Result: void
*   	
* Effect: 
*   	Stores the variable
****************************************************************************/

void RegistryString::store()

{
	setString(id, (LPCTSTR) value, root);
}
