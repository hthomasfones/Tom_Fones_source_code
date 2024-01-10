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
source madrawblk.sh
sleep $delay
#exit
#Check help for the testware
#cd $currdir
#cd $appbasepath 

### Customized tests: Device Cacheing
#
cd $appbasepath 
set +x ; printf "\nCache & Programmed i/o test\n" ; set -x ;
$testappbio_path$testappbio $devnum ini
sleep $delay
#

set +x ; printf "\nLet's do some programmed (mmaped) io  =================\n" ; set -x ;
$testappbio_path$testappbio $devnum piw 45 ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789
sleep $delay
$testappbio_path$testappbio $devnum pir 50
#exit

set +x ; printf "\nLet's align the read & write caches and confirm ========\n" ; set -x ;
$testappbio_path$testappbio $devnum awc 8  #Align the write cache
sleep $delay
$testappbio_path$testappbio $devnum arc 16  #Align the read cache
sleep $delay
$testappbio_path$testappbio $devnum mget 
#sleep $delay
#exit
set +x ; printf "\nPopulate device sectors through the write cache =================\n" ; set -x ;
$testappbio_path$testappbio $devnum rst  #Align the read, write caches to 0
$testappbio_path$testappbio $devnum pwc Sector_zero_000 #Program the write cache
sleep $delay
$testappbio_path$testappbio $devnum pwc Sector_one_111 
sleep $delay
$testappbio_path$testappbio $devnum pwc Sector_two_222 
sleep $delay

$testappbio_path$testappbio $devnum pwc Sector_three_333 
sleep $delay

$testappbio_path$testappbio $devnum pwc Sector_four_444 
sleep $delay
set +x ; printf "\nRetrieve device sectors through the read cache ==================\n" ; set -x ;
$testappbio_path$testappbio $devnum prc 
sleep $delay
$testappbio_path$testappbio $devnum prc 
sleep $delay
$testappbio_path$testappbio $devnum prc 
sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
$testappbio_path$testappbio $devnum prc 
sleep $delay
#exit

set +x ; printf "\nAlign (reset) the read cache and retrieve device sectors again=====\n" ; set -x ;
$testappbio_path$testappbio $devnum rst  #Align the read, write caches to 0
sleep $delay
$testappbio_path$testappbio $devnum prc 
sleep $delay
#exit
$testappbio_path$testappbio $devnum prc 
sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay

$testappbio_path$testappbio $devnum prc 
sleep $delay
$testappbio_path$testappbio $devnum prc 
#
$simapp_path$simapp 1 hun
sleep 1
#rmmod $madmodule
#rmmod $busmodule
#lsmod | grep "mad"
cd $currdir
set +x
dumpsize=4100
source madresults.sh
echo "=== madcachetest.sh  fini ==================="

