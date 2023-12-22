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
#/*  Module NAME : madhotplugtest.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH script for one test of the Mad simulation testware  
#/*                This script exercises multiple devices           
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
#/* $Id: madhotplugtest.sh, v 1.0 2021/01/01 00:00:00 htf $ 
                          
#Set up script environment variables
bdev=0
source madenv.sh
number_bus_slots=3
number_static_devs=0

#Clear the console; hide executing commands
clear
set -x

tree /sys/devices/$busdevname/

#Load the simulation & target drivers
source madinsmods.sh

#Check for help from the test app & sim-ui
cd $currdir
cd $appbasepath 

#source madappintro.sh

#printf "\nHotplug tests\n"
#for (( dn=1; dn<=$number_bus_slots; dn++ ))
#do
    $simapp_path$simapp 1 hun 
    sleep $delay
#done
#tree /sys/devices/$busdevname/

for (( dn=1; dn<=$number_bus_slots; dn++ ))
do
    $simapp_path$simapp $dn hpl 1001
done

set +x ; printf "\nThe sysfs tree for the bus:bus-device $busmodule : $busdevname should be populated\n" ; set -x
sleep $delay
tree /sys/bus/$busmodule/
tree /sys/devices/$busdevname/
sleep $delay
#
$simapp_path$simapp 1 hun 
$simapp_path$simapp 2 hun 
$simapp_path$simapp 3 hun 
$simapp_path$simapp 1 hun 

set +x ; printf "\nThe sysfs tree for the bus:bus-device $busmodule : $busdevname should be empty\n" ; set -x
sleep $delay
tree /sys/bus/$busmodule/
tree /sys/devices/$busdevname/
sleep $delay

rmmod $madmodule
rmmod $busmodule
lsmod | grep "mad"
pause
echo "=== madhotplugtest.sh fini ==================="

