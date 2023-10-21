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
#/*  Module NAME : maddirectiotest.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH script for running one test of the Mad simulation testware      
#/*                This script exercises direct-io between user mode kernel mode                 
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
#/* $Id: maddirectiotest.sh, v 1.0 2021/01/01 00:00:00 htf $                            
#/*                                                                            
#Set up script environment variables
source madenv.sh

clear
set -x

#Load the simulation & target drivers
source madinsmods.sh

#Check for help from the test app & sim-ui
cd $currdir
source madappintro.sh

### Customized tests: Direct I/O + DMA
#
printf "\nSanity tests\n"
$testapp_path$testapp $devnum rst
#$testapp_path$testapp $devnum get
$testapp_path$testapp $devnum mget
#$simapp_path$simapp $devnum get
#$simapp_path$simapp $devnum mget
$simapp_path$simapp $devnum idd

printf "\nDirect I/O tests  =================\n"
#printf "First some writes \n"
$testapp_path$testapp $devnum wdi 1 One_111111111111111111111 0 
#sleep $delay

$testapp_path$testapp $devnum rdi 1 xyz 0 
sleep $delay

#$testapp_path$testapp $devnum rdi 1 Zero_000000000000000000000 0 
#sleep $delay

#$testapp_path$testapp $devnum rdi 1 One_111111111111111111111 4096 
#sleep $delay

#$testapp_path$testapp $devnum rdi 1 xyz 4096 
#sleep $delay

#$testapp_path$testapp $devnum wdi 1 Two_2222222222222222222222 0 
#sleep $delay

#$testapp_path$testapp $devnum wdi 1 xyz 0 
#sleep $delay

#$testapp_path$testapp $devnum wdi 1 Three_333333333333333333333 12288 
#sleep $delay

#$testapp_path$testapp $devnum rdi 1 xyz 12288 
#sleep $delay

#printf "\nNow some reads\n"
#$testapp_path$testapp $devnum rdi 1 xyz 4096 
#sleep $delay

#$testapp_path$testapp $devnum rdi 1 xyz 8192 
#sleep $delay

#$testapp_path$testapp $devnum rdi 1 xyz 12288  
#sleep $delay

#$testapp_path$testapp $devnum mget
#
#kill all instances of the no-waited test app
#killall --verbose --wait $testapp
#ps -ef | grep -i $testapp

cd $currdir
set +x
dumpsize=10000
source madresults.sh
echo "=== fini ==================="











