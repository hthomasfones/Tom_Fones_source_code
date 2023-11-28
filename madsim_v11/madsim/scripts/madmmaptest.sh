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
#/*  Module NAME : madmmaptest.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH script for running one test of the Mad simulation testware      
#/*                This script exercises memmap between uswer & kernel mode and
#/*                confirms the results with ioctls                      
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
#/* $Id: madmmaptest.sh, v 1.0 2021/01/01 00:00:00 htf $                           

#Set up script environment variables
bdev=0 #We will setup  char-mode device(s)
source madenv.sh

clear

#Load the simulation & target drivers
source madinsmods.sh

#Check for help from the test app & sim-ui
cd $currdir
#source madappintro.sh

cd $appbasepath 
set +x
echo "\nMemmap tests... Get the device registers\n"
echo "from a simulator ioctl; from a simulator mmap\n"
echo "from a device driver ioctl; from a device driver mmap\n"
set -x

$simapp_path$simapp $devnum get 
sleep $delay
$simapp_path$simapp $devnum mget 
sleep $delay
$testapp_path$testapp $devnum get 
sleep $delay
$testapp_path$testapp $devnum mget 
#

#kill all instances of the no-waited test apps
#killall --verbose --wait $testapp
#ps -ef | grep -i $testapp

echo "=== fini ==================="











