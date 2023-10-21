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
#/*  Module NAME : madsetup.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH script for running setting the MAD environment for unit tests      
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
#/* $Id: madsetup.sh, v 1.0 2021/01/01 00:00:00 htf $                            
#/*                                                                   
#/*    
#Set up script environment variables
source madenv.sh

clear
set -x

#Load the simulation & target drivers
#number_static_devs=2
source madinsmods.sh

#Check help for the testware
cd $currdir
cd $appbasepath 
#source madappintro.sh

### Ready for a unit test
#
set +x
echo "=== fini ==================="











