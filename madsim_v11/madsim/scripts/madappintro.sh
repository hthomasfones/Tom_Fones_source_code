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
#/*  Module NAME : madappintro.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH sub-script for use by the scripts of the Mad simulation testware      
#/*                This sub-script invokes the test program and the siulation-ui and 
#/*                presents their help text                 
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
#/* $Id: madappintro.sh, v 1.0 2021/01/01 00:00:00 htf $                            
#/*                                                                            

printf "\nLet's see the test app & simulator-ui introduce themselves\n"
cd $appbasepath 
#ls -l
$testapp_path$testapp
#
$simapp_path$simapp
