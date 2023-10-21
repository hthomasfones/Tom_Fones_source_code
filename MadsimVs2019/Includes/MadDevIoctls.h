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
/*  Exe file ID  : MadDevice.sys, MadTestApp.exe                               */
/*                                                                             */
/*  Module  NAME : MadDevIoctls.h                                              */
/*                                                                             */
/*  DESCRIPTION  : Definition of Ioctls provided by the MadDevice.sys          */
/*                 Derived from WDK-Toaster\func                                */
/*                                                                             */
/*******************************************************************************/

// Define the IOCTL codes we will use.  The IOCTL code contains a command
// identifier, plus other information about the device, the type of access
// with which the file must have been opened, and the type of buffering.
//

// Device type           -- in the "User Defined" range."
#define MADDEV_TYPE 43000
#define MADDEV_IOCTL_ENUM_BASE 2833  
//0xb11

enum {MADDEV_ENUM_INITIALIZE  = MADDEV_IOCTL_ENUM_BASE,
	  MADDEV_ENUM_MAP_VIEWS,
      MADDEV_ENUM_UNMAP_VIEWS,
	  MADDEV_ENUM_RESET_INDECES,
      MADDEV_ENUM_GET_INTEN_REG,
      MADDEV_ENUM_SET_INTEN_REG,
      MADDEV_ENUM_GET_MAD_CONTROL_REG,
      MADDEV_ENUM_SET_MAD_CONTROL_REG,
      //MADDEV_ENUM_READ,
      //MADDEV_ENUM_WRITE,
      MADDEV_ENUM_CACHED_READ,
      MADDEV_ENUM_CACHED_WRITE,
      MADDEV_ENUM_ALIGN_READ,
      MADDEV_ENUM_ALIGN_WRITE,
	  MADDEV_ENUM_DONT_DISPLAY,

//* The following help implement I/O as IRP_MJ_XXX functions
//*
      /*MADDEV_MJ_READ,
      MADDEV_MJ_WRITE,*/
      MADDEV_MJ_INVALID_IO};

// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
//
#define MADDEV_IOCTL_INITIALIZE  \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_INITIALIZE, METHOD_NEITHER,  FILE_ANY_ACCESS)

#define MADDEV_IOCTL_MAP_VIEWS    \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_MAP_VIEWS, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

#define MADDEV_IOCTL_UNMAP_VIEWS    \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_UNMAP_VIEWS, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

#define MADDEV_IOCTL_RESET_INDECES    \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_RESET_INDECES, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

#define MADDEV_IOCTL_GET_INTEN_REG      \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_GET_INTEN_REG, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

 #define MADDEV_IOCTL_SET_INTEN_REG      \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_SET_INTEN_REG, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

#define MADDEV_IOCTL_GET_MAD_CONTROL_REG      \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_GET_MAD_CONTROL_REG, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

#define MADDEV_IOCTL_SET_MAD_CONTROL_REG      \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_SET_MAD_CONTROL_REG, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

//#define MADDEV_IOCTL_READ      \
//        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_READ, METHOD_BUFFERED,  \
//        (FILE_WRITE_DATA | FILE_READ_DATA))

//#define MADDEV_IOCTL_WRITE    \
//        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_WRITE, METHOD_BUFFERED,  \
//        (FILE_WRITE_DATA | FILE_READ_DATA))

#define MADDEV_IOCTL_CACHE_READ \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_CACHED_READ, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

#define MADDEV_IOCTL_CACHE_WRITE \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_CACHED_WRITE, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

#define MADDEV_IOCTL_ALIGN_READ \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_ALIGN_READ, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

#define MADDEV_IOCTL_ALIGN_WRITE \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_ALIGN_WRITE, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

#define  IOCTL_MADDEVICE_DONT_DISPLAY_IN_UI_DEVICE \
        CTL_CODE(MADDEV_TYPE, MADDEV_ENUM_DONT_DISPLAY, METHOD_BUFFERED,  \
        (FILE_WRITE_DATA | FILE_READ_DATA))

typedef struct _MADDEV_MAP_VIEWS
    {
	PHYSICAL_ADDRESS liDeviceRegs;
    PVOID            pDeviceRegs;
    PVOID            pPioRead;
    PVOID            pPioWrite;
    } MADDEV_MAP_VIEWS, *PMADDEV_MAP_VIEWS;

typedef struct _MADDEV_IOCTL_STRUCT
       {
       GUID SecurityKey;
       char DataBufr[MAD_SECTOR_SIZE];
       } MADDEV_IOCTL_STRUCT, *PMADDEV_IOCTL_STRUCT;
