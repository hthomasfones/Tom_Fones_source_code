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
#/*  Module NAME : madblkraw.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH sub-script for setting up a raw block device for i/o    
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
#/* $Id: madblkraw.sh, v 1.0 2021/01/01 00:00:00 htf $                            
#/*        
set +x ; printf "\nLets see if we can create a raw block device for i/o\n" ; set -x  
mkdir /dev/raw 
mknod /dev/raw/raw1 c 162 1
modprobe raw
#mknod /dev/raw/rawctl c 162 0       
                                      
raw /dev/raw/raw$dx  /dev/fd$dx
ls -l /dev/raw
sleep $delay
#
set +x ; printf "\nLets see if we can send an ioctl to the raw block device\n" ; set -x  
cd $projectdir  #appbasepath 
$testappbio_path$testappbio $devnum rst
sleep $delay
