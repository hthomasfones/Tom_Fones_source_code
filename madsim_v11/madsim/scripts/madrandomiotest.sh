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
bdev=1 #We will setup block-mode device(s)
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

$testappbio_path$testappbio $devnum rst

set +x ; printf "First some writes at various locations X pages apart\n" ; set -x
$testappbio_path$testappbio $devnum wrr 4100 0 Zero_00000000000 
#
$testappbio_path$testappbio $devnum wrr 4100 4096 One_111111111111
#
$testappbio_path$testappbio $devnum wrr 4100 8192 Two_222222222222
#
$testappbio_path$testappbio $devnum wrr 4100 12288 Three_3333333333 
#
$testappbio_path$testappbio $devnum wrr 4100 16384 Four_44444444444 
#
$testappbio_path$testappbio $devnum wrr 4100 20480 Five_55555555555 
#
$testappbio_path$testappbio $devnum wrr 4100 24576 Six_666666666666
#
#$testappbio_path$testappbio $devnum wrr 16900 28672 Seven_7777777777
#
set +x ; printf "\nNow some reads... \n" ; set -x
$testappbio_path$testappbio $devnum rdr 4100 0
#
$testappbio_path$testappbio $devnum rdr 4100 4096
#
$testappbio_path$testappbio $devnum rdr 4100 8192
#
$testappbio_path$testappbio $devnum rdr 4100 12288 
#
$testappbio_path$testappbio $devnum rdr 4100 16384
#
$testappbio_path$testappbio $devnum rdr 4100 20480
#
$testappbio_path$testappbio $devnum rdr 4100 24576
#
$testappbio_path$testappbio $devnum rdr 16900 28672
#
sleep 1

cd $currdir
set +x
dumpsize=30000
source madresults.sh
echo "=== fini ==================="

