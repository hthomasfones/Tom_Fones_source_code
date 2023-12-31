#!/bin/sh
##/*                                                                            
#/*  PRODUCT      : MAD Device Simulation Framework                            
#/*  COPYRIGHT    : (c) 2021 HTF Consulting                                    
#/*                                                                            
#/* This source code is provided by Dual/GPL license to the Linux open source   
#/* community                                                                  
#/*                                                                             
#/*****************************************************************************
#/*                                                                            
#/*  Module NAME : madenv.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH sub-script for setting up tests of the Mad simulation testware      
#/*                This sub-script creates environment variables                 
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
#/* $Id: madenv.sh, v 1.0 2021/01/01 00:00:00 htf $                            
#/*                                                                          
delay=0.25

busdrvrpath="../madbus/KERN_SRC"
busmodule="madbus"
busdevobj="madbusobj"
busdevname="madbus_0"
#
if [ $bdev -ge 1 ];
then
    echo "Configuring for the block-dev driver stack"
    madmodule="maddevb"
    maddevobj="maddevb_obj"
    madmodule2="maddevc"
    devtype='b'
    pcidev=8193 #x2001
else
    echo "Configuring for the char-dev driver stack"
    madmodule="maddevc"
    maddevobj="maddevc_obj"
    madmodule2="maddevb"
    devtype='c'
    pcidev=4097 #x1001
fi    
sleep $delay
trgtdrvrpath="../$madmodule/KERN_SRC"

# number of devices 
number_bus_slots=1
number_static_devs=$number_bus_slots
devnum=$number_static_devs #last device

# The bus drivers own device-class specific devices will be memory devices
madbus_major=1 

maddev_major=0
mode="664"
#
appbasepath="/home/htfones/eclipse-wkspc/madsim"
scriptpath="../../scripts"
simapp_path="madsimui/Debug/"
simapp="madsimui"
#
simappauto_path="madsimauto/Debug/"
simappauto="madsimauto"
#
testapp_path="madtestc/Debug/"
testapp="madtestc"
#
testappbio_path="madtestb/Debug/"
testappbio="madtestb"
madblockdevpath="/dev/fd"

dumpsize=1000
currdir=$(pwd) #where are we

echo "Let's user-id and timestamp this execution"
echo "Hello... $(whoami). The local time is: $(date)"
sleep $delay
# Group: since distributions do it differently, look for wheel or use staff
if grep -q '^staff:' /etc/group;
then
    group="staff"
else
    group="wheel"
fi

