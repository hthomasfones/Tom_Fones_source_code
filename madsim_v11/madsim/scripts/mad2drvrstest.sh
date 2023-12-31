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
#/*  Module NAME : mad2drvrstest.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH script for one test of the Mad simulation testware  
#/*                This script exercises hotplug with multiple device drivers           
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
#/*                                                                            
#/* $Id: mad2drvrstest.sh, v 1.0 2021/01/01 00:00:00 htf $ 
                          
#Set up script environment variables
bdev=0 #Char-mode device 1st - then a block-mode on the 2nd driver
source madenv.sh
number_bus_slots=2
number_static_devs=$number_bus_slots

#Clear the console; hide executing commands
clear
set -x

#Load the simulation & target drivers
#
number_static_devs=0
maddev_major=401
#
mknod /dev/${maddevobj}$dn $devtype $maddev_major 1
mknod /dev/${maddevobj}$dn $devtype $maddev_major 2
#
source madinsmods.sh

ln -sfv ${maddevobj} /dev/${maddevobj}

cd ../../
cd $madmodule2/KERN_SRC
/sbin/insmod ./$madmodule2.ko maddev_major=$maddev_major maddev_max_devs=$number_bus_slots maddev_nbr_devs=0 || exit 1
lsmod | grep "mad"

printf "\nChange permission modes for all nodes... \n"
#chgrp $group /dev/${maddevobj}[0-$number_static_devs] 
chmod -v $mode  /dev/${maddevobj}[0-$number_bus_slots]

#Check for help from the test app & sim-ui
cd $currdir
cd $appbasepath 

printf "\n Multiple driver/device-type tests\n"
$simapp_path$simapp 1 hpl 1001 
sleep $delay
$simapp_path$simapp 2 hpl 2001
tree /sys/devices/$busdevname/
sleep $delay
#
$simapp_path$simapp 1 hun  
sleep $delay
$simapp_path$simapp 2 hun 
sleep $delay
#exit
tree /sys/devices/$busdevname/
sleep $delay
#
#$simapp_path$simapp 1 hpl 1002 
#sleep $delay
#$simapp_path$simapp 2 hpl 2002
#sleep $delay
#tree /sys/devices/$busdevname/
#
#$simapp_path$simapp 1 hun  
#$simapp_path$simapp 2 hun 
#tree /sys/devices/$busdevname/
#sleep $delay
#
#exit
rmmod ${madmodule2} 
rmmod $madmodule
rmmod $busmodule
lsmod | grep "mad"
echo "=== mad2drvrstest.sh fini ==================="











