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
#/*  Module NAME : madresults.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH sub-script for aiding tests of the Mad simulation testware      
#/*                This subscript presents results                 
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
#/* $Id: madresults.sh, v 1.0 2021/01/01 00:00:00 htf $                            
#/*                                                                    
set +x ; printf "\nLet's present the device data... \n" ; set -x
cd $appbasepath
#
rm maddev$devnum.dat
#
$simapp_path$simapp $devnum sav $dumpsize

if [ -e maddev$devnum.dat ]
then #needs a separate line!
     cat maddev$devnum.dat | more
     xxd maddev$devnum.dat | more
else
    set -x ; printf "File not found! maddev$devnum.dat\n" ; set +x
fi
set +x
printf "\n"
###############################################










