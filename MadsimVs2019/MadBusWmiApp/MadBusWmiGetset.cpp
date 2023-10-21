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
/*  Exe file ID  : MadBusWmiApp.exe                                            */
/*                                                                             */
/*  Module  NAME : MadBusWmiGetSet.cpp                                         */
/*                                                                             */
/*  DESCRIPTION  : Text defines & function definitions                         */
/*                 Derived from WDK-Toaster\Wmi                                */
/*                                                                             */
/*******************************************************************************/

#include "MadBusWmi.h"

#pragma warning(suppress: 6102) 
//#pragma warning(suppress: 6387) 

/*--
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
IEnumWbemClassObject* enumerator  = NULL;
IWbemClassObject* classObj        = NULL;
IWbemClassObject* instanceObj     = NULL;

const BSTR className   = SysAllocString(MADBUS_INFO_CLASS);

VARIANT instProperty;
ULONG   nbrObjsSought = 1;
ULONG   nbrObjsReturned;
char x;

    // Create an Enumeration object to enumerate the instances of the given class.
    //
    status = WbemServices->CreateInstanceEnum(className,
                                              WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                              NULL,
                                              &enumerator);
	printf_s("WbemServices->CreateInstanceEnum returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status))
        goto exit;

    // Set authentication information for the interface.
    //
    status = SetInterfaceSecurity(enumerator, UserId, Password, DomainName);
	printf_s("SetInterfaceSecurity returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) 
        goto exit;

    do {// Get the instance object for each instance of the class.
        //
        status = enumerator->Next(WBEM_INFINITE,
                                  nbrObjsSought,
                                  &instanceObj,
                                  &nbrObjsReturned);
		printf_s("pEnumerator->Next returns: 0x%X\n", status);
		scanf_s("%c", &x, 1);
        if (status == WBEM_S_FALSE) {
            status = S_OK;
            break;
        }

        if (FAILED(status)) {
            if (status == WBEM_E_INVALID_CLASS) 
                printf("ERROR: MadBus driver may not be active on the system.\n");
 
			goto exit;
        }

        // To obtain the object path of the object for which the method has to be
        // executed, query the "__PATH" property of the WMI instance object.
        //
        status = instanceObj->Get(_bstr_t(L"__PATH"), 0, &instProperty, 0, 0);
		printf_s("instanceObj->Get (Path) returns: 0x%X\n", status);
        if (FAILED(status)) 
            goto exit;
  
        printf("\n");
        printf("Instance Path .: %ws\n", (wchar_t*)instProperty.bstrVal);

        // Get the current value of the DummyValue property.
        //
        status = instanceObj->Get(MADBUS_INFO_ERRORCOUNT, 0, &instProperty, NULL, NULL);
		printf_s("instanceObj->Get (ErrorCount) returns: 0x%X\n", status);
        if (FAILED(status)) 
            goto exit;

        printf("  Property ....: %ws\n", MADBUS_INFO_ERRORCOUNT);
        printf("  Old Value ...: %d\n", instProperty.lVal);

        // Set a new value for the DummyValue property.
        //
        instProperty.lVal++;

		status = instanceObj->Put(MADBUS_INFO_ERRORCOUNT, 0, &instProperty, 0);
        if (FAILED(status))
            goto exit;

        status = WbemServices->PutInstance(instanceObj,
                                           WBEM_FLAG_UPDATE_ONLY,
                                           NULL,
                                           NULL);
		printf_s("WbemServices->PutInstance returns: 0x%X\n", status);
        if (FAILED(status))
            goto exit;
  
        status = instanceObj->Get(_bstr_t(MADBUS_INFO_ERRORCOUNT), 0, &instProperty, NULL, NULL);
		printf_s("instanceObj->Get (ErrorCount) returns: 0x%X\n", status);
        if (FAILED(status))
            goto exit;

        printf("  New Value ...: %d\n", instProperty.lVal);

        instanceObj->Release();
        instanceObj = NULL;

    } while (!FAILED(status));

exit:
    if (className != NULL)
        SysFreeString(className);

    if (classObj != NULL) 
        classObj->Release();

#pragma warning(suppress: 6102)
    if (enumerator != NULL)
        enumerator->Release();
 
#pragma warning(suppress: 6102)
    if (instanceObj != NULL) 
        instanceObj->Release();
 
    return status;
}


