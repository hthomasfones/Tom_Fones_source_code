#ifndef _MadBusMof_h_
#define _MadBusMof_h_

// MadBusWmiInfo - MadBusWmiInfo
// Mad Bus driver information
#define MadBusWmiInfoGuid \
    { 0x0006a660,0x8f12,0x11d2, { 0xb8,0x54,0x00,0xc0,0x4f,0xad,0x51,0x71 } }

#if ! (defined(MIDL_PASS))
DEFINE_GUID(MadBusWmiInfo_GUID, \
            0x0006a660,0x8f12,0x11d2,0xb8,0x54,0x00,0xc0,0x4f,0xad,0x51,0x71);
#endif


typedef struct _MadBusWmiInfo
{
    // Number of errors that occurred on this bus
    ULONG ErrorCount;
    #define MadBusWmiInfo_ErrorCount_SIZE sizeof(ULONG)
    #define MadBusWmiInfo_ErrorCount_ID 1

    // Current number of devices plugged into the bus.
    ULONG CurrDevCount;
    #define MadBusWmiInfo_CurrDevCount_SIZE sizeof(ULONG)
    #define MadBusWmiInfo_CurrDevCount_ID 2

    // Total number of devices plugged into the bus.
    ULONG TotlDevCount;
    #define MadBusWmiInfo_TotlDevCount_SIZE sizeof(ULONG)
    #define MadBusWmiInfo_TotlDevCount_ID 3

} MadBusWmiInfo, *PMadBusWmiInfo;

#define MadBusWmiInfo_SIZE (FIELD_OFFSET(MadBusWmiInfo, TotlDevCount) + MadBusWmiInfo_TotlDevCount_SIZE)

// MadBusWmiControl - MadBusWmiControl
// WMI method
#define MadBusWmiControlGuid \
    { 0x37f5a699,0x0ac7,0x40b6, { 0xad,0x78,0x34,0x5a,0x0e,0xe6,0x0c,0x17 } }

#if ! (defined(MIDL_PASS))
DEFINE_GUID(MadBusWmiControl_GUID, \
            0x37f5a699,0x0ac7,0x40b6,0xad,0x78,0x34,0x5a,0x0e,0xe6,0x0c,0x17);
#endif

//
// Method id definitions for MadBusWmiControl
#define MadBusWmiControl1     1
typedef struct _MadBusWmiControl1_IN
{
    // 
    ULONG InData;
    #define MadBusWmiControl1_IN_InData_SIZE sizeof(ULONG)
    #define MadBusWmiControl1_IN_InData_ID 1

} MadBusWmiControl1_IN, *PMadBusWmiControl1_IN;

#define MadBusWmiControl1_IN_SIZE (FIELD_OFFSET(MadBusWmiControl1_IN, InData) + MadBusWmiControl1_IN_InData_SIZE)

typedef struct _MadBusWmiControl1_OUT
{
    // 
    ULONG OutData;
    #define MadBusWmiControl1_OUT_OutData_SIZE sizeof(ULONG)
    #define MadBusWmiControl1_OUT_OutData_ID 2

} MadBusWmiControl1_OUT, *PMadBusWmiControl1_OUT;

#define MadBusWmiControl1_OUT_SIZE (FIELD_OFFSET(MadBusWmiControl1_OUT, OutData) + MadBusWmiControl1_OUT_OutData_SIZE)

#define MadBusWmiControl2     2
typedef struct _MadBusWmiControl2_IN
{
    // 
    ULONG InData1;
    #define MadBusWmiControl2_IN_InData1_SIZE sizeof(ULONG)
    #define MadBusWmiControl2_IN_InData1_ID 1

    // 
    ULONG InData2;
    #define MadBusWmiControl2_IN_InData2_SIZE sizeof(ULONG)
    #define MadBusWmiControl2_IN_InData2_ID 2

} MadBusWmiControl2_IN, *PMadBusWmiControl2_IN;

#define MadBusWmiControl2_IN_SIZE (FIELD_OFFSET(MadBusWmiControl2_IN, InData2) + MadBusWmiControl2_IN_InData2_SIZE)

typedef struct _MadBusWmiControl2_OUT
{
    // 
    ULONG OutData;
    #define MadBusWmiControl2_OUT_OutData_SIZE sizeof(ULONG)
    #define MadBusWmiControl2_OUT_OutData_ID 3

} MadBusWmiControl2_OUT, *PMadBusWmiControl2_OUT;

#define MadBusWmiControl2_OUT_SIZE (FIELD_OFFSET(MadBusWmiControl2_OUT, OutData) + MadBusWmiControl2_OUT_OutData_SIZE)

#define MadBusWmiControl3     3
typedef struct _MadBusWmiControl3_IN
{
    // 
    ULONG InData1;
    #define MadBusWmiControl3_IN_InData1_SIZE sizeof(ULONG)
    #define MadBusWmiControl3_IN_InData1_ID 1

    // 
    ULONG InData2;
    #define MadBusWmiControl3_IN_InData2_SIZE sizeof(ULONG)
    #define MadBusWmiControl3_IN_InData2_ID 2

} MadBusWmiControl3_IN, *PMadBusWmiControl3_IN;

#define MadBusWmiControl3_IN_SIZE (FIELD_OFFSET(MadBusWmiControl3_IN, InData2) + MadBusWmiControl3_IN_InData2_SIZE)

typedef struct _MadBusWmiControl3_OUT
{
    // 
    ULONG OutData1;
    #define MadBusWmiControl3_OUT_OutData1_SIZE sizeof(ULONG)
    #define MadBusWmiControl3_OUT_OutData1_ID 3

    // 
    ULONG OutData2;
    #define MadBusWmiControl3_OUT_OutData2_SIZE sizeof(ULONG)
    #define MadBusWmiControl3_OUT_OutData2_ID 4

} MadBusWmiControl3_OUT, *PMadBusWmiControl3_OUT;

#define MadBusWmiControl3_OUT_SIZE (FIELD_OFFSET(MadBusWmiControl3_OUT, OutData2) + MadBusWmiControl3_OUT_OutData2_SIZE)


typedef struct _MadBusWmiControl
{
    // MadBus Control Property
    ULONG ControlValue;
    #define MadBusWmiControl_ControlValue_SIZE sizeof(ULONG)
    #define MadBusWmiControl_ControlValue_ID 1

} MadBusWmiControl, *PMadBusWmiControl;

#define MadBusWmiControl_SIZE (FIELD_OFFSET(MadBusWmiControl, ControlValue) + MadBusWmiControl_ControlValue_SIZE)

#endif
