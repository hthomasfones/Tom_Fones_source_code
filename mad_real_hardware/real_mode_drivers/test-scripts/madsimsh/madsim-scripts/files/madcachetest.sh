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
printf "\n+++++++++++++++++++++++++++++ MadCacheTest +++++++++++++++++++++++++++++++++++++++++++++++++++ \n"
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

### Customized tests: Device Cacheing
cd $appbasepath 
set +x ; printf "\nCache & Programmed i/o test\n" ; set -x ;
$testappbio_path$testappbio $devnum ini
sleep $delay
#
set +x ; printf "\nLet's do some programmed (mmaped) io  =================\n" ; set -x ;
$testappbio_path$testappbio $devnum piw 45 ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789
sleep $delay
$testappbio_path$testappbio $devnum pir 50
sleep $delay
set +x ; printf "\nLet's align the read & write caches and confirm ========\n" ; set -x ;
$testappbio_path$testappbio $devnum awc 8  #Align the write cache
sleep $delay
$testappbio_path$testappbio $devnum arc 16  #Align the read cache
sleep $delay
$testappbio_path$testappbio $devnum mget 
sleep $delay
$testappbio_path$testappbio $devnum get 
sleep $delay
#
set +x ; printf "\nPopulate device sectors through the write cache =================\n" ; set -x ;
$testappbio_path$testappbio $devnum rst  #Align the read, write caches to 0
sleep $delay
$testappbio_path$testappbio $devnum get 
sleep $delay
$testappbio_path$testappbio $devnum pwc :....................:
sleep $delay
$testappbio_path$testappbio $devnum prc 
sleep $delay
$testappbio_path$testappbio $devnum get 
sleep $delay
$testappbio_path$testappbio $devnum pwc SECTOR_ZERO_0000000000_..........
sleep $delay
$testappbio_path$testappbio $devnum pwc SECTOR_ONE_1111111111_..........
sleep $delay
$testappbio_path$testappbio $devnum pwc SECTOR_TWO_222222222_.......... 
sleep $delay
$testappbio_path$testappbio $devnum pwc SECTOR_THREE_3333333333_.......... 
sleep $delay
$testappbio_path$testappbio $devnum pwc SECTOR_FOUR_4444444444_........... 
sleep $delay
$testappbio_path$testappbio $devnum pwc *::::::::::::::::::::*
sleep $delay
$testappbio_path$testappbio $devnum get 
sleep $delay
dumpsize=4100
cd $currdir
source madresults.sh

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
$testappbio_path$testappbio $devnum get 

set +x ; printf "\nAlign (reset) the read cache and retrieve device sectors again=====\n" ; set -x ;
$testappbio_path$testappbio $devnum rst  #Align the read, write caches to 0
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
$testappbio_path$testappbio $devnum prc 

lsmod | grep "mad"
cd $currdir
set +x
dumpsize=4100
source madresults.sh

rmmod $madmodule
rmmod $busmodule
echo "=== madcachetest.sh  fini ==================="

