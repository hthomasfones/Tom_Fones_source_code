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
/*  Module  NAME : MadDevWmiExecute.cpp                                        */
/*                                                                             */
/*  DESCRIPTION  : Text defines & function definitions                         */
/*                 Derived from WDK-Toaster\Wmi                                */
/*                                                                             */
/*******************************************************************************/

#include "MadDevWmi.h"

/*++
Routine Description:
    This routine enumerates the instances of the MadDevice method class and
    executes the methods in the class for each instance.

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

HRESULT ExecuteMethodsInClass(__in     IWbemServices* WbemServices,
                              __in_opt PWCHAR UserId,
                              __in_opt PWCHAR Password,
                              __in_opt PWCHAR DomainName)
{
HRESULT status = S_OK;

IEnumWbemClassObject* enumerator  = NULL;
IWbemClassObject* classObj        = NULL;
IWbemClassObject* instanceObj     = NULL;

const BSTR className   = SysAllocString(MADDEVICE_MAD_CONTROL_METHOD_CLASS);

VARIANT pathVariable;
_bstr_t instancePath;
ULONG   nbrObjsSought = 1;
ULONG   nbrObjsReturned;
//char x;

    // Create an Enumeration object to enumerate the instances of the given class.
    //
    status = WbemServices->CreateInstanceEnum(className,
                                              WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                              NULL,
                                              &enumerator);
	printf_s("WbemServices->CreateInstanceEnum returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) {
        goto exit;
    }

    // Set authentication information for the interface.
    //
    status = SetInterfaceSecurity(enumerator, UserId, Password, DomainName);
	printf_s("SetInterfaceSecurity returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) {
        goto exit;
    }

    // Get the class object for the method definition.
    //
    status = WbemServices->GetObject(className, 0, NULL, &classObj, NULL);
	printf_s("WbemServices->GetObject returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) {
        goto exit;
    }

    do {// Get the instance object for each instance of the class.
        //
        status = enumerator->Next(WBEM_INFINITE,
                                  nbrObjsSought,
                                  &instanceObj,
                                  &nbrObjsReturned);

        if (status == WBEM_S_FALSE) {
            status = S_OK;
            break;
        }

        if (FAILED(status)) {
            if (status == WBEM_E_INVALID_CLASS) {
                printf("ERROR: MadDevice driver may not be active on the system.\n");
            }
            goto exit;
        }

        // To obtain the object path of the object for which the method has to be
        // executed, query the "__PATH" property of the WMI instance object.
        //
        status = instanceObj->Get(_bstr_t(L"__PATH"), 0, &pathVariable, NULL, NULL);
        if (FAILED(status)) 
            goto exit;

        instancePath = pathVariable.bstrVal;
        instanceObj->Release();
        instanceObj = NULL;

        // Execute the methods in this instance of the class.
        //
#pragma warning(suppress: 6387)
        status = ExecuteMethodOneInInstance(WbemServices, classObj, instancePath, IN_DATA1_VALUE);
        //if (FAILED(status)) {
        //    goto exit;
        //}

#pragma warning(suppress: 6387)
        status = ExecuteMethodTwoInInstance(WbemServices, classObj, instancePath, IN_DATA1_VALUE, IN_DATA2_VALUE);
        //if (FAILED(status)) {
        //    goto exit;
        //}

#pragma warning(suppress: 6387)
        status = ExecuteMethodThreeInInstance(WbemServices, classObj, instancePath, IN_DATA1_VALUE, IN_DATA2_VALUE);
        //if (FAILED(status)) {
        //    goto exit;
        //}
    } while (!FAILED(status));

exit:
    if (className != NULL) {
        SysFreeString(className);
    }

    if (classObj != NULL) {
        classObj->Release();
    }

#pragma warning(suppress: 6102)
    if (enumerator != NULL) {
        enumerator->Release();
    }

#pragma warning(suppress: 6102)
    if (instanceObj != NULL) {
        instanceObj->Release();
    }

    return status;
}


HRESULT
ExecuteMethodOneInInstance(
    __in IWbemServices* WbemServices,
    __in IWbemClassObject* ClassObj,
    __in const BSTR InstancePath,
    __in ULONG InData
    )
{
HRESULT status;

IWbemClassObject* inputParamsObj  = NULL;
IWbemClassObject* inputParamsInstanceObj  = NULL;
IWbemClassObject* outputParamsInstanceObj = NULL;

const BSTR methodName = SysAllocString(MADDEVICE_MAD_CONTROL_METHOD_1);
VARIANT funcParam;
//char x;

    // Get the input parameters class objects for the method.
    //
    status = ClassObj->GetMethod(methodName, 0, &inputParamsObj, NULL);
	printf_s("ClassObj->GetMethod(1) returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) {
        goto exit;
    }

    // Spawn an instance of the input parameters class object.
    //
    status = inputParamsObj->SpawnInstance(0, &inputParamsInstanceObj);
    if (FAILED(status)) {
        goto exit;
    }

    // Set the input variables values (i.e., inData for MadDeviceMethod1).
    //
    funcParam.vt = VT_I4;
    funcParam.ulVal = InData;

    status = inputParamsInstanceObj->Put(L"InData", 0, &funcParam, 0);
    if (FAILED(status)) {
        goto exit;
    }

    // Call the method.
    //
    printf("\n");
    printf("Instance Path .: %ws\n", (wchar_t*)InstancePath);
    printf("  Method Name..: %ws\n", (wchar_t*)methodName);
    status = WbemServices->ExecMethod(InstancePath,
                                      methodName,
                                      0,
                                      NULL,
                                      inputParamsInstanceObj,
                                      &outputParamsInstanceObj,
                                      NULL);
	printf_s("WbemServices->ExecMethod(1) returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) {
        goto exit;
    }

    // Get the "in" Parameter values from the input parameters object.
    //
    status = inputParamsInstanceObj->Get(L"InData", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     InData....: 0x%X\n", funcParam.ulVal);

    // Get the "out" Parameter values from the output parameters object.
    //
    status = outputParamsInstanceObj->Get(L"OutData", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     OutData...: 0x%X\n", funcParam.ulVal);

exit:
    if (methodName != NULL) {
        SysFreeString(methodName);
    }

    if (inputParamsObj != NULL) {
        inputParamsObj->Release();
    }

    if (inputParamsInstanceObj != NULL) {
        inputParamsInstanceObj->Release();
    }

    if (outputParamsInstanceObj != NULL) {
        outputParamsInstanceObj->Release();
    }

    return status;
}


HRESULT
ExecuteMethodTwoInInstance(
    __in IWbemServices* WbemServices,
    __in IWbemClassObject* ClassObj,
    __in const BSTR InstancePath,
    __in ULONG InData1,
    __in ULONG InData2
    )
{
HRESULT status;

IWbemClassObject* inputParamsObj  = NULL;
IWbemClassObject* inputParamsInstanceObj  = NULL;
IWbemClassObject* outputParamsInstanceObj = NULL;

const BSTR methodName = SysAllocString(MADDEVICE_MAD_CONTROL_METHOD_2);
VARIANT funcParam;
//char x;

    // Get the input parameters class objects for the method.
    //
    status = ClassObj->GetMethod(methodName, 0, &inputParamsObj, NULL);
	printf_s("ClassObj->GetMethod(2) returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) {
        goto exit;
    }

    // Spawn an instance of the input parameters class object.
    //
    status = inputParamsObj->SpawnInstance(0, &inputParamsInstanceObj);
    if (FAILED(status)) {
        goto exit;
    }

    // Set the input variables values (i.e., inData1, inData2 for MadDeviceMethod2).
    //
    funcParam.vt = VT_I4;
    funcParam.ulVal = InData1;

    status = inputParamsInstanceObj->Put(L"InData1", 0, &funcParam, 0);
    if (FAILED(status)) {
        goto exit;
    }

    funcParam.vt = VT_I4;
    funcParam.ulVal = InData2;

    status = inputParamsInstanceObj->Put(L"InData2", 0, &funcParam, 0);
    if (FAILED(status)) {
        goto exit;
    }

    // Call the method.
    //
    printf("\n");
    printf("Instance Path .: %ws\n", (wchar_t*)InstancePath);
    printf("  Method Name..: %ws\n", (wchar_t*)methodName);
    status = WbemServices->ExecMethod(InstancePath,
                                      methodName,
                                      0,
                                      NULL,
                                      inputParamsInstanceObj,
                                      &outputParamsInstanceObj,
                                      NULL);
	printf_s("WbemServices->ExecMethod(2) returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) {
        goto exit;
    }

    // Get the "in" Parameter values from the input parameters object.
    //
    status = inputParamsInstanceObj->Get(L"InData1", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     InData1...: 0x%X\n", funcParam.ulVal);

    status = inputParamsInstanceObj->Get(L"InData2", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     InData2...: 0x%X\n", funcParam.ulVal);

    // Get the "out" Parameter values from the output parameters object.
    //
    status = outputParamsInstanceObj->Get(L"OutData", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     OutData...: 0x%X\n", funcParam.ulVal);

exit:
    if (methodName != NULL) {
        SysFreeString(methodName);
    }

    if (inputParamsObj != NULL) {
        inputParamsObj->Release();
    }

    if (inputParamsInstanceObj != NULL) {
        inputParamsInstanceObj->Release();
    }

    if (outputParamsInstanceObj != NULL) {
        outputParamsInstanceObj->Release();
    }

    return status;
}


HRESULT
ExecuteMethodThreeInInstance(
    __in IWbemServices* WbemServices,
    __in IWbemClassObject* ClassObj,
    __in const BSTR InstancePath,
    __in ULONG InData1,
    __in ULONG InData2
    )
{
HRESULT status;

IWbemClassObject* inputParamsObj  = NULL;
IWbemClassObject* inputParamsInstanceObj  = NULL;
IWbemClassObject* outputParamsInstanceObj = NULL;

const BSTR methodName = SysAllocString(MADDEVICE_MAD_CONTROL_METHOD_3);
VARIANT funcParam;
//char x;

    // Get the input parameters class objects for the method.
    //
    status = ClassObj->GetMethod(methodName, 0, &inputParamsObj, NULL);
	printf_s("ClassObj->GetMethod(3) returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) 
        goto exit;

    // Spawn an instance of the input parameters class object.
    //
    status = inputParamsObj->SpawnInstance(0, &inputParamsInstanceObj);
    if (FAILED(status)) 
        goto exit;

    // Set the input variables values (i.e., inData1, inData2 for MadDeviceMethod3).
    //
    funcParam.vt = VT_I4;
    funcParam.ulVal = InData1;

    status = inputParamsInstanceObj->Put(L"InData1", 0, &funcParam, 0);
    if (FAILED(status)) 
        goto exit;

    funcParam.vt = VT_I4;
    funcParam.ulVal = InData2;

    status = inputParamsInstanceObj->Put(L"InData2", 0, &funcParam, 0);
    if (FAILED(status)) 
        goto exit;

    // Call the method.
    //
    printf("\n");
    printf("Instance Path .: %ws\n", (wchar_t*)InstancePath);
    printf("  Method Name..: %ws\n", (wchar_t*)methodName);
    status = WbemServices->ExecMethod(InstancePath,
                                      methodName,
                                      0,
                                      NULL,
                                      inputParamsInstanceObj,
                                      &outputParamsInstanceObj,
                                      NULL);
	printf_s("WbemServices->ExecMethod(3) returns: 0x%X\n", status);
	//scanf_s("%c", &x, 1);
    if (FAILED(status)) 
        goto exit;

    // Get the "in" Parameter values from the input parameters object.
    //
    status = inputParamsInstanceObj->Get(L"InData1", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) 
        goto exit;

	printf("     InData1...: 0x%X\n", funcParam.ulVal);

    status = inputParamsInstanceObj->Get(L"InData2", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) 
        goto exit;

	printf("     InData2...: 0x%X\n", funcParam.ulVal);

    // Get the "out" Parameter values from the output parameters object.
    //
    status = outputParamsInstanceObj->Get(L"OutData1", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) 
        goto exit;

	printf("     OutData1..: 0x%X\n", funcParam.ulVal);

    status = outputParamsInstanceObj->Get(L"OutData2", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) 
        goto exit;
 
    printf("     OutData2..: 0x%X\n", funcParam.ulVal);

exit:
    if (methodName != NULL) 
        SysFreeString(methodName);

    if (inputParamsObj != NULL) 
        inputParamsObj->Release();
 
    if (inputParamsInstanceObj != NULL) 
        inputParamsInstanceObj->Release();

    if (outputParamsInstanceObj != NULL) 
        outputParamsInstanceObj->Release();
 
    return status;
}


