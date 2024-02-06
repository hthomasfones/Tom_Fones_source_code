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
#/*  Module NAME : madalldevicestest.sh                                                  
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
#/* $Id: madalldevicestest.sh, v 1.0 2021/01/01 00:00:00 htf $ 
                          
#Set up script environment variables
bdev=0 #We will setup char-mode device(s)
source madenv.sh
number_bus_slots=3
number_static_devs=$number_bus_slots

#Clear the console; hide executing commands
clear
set -x

#Load the simulation & target drivers
source madinsmods.sh

#Check for help from the test app & sim-ui
cd $scriptpath
#source madappintro.sh
cd $projectdir  

### Customized tests: Buffered i/o
#
#printf "\nConnectivity tests\n"
for (( dn=1; dn<= $number_static_devs; dn++ ))
do
    $testapp_path$testapp $dn rst
    $testapp_path$testapp $dn get
    $testapp_path$testapp $dn mget
    $simapp_path$simapp $dn get
    $simapp_path$simapp $dn mget
    #$simapp_path$simapp $dn idd

    printf "\nBuffered I/O tests  =================\n"
    printf "First some writes \n"
    $testapp_path$testapp $dn wb 33 0 abcdefghijklmnop
    sleep $delay

    $testapp_path$testapp $dn wba 17 0 0123456789ABCDEF
    sleep $delay

    printf "\nNow some reads\n"
    $testapp_path$testapp $dn rb 50
    sleep $delay

    $testapp_path$testapp $dn rba 50
    sleep $delay

    $testapp_path$testapp $dn get
#
    devnum=$dn
    source $scriptpath"madresults.sh"
done
echo "=== madalldevicestest.sh fini ==================="











