#ifndef _MadDeviceMof_h_
#define _MadDeviceMof_h_

// MadDevWmiInfo - MadDevWmiInfo
// MadDevice driver information
#define MadDevWmiInfoGuid \
    { 0xbba21300,0x6dd3,0x11d2, { 0xb8,0x44,0x00,0xc0,0x4f,0xad,0x51,0x71 } }

#if ! (defined(MIDL_PASS))
DEFINE_GUID(MadDevWmiInfo_GUID, \
            0xbba21300,0x6dd3,0x11d2,0xb8,0x44,0x00,0xc0,0x4f,0xad,0x51,0x71);
#endif


typedef struct _MadDevWmiInfo
{
    // How the Model-Abstract is connected to the computer
    ULONG ConnectorType;
    #define MadDevWmiInfo_ConnectorType_SIZE sizeof(ULONG)
    #define MadDevWmiInfo_ConnectorType_ID 1

    // Number of errors that occurred on this device
    ULONG ErrorCount;
    #define MadDevWmiInfo_ErrorCount_SIZE sizeof(ULONG)
    #define MadDevWmiInfo_ErrorCount_ID 2

    // This indicates the total service time (in seconds) of this Model-Abstract-Demo device.
    ULONG ServiceTime;
    #define MadDevWmiInfo_ServiceTime_SIZE sizeof(ULONG)
    #define MadDevWmiInfo_ServiceTime_ID 3

    // This indicates the total throughput (in KiloBytes) of this Model-Abstract-Demo device.
    ULONG IoCountKb;
    #define MadDevWmiInfo_IoCountKb_SIZE sizeof(ULONG)
    #define MadDevWmiInfo_IoCountKb_ID 4

    // This indicates the total power consumption (in milli-watts) of this Model-Abstract-Demo device.
    ULONGLONG PowerUsed_mW;
    #define MadDevWmiInfo_PowerUsed_mW_SIZE sizeof(ULONGLONG)
    #define MadDevWmiInfo_PowerUsed_mW_ID 5

    // The MadDevice Model Name.
    CHAR VariableData[1];
    #define MadDevWmiInfo_ModelName_ID 6

} MadDevWmiInfo, *PMadDevWmiInfo;

// MadDeviceNotifyDeviceArrival - MadDeviceNotifyDeviceArrival
// Notify MadDevice Arrival
#define MadDeviceNotifyDeviceArrivalGuid \
    { 0x01cdaff1,0xc901,0x45b4, { 0xb3,0x59,0xb5,0x54,0x27,0x25,0xe2,0x9c } }

#if ! (defined(MIDL_PASS))
DEFINE_GUID(MadDeviceNotifyDeviceArrival_GUID, \
            0x01cdaff1,0xc901,0x45b4,0xb3,0x59,0xb5,0x54,0x27,0x25,0xe2,0x9c);
#endif


typedef struct _MadDeviceNotifyDeviceArrival
{
    // Device Model Name
    CHAR VariableData[1];
    #define MadDeviceNotifyDeviceArrival_ModelName_ID 1

} MadDeviceNotifyDeviceArrival, *PMadDeviceNotifyDeviceArrival;

// MadDevWmiControl - MadDevWmiControl
// WMI method
#define MadDevWmiControlGuid \
    { 0xcaae7d9f,0xacf7,0x4737, { 0xa4,0xe9,0x01,0xc2,0x9d,0x3f,0xe1,0x94 } }

#if ! (defined(MIDL_PASS))
DEFINE_GUID(MadDevWmiControl_GUID, \
            0xcaae7d9f,0xacf7,0x4737,0xa4,0xe9,0x01,0xc2,0x9d,0x3f,0xe1,0x94);
#endif

//
// Method id definitions for MadDevWmiControl
#define MadDevWmiControl1     1
typedef struct _MadDevWmiControl1_IN
{
    // 
    ULONG InData;
    #define MadDevWmiControl1_IN_InData_SIZE sizeof(ULONG)
    #define MadDevWmiControl1_IN_InData_ID 1

} MadDevWmiControl1_IN, *PMadDevWmiControl1_IN;

#define MadDevWmiControl1_IN_SIZE (FIELD_OFFSET(MadDevWmiControl1_IN, InData) + MadDevWmiControl1_IN_InData_SIZE)

typedef struct _MadDevWmiControl1_OUT
{
    // 
    ULONG OutData;
    #define MadDevWmiControl1_OUT_OutData_SIZE sizeof(ULONG)
    #define MadDevWmiControl1_OUT_OutData_ID 2

} MadDevWmiControl1_OUT, *PMadDevWmiControl1_OUT;

#define MadDevWmiControl1_OUT_SIZE (FIELD_OFFSET(MadDevWmiControl1_OUT, OutData) + MadDevWmiControl1_OUT_OutData_SIZE)

#define MadDevWmiControl2     2
typedef struct _MadDevWmiControl2_IN
{
    // 
    ULONG InData1;
    #define MadDevWmiControl2_IN_InData1_SIZE sizeof(ULONG)
    #define MadDevWmiControl2_IN_InData1_ID 1

    // 
    ULONG InData2;
    #define MadDevWmiControl2_IN_InData2_SIZE sizeof(ULONG)
    #define MadDevWmiControl2_IN_InData2_ID 2

} MadDevWmiControl2_IN, *PMadDevWmiControl2_IN;

#define MadDevWmiControl2_IN_SIZE (FIELD_OFFSET(MadDevWmiControl2_IN, InData2) + MadDevWmiControl2_IN_InData2_SIZE)

typedef struct _MadDevWmiControl2_OUT
{
    // 
    ULONG OutData;
    #define MadDevWmiControl2_OUT_OutData_SIZE sizeof(ULONG)
    #define MadDevWmiControl2_OUT_OutData_ID 3

} MadDevWmiControl2_OUT, *PMadDevWmiControl2_OUT;

#define MadDevWmiControl2_OUT_SIZE (FIELD_OFFSET(MadDevWmiControl2_OUT, OutData) + MadDevWmiControl2_OUT_OutData_SIZE)

#define MadDevWmiControl3     3
typedef struct _MadDevWmiControl3_IN
{
    // 
    ULONG InData1;
    #define MadDevWmiControl3_IN_InData1_SIZE sizeof(ULONG)
    #define MadDevWmiControl3_IN_InData1_ID 1

    // 
    ULONG InData2;
    #define MadDevWmiControl3_IN_InData2_SIZE sizeof(ULONG)
    #define MadDevWmiControl3_IN_InData2_ID 2

} MadDevWmiControl3_IN, *PMadDevWmiControl3_IN;

#define MadDevWmiControl3_IN_SIZE (FIELD_OFFSET(MadDevWmiControl3_IN, InData2) + MadDevWmiControl3_IN_InData2_SIZE)

typedef struct _MadDevWmiControl3_OUT
{
    // 
    ULONG OutData1;
    #define MadDevWmiControl3_OUT_OutData1_SIZE sizeof(ULONG)
    #define MadDevWmiControl3_OUT_OutData1_ID 3

    // 
    ULONG OutData2;
    #define MadDevWmiControl3_OUT_OutData2_SIZE sizeof(ULONG)
    #define MadDevWmiControl3_OUT_OutData2_ID 4

} MadDevWmiControl3_OUT, *PMadDevWmiControl3_OUT;

#define MadDevWmiControl3_OUT_SIZE (FIELD_OFFSET(MadDevWmiControl3_OUT, OutData2) + MadDevWmiControl3_OUT_OutData2_SIZE)


typedef struct _MadDevWmiControl
{
    // MadDevice Control Property
    ULONG ControlValue;
    #define MadDevWmiControl_ControlValue_SIZE sizeof(ULONG)
    #define MadDevWmiControl_ControlValue_ID 1

} MadDevWmiControl, *PMadDevWmiControl;

#define MadDevWmiControl_SIZE (FIELD_OFFSET(MadDevWmiControl, ControlValue) + MadDevWmiControl_ControlValue_SIZE)

#endif
