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
/*  Exe file ID  : MadDevWmiApp.exe                                            */
/*                                                                             */
/*  Module  NAME : WmiDevGetSet.cpp                                            */
/*                                                                             */
/*  DESCRIPTION  : Text defines & function definitions                         */
/*                 Derived from WDK-Toaster\Wmi                                */
/*                                                                             */
/*******************************************************************************/

#include "MadDevWmi.h"

/*++
Routine Description:
    This routine enumerates the instances of the MadDevice Device Information
    class and gets/sets the value of one of its Properties.

Arguments:
    WbemServices - Pointer to the WBEM services interface used for accessing
        the WMI services.

    UserId - Pointer to the user id information or NULL.

    Password - Pointer to password or NULL. If the user id is not specified,
        this parameter is ignored.

    DomainName - Pointer to domain name or NULL. If the user id is not specified,
        this parameter is ignored.

Return Value:
    HRESULT Status code.
--*/
HRESULT GetAndSetValuesInClass(__in     IWbemServices* WbemServices,
                               __in_opt PWCHAR UserId,
                               __in_opt PWCHAR Password,
                               __in_opt PWCHAR DomainName)

{
HRESULT status = S_OK;
IEnumWbemClassObject* pEnumerator  = NULL;
IWbemClassObject* pClassObj        = NULL;
IWbemClassObject* pInstanceObj     = NULL;

const BSTR className   = SysAllocString(MADDEVICE_INFO_CLASS);

VARIANT instProperty;
ULONG   nbrObjsSought = 1;
ULONG   nbrObjsReturned;
//char x;

    // Create an Enumeration object to enumerate the instances of the given class.
    //
    status = WbemServices->CreateInstanceEnum(className,
                                              WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                              NULL,
                                              &pEnumerator);
	printf_s("WbemServices->CreateInstanceEnum returns: 0x%X, pEnumerator=%p\n", status, pEnumerator);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) 
        goto exit;
 
    // Set authentication information for the interface.
    //
    status = SetInterfaceSecurity(pEnumerator, UserId, Password, DomainName);
	printf_s("SetInterfaceSecurity returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) 
        goto exit;
  
    do {// Get the instance object for each instance of the class.
        //
        status = pEnumerator->Next(WBEM_INFINITE,
                                  nbrObjsSought,
                                  &pInstanceObj,
                                  &nbrObjsReturned);
		printf_s("pEnumerator->Next returns: 0x%X\n", status);
		//scanf_s("%c", &x, 1);
        if (status == WBEM_S_FALSE) {
            status = S_OK;
            break;
        }

        if (FAILED(status)) {
            if (status == WBEM_E_INVALID_CLASS)
                printf("ERROR: MadDevice driver may not be active on the system.\n");
 
			goto exit;
        }

        // To obtain the object path of the object for which the method has to be
        // executed, query the "__PATH" property of the WMI instance object.
        //
        status = pInstanceObj->Get(_bstr_t(L"__PATH"), 0, &instProperty, 0, 0);
		printf_s("pInstanceObj->Get returns: 0x%X\n", status);
		//scanf_s("%c", &x, 1);
        if (FAILED(status)) 
            goto exit;
        
        printf("\n");
        printf("Instance Path .: %ws\n", (wchar_t*)instProperty.bstrVal);

		status = 
		WbemServices->GetObject(instProperty.bstrVal, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pClassObj, NULL);
		printf_s("WbemServices->GetObject returns: 0x%X\n", status);
		//scanf_s("%c", &x, 1);
		//if (FAILED(status))
		//	goto exit;

        // Get the current value(s) of the named property(s).
        //
		status = pInstanceObj->Get(MADDEV_INFO_CONNECTORTYPE, 0, &instProperty, NULL, NULL);
		printf_s("pInstanceObj->Get returns: 0x%X\n", status);
		//scanf_s("%c", &x, 1);
        if (FAILED(status)) 
            goto exit;
        //
		printf("  Property ........: %ws\n", MADDEV_INFO_CONNECTORTYPE);
        printf("  Current Value ...: 0x%X\n", instProperty.lVal);
		
		status = pInstanceObj->Get(MADDEV_INFO_ERRORCOUNT, 0, &instProperty, NULL, NULL);
		printf_s("pInstanceObj->Get returns: 0x%X\n", status);
		//scanf_s("%c", &x, 1);
		if (FAILED(status))
			goto exit;
		//
		printf("  Property ........: %ws\n", MADDEV_INFO_ERRORCOUNT);
		printf("  Current Value ...: 0x%X\n", instProperty.lVal);

		status = pInstanceObj->Get(MADDEV_INFO_SERVICETIME, 0, &instProperty, NULL, NULL);
		printf_s("pInstanceObj->Get returns: 0x%X\n", status);
		//scanf_s("%c", &x, 1);
		if (FAILED(status))
			goto exit;
		//
		printf("  Property ........: %ws\n", MADDEV_INFO_SERVICETIME);
		printf("  Current Value ...: 0x%X\n", instProperty.lVal);

		status = pInstanceObj->Get(MADDEV_INFO_IOCOUNT, 0, &instProperty, NULL, NULL);
		printf_s("pInstanceObj->Get returns: 0x%X\n", status);
		//scanf_s("%c", &x, 1);
		if (FAILED(status))
			goto exit;
		//
		printf("  Property ........: %ws\n", MADDEV_INFO_IOCOUNT);
		printf("  Current Value ...: 0x%X\n", instProperty.lVal);

		status = pInstanceObj->Get(MADDEV_INFO_POWERUSED, 0, &instProperty, NULL, NULL);
		printf_s("pInstanceObj->Get returns: 0x%X\n", status);
		//scanf_s("%c", &x, 1);
		if (FAILED(status))
			goto exit;
		//
		ULONG PowerUsed = (ULONG)instProperty.lVal;
		printf("  Property ........: %ws\n", MADDEV_INFO_POWERUSED);
		printf("  Current Value ...: 0x%X\n", PowerUsed);

		pInstanceObj->Release();
		pInstanceObj = NULL;
    } while (!FAILED(status));

exit:
    if (className != NULL)
        SysFreeString(className);
 
    if (pClassObj != NULL) 
        pClassObj->Release();

    if (pEnumerator != NULL)
        pEnumerator->Release();
 
#pragma warning(suppress: 6102)
    if (pInstanceObj != NULL) 
        pInstanceObj->Release();
 
    return status;
}


