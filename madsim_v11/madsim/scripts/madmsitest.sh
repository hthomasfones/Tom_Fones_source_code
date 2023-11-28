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
#/*  Module NAME : madmsitest.sh                                                  
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
#/* $Id: madmsitest.sh, v 1.0 2021/01/01 00:00:00 htf $ 
                          
#Set up script environment variables
bdev=0 #We will setup  char-mode device(s)
source madenv.sh

#Clear the console; hide executing commands
clear
set -x

#Load the simulation & target drivers
number_static_devs=0
source madinsmods.sh

#Check for help from the test app & sim-ui
cd $currdir
cd $appbasepath 

#source madappintro.sh

#printf "\nMSI tests\n"
#$simapp_path$simapp 1 hun 
#$simapp_path$simapp 2 hun
#
$simapp_path$simapp 1 hpl 1002 
#$simapp_path$simapp 1 hpl 1002
# 
#tree /sys/devices/$busdevname/
#
$testapp_path$testapp 1 wb 16 0 ABCDEFGHIJLLMNOP
sleep $delay
$testapp_path$testapp 1 wb 16 0 abcdefghijklmnop
sleep $delay
$testapp_path$testapp 1 wb 16 0 0123456789ABCDEF
sleep $delay
$testapp_path$testapp 1 wb 16 0 fedcbi9876543210
sleep $delay
$testapp_path$testapp 1 wb 16 0 ABCDEFGHIJLLMNOP
sleep $delay
#
$testapp_path$testapp 1 rb 16 0
sleep $delay
$testapp_path$testapp 1 rb 16 0
sleep $delay
$testapp_path$testapp 1 rb 16 0
sleep $delay
$testapp_path$testapp 1 rb 16 0
sleep $delay
$testapp_path$testapp 1 rb 16 0
#
cd $currdir
devnum=1
source madresults.sh
sleep 20
$simapp_path$simapp 1 hun 


#kill all instances of the no-waited test apps
killall --verbose --wait $testapp
ps -ef | grep -i $testapp

rmmod $madmodule
rmmod $busmodule
lsmod | grep "mad"

echo "=== madmsitest.sh fini ==================="











