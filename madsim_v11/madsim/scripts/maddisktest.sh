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
#/*  Module NAME : maddisktest.sh                                                  
##/*                                                                            
#/*  DESCRIPTION : A BASH script for one test of the Mad simulation testware  
#/*                This script insmod(s) the bus-simulation driver and the 
#/*                block dev driver and exercises one or more disks 
#/*                           
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
#/* $Id: maddisktest.sh, v 1.0 2021/01/01 00:00:00 htf $ 
                          
#Set up script environment variables
bdev=1  #we will load and exercise the block-mode device driver
pcidev=8193 # x2001 block device; legacy int (non-msi)
source madenv.sh
pcidev=8193 # x2001 block device; legacy int (non-msi)
#
dx=$devnum
mountpath="/mnt/maddisk$dx"
partpath=$madblockdevpath$dx 

clear

#Load the simulation & target drivers
source madinsmods.sh
sleep $delay
ls -l /dev | grep "fd"
#
cd $scriptspath
#source madrawblk.sh
sleep $delay
#
cd $projectdir  #appbasepath 
#
set +x ; printf "\nList all floppy disks\n" ; set -x
ls -l $madblockdevpath*
#
#if [ ! -f "$madblockdevpath$dx" ]; 
#then
#    echo "Disk "$madblockdevpath$dx" missing!"
#    exit 9
#fi    

set +x ; printf "\nList initial parameters of our disk(s)\n" ; set -x
lsblk -o NAME,KNAME,PATH,FSAVAIL,FSSIZE,FSTYPE,FSAVAIL "$madblockdevpath$dx"
sleep $delay
#
fdisk -l $madblockdevpath$dx
sleep $delay
#if [ ! -f "$madblockdevpath$dx""p1" ];
#then
#    echo "Disk "$madblockdevpath$dx"p1 missing!"
#    exit 9
#fi    
#
#sleep $delay
set  +x ; echo "" ; set -x
rm -r "$mountpath""/"
#
mkdir $mountpath
if [ ! -f "$mountpath" ];
then
    set +x ; echo "Mount path "$mountpath" missing!" ; set -x
    #exit 9
fi    

#
sleep $delay
#Make a file system on our block device
mkfs.msdos -I -F 12 -R 1 -s 16 -S 512 -i "DEADFACE" -n maddisk$dx -v $partpath 
#
sleep $delay
set  +x ; echo "" ; set -x
#
#Check the new file system on our block device
dosfsck -v -a -w $partpath
set  +x ; echo "" ; set -x
#
sleep $delay
#Mount the new device to a mount path and verify
mount -v -t msdos $partpath $mountpath
findmnt $partpath
sleep $delay
set  +x 
echo "" 
#
printf "\nList UPDATED parameters of our disk(s)\n"
set -x
lsblk -o NAME,KNAME,PATH,FSAVAIL,FSSIZE,FSTYPE,FSAVAIL "$partpath"
#
#Make a new file to show in the directory
cd $mountpath
pwd
cp /boot/grub/grub.cfg  grub1.cfg
sleep $delay
ls -l
#sleep $delay
#cat grub1.cfg
set  +x ; echo "" ; set -x

#Exercise the disk through fio
fio --filename=$mountpath"/fiotest" --size=100kb --rw=randrw --bs=32k --time_based --runtime=10 --numjobs=1 --name=madjob1
sleep $delay
#
#Make another new file to show in the directory
cd $mountpath
pwd
cp /boot/grub/grub.cfg  grub2.cfg
sleep $delay
sync
sleep $delay
#
ls -l
sleep $delay
#cat grub2.cfg
set  +x ; echo "" ; set -x
#
cd ~
#
sleep $delay
#Save the disk to a file
dd if=$partpath  of="devfd"$dx
#
sleep $delay
umount $mountpath
umount $madblockdevpath$dx
sleep $delay
#rmmod $madmodule
#rmmod $busmodule
#lsmod | grep "mad"

#Dump the device-file in hex
#The MBR and directory should appear at the beginning
#The copy of grub.cfg is near the bottom - after the random fio data 
printf "\nLets look for an MBR, a directory, data file(s) and binary FIO data\n\n"

xxd "devfd"$dx | more 

rmmod $madmodule
rmmod $busmodule
lsmod | grep "mad"

set  +x
printf "\n=== maddisktest.sh fini ===================\n" 











