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
#/*  Module NAME : madrandomiotest.sh                                                  
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
#/* $Id: madrandomiotest.sh, v 1.0 2021/01/01 00:00:00 htf $                            
#/*                                                                            
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
### Customized tests: Random i/o
set +x ; printf "\nRandom i/o tests  =================\n" ; set -x
$simapp_path$simapp $devnum idd b 2048
#
$simapp_path$simapp $devnum get

$testapp_path$testapp $devnum rst

set +x ; printf "First some writes at various locations X pages apart\n" ; set -x
$testapp_path$testapp $devnum wrr 4100 0 Zero_00000000000 
#
$testapp_path$testapp $devnum wrr 4100 4096 One_111111111111
#
$testapp_path$testapp $devnum wrr 4100 8192 Two_222222222222
#
$testapp_path$testapp $devnum wrr 4100 12288 Three_3333333333 
#
$testapp_path$testapp $devnum wrr 4100 16384 Four_44444444444 
#
$testapp_path$testapp $devnum wrr 4100 20480 Five_55555555555 
#
$testapp_path$testapp $devnum wrr 4100 24576 Six_666666666666
#
#$testapp_path$testapp $devnum wrr 16900 28672 Seven_7777777777
#
set +x ; printf "\nNow some reads... \n" ; set -x
$testapp_path$testapp $devnum rdr 4100 0
#
$testapp_path$testapp $devnum rdr 4100 4096
#
$testapp_path$testapp $devnum rdr 4100 8192
#
$testapp_path$testapp $devnum rdr 4100 12288 
#
$testapp_path$testapp $devnum rdr 4100 16384
#
$testapp_path$testapp $devnum rdr 4100 20480
#
$testapp_path$testapp $devnum rdr 4100 24576
#
$testapp_path$testapp $devnum rdr 16900 28672
#
sleep 1

cd $currdir
set +x
dumpsize=30000
source madresults.sh
echo "=== fini ==================="

