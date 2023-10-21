/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_MadDiskMP,
    0xb94a78cd,0x5385,0x413b,0xa0,0xcb,0x43,0x7e,0x0e,0x5c,0x73,0x57);
// {b94a78cd-5385-413b-a0cb-437e0e5c7357}
