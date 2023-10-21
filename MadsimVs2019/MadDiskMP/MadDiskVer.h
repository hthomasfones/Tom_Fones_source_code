/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    Madver.h

Abstract:

    Private header file for Mad_u3.sys modules.  This contains private
    structure and function declarations as well as constant values which do
    not need to be exported.

Environment:

    kernel mode only

Notes:


Revision History:

--*/


/*
*************************************************************************
*                                                                       *
*   Copyright 1994-2008 LSI Corporation. All rights reserved.           *
*                                                                       *
************************************************************************/


#ifndef _MADDISKVER_H_
#define _MADDISKVER_H_

// version numbers for VER_FILEVERSION, keep in sync with version string
#define MAD_VERSION_MAJOR       5
#define MAD_VERSION_MINOR       9
#define MAD_VERSION_BUILD      10
#define MAD_VERSION_REV         5

#define MAD_CMN_VERSION_NUMBER  "5.09.10.05"

#define MAD_VERSION_LABEL       "MadDiskMP_" MAD_CMN_VERSION_NUMBER

#endif //_MADDISKVER_H_

