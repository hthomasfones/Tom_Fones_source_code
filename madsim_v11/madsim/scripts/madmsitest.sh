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
source madenv.sh

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
#$simapp_path$simapp_exec 1 hun 
#$simapp_path$simapp 2 hun
#
$simapp_path$simapp 1 hpl 1001 
$simapp_path$simapp 2 hpl 1002
# 
tree /sys/devices/$busdevname/
#
$testapp_path$testapp 1 wb 16 abcdefghijklmnop &
sleep $delay
$simapp_path$simapp 1 cbw
sleep $delay
#
$testapp_path$testapp 1 rb 16 &
sleep $delay
$simapp_path$simapp 1 cbr
sleep $delay

#$simapp_path$simapp 2 hpl 1002 
#
$testapp_path$testapp 2 wb 16 abcdefghijklmnop &
sleep $delay
$simapp_path$simapp 2 cbw
sleep $delay
#
$testapp_path$testapp 2 rb 16 &
sleep $delay
$simapp_path$simapp 2 cbr
sleep $delay

$simapp_path$simapp 1 hun 
$simapp_path$simapp 2 hun  

$simapp_path$simapp 1 hpl 1003 
$simapp_path$simapp 2 hpl 1004

#kill all instances of the no-waited test apps
killall --verbose --wait $testapp
ps -ef | grep -i $testapp

rmmod $madmodule
rmmod $busmodule
lsmod | grep "mad"

echo "=== madmsitest.sh fini ==================="











