#!/bin/sh
#/*                                                                            
#/*  PRODUCT      : MAD Device Simulation Framework                            
#/*  COPYRIGHT    : (c) 2021 HTF Consulting                                    
#/*                                                                            
#/* This source code is provided by Dual/GPL license to the Linux open source   
#/* community                                                                  
#/*                                                                             
#/*****************************************************************************
#/*                                                                            
#/*  Module NAME : madinsmods.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH sub-script for aiding tests of the Mad simulation testware      
#/*                This sub-script sets up the environment and loads the drivers                 
#/*                                                                        
#/*  MODULE_AUTHOR("HTF Consulting");                                          
#/*  MODULE_LICENSE("Dual/GPL");                                               
#/*                                                                            
#/* The source code in this file can be freely used, adapted, and redistributed
#/* in source or binary form, so long as an acknowledgment appears in derived  
#/* source files.  The citation should state that the source code comes from a 
#/* set of source files developed by HTF Consulting http://www.htfconsulting.com  
#/*                                               
#/* No warranty is attached.                                                   
#/* HTF Consulting assumes no responsibility for errors or fitness of use      
#/*                                                                            
#/* $Id: madinsmods.sh, v 1.0 2021/01/01 00:00:00 htf $                            
#/*                                                                            

echo "Remove the (existing?) driver stack.."
set -x
rmmod $madmodule.ko
rmmod $madmodule2.ko
rmmod $busmodule.ko
lsmod | grep "mad"
dmesg -C
set +x

## Assign device-class specific major number
#  1 char	Memory devices
#1 = /dev/mem		Physical memory access
#2 = /dev/kmem		Kernel virtual memory access
##
#   1 block	RAM disk
#		  0 = /dev/ram0		First RAM disk
#		  1 = /dev/ram1		Second RAM disk
#		    ...
#		250 = /dev/initrd	Initial RAM disk
#
# invoke insmod with all arguments we got
# and use a pathname, as insmod doesn't look in . by default

# Remove stale device nodes 
set +x ; printf "\nRemove stale device nodes from /dev/... \n" ; set -x
rm -fv /dev/${maddevobj}[0-$number_bus_slots]

# Remove stale bus nodes and replace them, then give gid and perms
printf "\nRemove stale bus-device nodes from /dev...\n "
set -x
rm -fv /dev/${busdevobj}[0-$number_bus_slots]

### Create device nodes to be accessed by the simulator-ui
### Not a normal thing to be done for a bus driver
### We can pre-create bus-device nodes before loading the bus driver
### because we are assigning a static dev_t major number
### (u)nbuffered = (c)haracter ++++++++++++++
###include a device 0 to issue plug-n-play ioctls to
#
set +x ; printf "\nCreate new bus-device nodes for opening... \n" ; set -x
for (( dn=0; dn<=$number_bus_slots; dn++ ))
do
    mknod /dev/${busdevobj}$dn u $madbus_major $dn
done

set +x ; printf "\nLoad the bus & device(s) simulation driver... madbus.ko\n" ; set -x
cd $busdrvrpath
/sbin/insmod ./$busmodule.ko madbus_major=$madbus_major madbus_nbr_slots=$number_bus_slots $* || exit 1

set +x ; printf "\nCreate symbolic links between... \n" ; set -x
ln -sfv ${busdevobj} /dev/${busdevobj}
#
set +x ; printf "\nChange permission modes for all nodes to $mode \n" ; set -x
#chgrp $group /dev/${busdevobj}[0-$number_devs] 
chmod -v $mode  /dev/${busdevobj}[0-$number_bus_slots]

sleep 1
### load the device driver
#
set +x ;  printf "\nLoad the target device driver... $madmodule.ko\n" ; set -x
cd ..
cd $trgtdrvrpath

### invoke insmod including driver-pathname with all the input arguments 
/sbin/insmod ./$madmodule.ko maddev_max_devs=$number_bus_slots  \
                             maddev_nbr_devs=$number_static_devs $* || exit 1

if [ $maddev_major -eq 0 ]
then
    set +x ; printf "\nDetermine dynamic major number allocated by the device driver\n" ; set -x
    maddev_major=`cat /proc/devices | awk "\\$2==\"$maddevobj\" {print \\$1}"`
fi

set +x ; printf "\nCreate new device nodes for opening... \n" ; set -x
for (( dn=1; dn<=$number_static_devs; dn++ ))
do
    mknod /dev/${maddevobj}$dn $devtype $maddev_major $dn
done

set +x ; printf "\nCreate symbolic links between... \n" ; set -x
ln -sfv ${maddevobj} /dev/${maddevobj}

set +x ; printf "\nChange permission modes for all nodes to $mode \n" ; set -x
#chgrp $group /dev/${maddevobj}[0-$number_static_devs] 
chmod -v $mode  /dev/${maddevobj}[0-$number_static_devs]

set +x
printf "\nSome integrity checks... \n"
echo "Loaded drivers & dependencies... "
set -x
lsmod | grep mad
#
lsmaddev='lsmod | grep maddev'
#
if [ -z "$lsmaddev" ];
then
    set +x ; echo "maddev(X) driver missing" ; set -x
    exit 9
fi    
#
set +x ; printf "\nList all $busmodule child-device-simulating threads (mbdthreadX)\n" ; set -x
ps -ef | grep "mbdthread"
#
set +x ; printf "\nPresenting the sysfs tree for the bus driver... ($busmodule)\n" ; set -x
tree /sys/bus/$busmodule/
#
set +x ; printf "\nPresenting the sysfs tree for the root bus device... ($busdevname)\n" ; set -x
tree /sys/devices/$busdevname/
set +x ; printf "\n" ; set -x
#
set +x ; printf "\nPresenting the set of target devices... ("$maddevobj"X)\n" ; set -x
ls -l /dev/ | grep "mad" #$maddevobj
set +x ; printf "\n" ; set -x

