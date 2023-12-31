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
#/*  Module NAME : madcacheiotest.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH script for running one test of the Mad simulation testware      
#/*                This script exercises device cacheing                 
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
#/* $Id: madcacheiotest.sh, v 1.0 2021/01/01 00:00:00 htf $                            
#/*                                                                     

#Set up script environment variables
bdev=1 #We will setup  block-mode device(s)
source madenv.sh
number_static_devs=1 #override the general environment setting

clear
printf "\nmadcachetest.sh begins \n"
set -x

#Load the simulation & target drivers
source madinsmods.sh
devnum=1
dx=$devnum
#
sleep $delay
cd $appbasepath"/scripts"
#source madrawblk.sh
sleep $delay
#exit
#Check help for the testware
cd $currdir
cd $appbasepath 

### Customized tests: Device Cacheing
#
cd $appbasepath 
set +x ; printf "\nCache & Programmed i/o test\n" ; set -x ;
$testappbio_path$testappbio $devnum ini
#exit
#$testappbio_path$testappbio $devnum get
#$simappbio_path$simappbio $devnum get
#exit
#$simapp_path$simapp $devnum mgt
#$simapp_path$simapp $devnum idd
#
set +x ; printf "\nLet's do some programmed i/o =================\n" ; set -x ;
#$testappbio_path$testappbio $devnum piw 45 ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789
#
#$testappbio_path$testappbio $devnum pir 50
#exit
set +x ; printf "\nPopulate device sectors through the write cache =================\n" ; set -x ;
#$testappbio_path$testappbio $devnum apwc ##Align the write cache
$testappbio_path$testappbio $devnum pwc Sector_zero_000 #Program the write cache
sleep $delay
$testappbio_path$testappbio $simappbio_path$simappbio $devnum get
exit
$testappbio_path$testappbio $devnum pwc Sector_zero_000 #Program the write cache
sleep $delay
$testappbio_path$testappbio $devnum prc 
sleep $delay
exit
#$simapp_path$simapp $devnum fwc #Flush the write cache
#sleep $delay

$testappbio_path$testappbio $devnum pwc Sector_one_111 
sleep $delay
#$simapp_path$simapp $devnum fwc
#sleep $delay

$testappbio_path$testappbio $devnum pwc Sector_two_222 
sleep $delay
#$simapp_path$simapp $devnum fwc
#sleep $delay

$testappbio_path$testappbio $devnum pwc Sector_three_333 
sleep $delay
#$simapp_path$simapp $devnum fwc
#sleep $delay

$testappbio_path$testappbio $devnum pwc Sector_four_444 
sleep $delay
#$simapp_path$simapp $devnum fwc
#sleep $delay
$testappbio_path$testappbio $devnum get

set +x ; printf "\nRetrieve device sectors through the read cache ==================\n" ; set +x ;
$testappbio_path$testappbio $devnum prc 
sleep $delay
#$simapp_path$simapp $devnum lrc
#sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
#$simapp_path$simapp $devnum lrc
#sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
#$simapp_path$simapp $devnum lrc
#sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
$simapp_path$simappc $devnum lrc
sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
#$simapp_path$simapp $devnum lrc
#sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
$simapp_path$simapp $devnum lrc
#sleep $delay
#$testappbio_path$testappbio $devnum get

set +x ; printf "\nAlign (reset) the read cache and retrieve device sectors again=====\n" ; set +x ;
$testappbio_path$testappbio $devnum arc 1 
sleep $delay
#simapp_path$simapp $devnum arc
#sleep $delay
#$testappbio_path$testappbio $devnum get

$testappbio_path$testappbio $devnum prc 
sleep $delay
#$simapp_path$simapp $devnum lrc
#sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
#$simapp_path$simapp $devnum lrc
#sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
#$simapp_path$simapp $devnum lrc
#sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
$simapp_path$simapp $devnum lrc
#sleep $delay
#$testappbio_path$testappbio $devnum get
#
#$simapp_path$simapp 1 hun
sleep 900
rmmod $madmodule
rmmod $busmodule
lsmod | grep "mad"
cd $currdir
set +x
dumpsize=4100
source madresults.sh
echo "=== madcachetest.sh  fini ==================="











