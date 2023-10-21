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
#/*  Module NAME : maddriverstack.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH script for one test of the Mad simulation testware  
#/*                This script just insmod(s) the bus-simulation driver and one device driver 
#/*                with N staic devices -- see madenv.sh           
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
#/* $Id: maddriverstack.sh, v 1.0 2021/01/01 00:00:00 htf $ 
                          
#Set up script environment variables
#
bdev=0  #which device driver; 0=char-mode, 1=block-mode
source madenv.sh

clear
set -x

#Load the simulation driver & target device driver
source madinsmods.sh
sleep 1
lsmod | grep "mad"
sleep 1
cat /proc/meminfo | grep -i "CMa"
#cat /proc/meminfo | grep -i "Free:"
##exit
sleep 1
rmmod $madmodule
rmmod $busmodule
lsmod | grep "mad"
sleep 1
cat /proc/meminfo | grep -i "Cma"
#cat /proc/meminfo | grep -i "Free:"
set +x
echo "=== maddriverstacktest.sh fini ==================="











