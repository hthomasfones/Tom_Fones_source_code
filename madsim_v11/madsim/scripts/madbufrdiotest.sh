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
#/*  Module NAME : madbufrdiotest.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH script for running one test of the Mad simulation testware      
#/*                This script exercises all types of buffered io                 
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
#/* $Id: madbufrdiotest.sh, v 1.0 2021/01/01 00:00:00 htf $                           

#Set up script environment variables
bdev=0 #We will setup  char-mode device(s)
source madenv.sh
clear

#Load the simulation & target drivers
source madinsmods.sh

#Check for help from the test app & sim-ui
cd $currdir
#source "$scriptpath/madappintro.sh"

cd projectdir #appbasepath 
#set +x
#echo "\nSanity tests... Get the device registers\n"
#echo "from a simulator ioctl; from a simulator mmap\n"
#echo "from a device driver ioctl; from a device driver mmap\n"
#set -x

#$simapp_path$simapp $devnum get 
#sleep $delay
#$simapp_path$simapp $devnum mget 
#sleep $delay
#$testapp_path$testapp $devnum get 
#sleep $delay
#$testapp_path$testapp $devnum mget 
#
#$testapp_path$testapp $devnum rst 

### Customized tests: Buffered i/o
$simapp_path$simapp $devnum idd b

set +x
printf "\nBuffered I/O tests  =================\n"
printf "First some writes \n"
set -x
$testapp_path$testapp $devnum wb 17 0 0123456789ABCDEF 
sleep $delay

$testapp_path$testapp $devnum wba 16 0 ABCDEFGHIJKLMNOP
sleep $delay

$testapp_path$testapp $devnum wba 50 0 fedcba9876543210 
sleep $delay

$testapp_path$testapp $devnum wba 33 0 zZzZzZzZzZzZzZzZz 
sleep $delay

$testapp_path$testapp $devnum wbq 33 0 AaAaAaAaAaAaAaAaAaAaAaAaAa 
sleep $delay

set +x ; printf "\nNow some reads\n" ; set -x
$testapp_path$testapp $devnum rb 50 0
sleep $delay

$testapp_path$testapp $devnum rba 50 0
sleep $delay

$testapp_path$testapp $devnum rbq 50 0
sleep $delay

$testapp_path$testapp $devnum rb 50 0
sleep $delay

#kill all instances of the no-waited test apps
#killall --verbose --wait $testapp
#ps -ef | grep -i $testapp

cd $projectdir
source $scriptpath"/madresults.sh"

echo "=== fini ==================="











