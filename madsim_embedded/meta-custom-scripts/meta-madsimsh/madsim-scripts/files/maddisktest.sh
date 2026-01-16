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
printf "\n+++++++++++++++++++++++++++++ MadDiskTest +++++++++++++++++++++++++++++++++++++++++++++++++++ \n"

#Load the simulation & target drivers
source madinsmods.sh
sleep $delay
ls -l /dev | grep "fd"
#
cd $appbasepath"/scripts"
#source madrawblk.sh
#sleep $delay
cd $appbasepath 
#
set -x
(set +x ; printf "\nList all floppy disks\n")
ls -l $partpath*
#delay=1
#
#if [ ! -f "$madblockdevpath$dx" ]; 
#then
#    echo " ==================== Disk "$madblockdevpath$dx" missing!"
#    exit 9
#fi    

last_64K_block=127   ##The last 64KB block in 8MB
(set +x; echo  "===================== dd if=/dev/zero of=$partpath ..." > /dev/kmsg)
dd if=/dev/zero of=$partpath bs=64K count=1
sleep $delay
#dd if=/dev/zero of=$partpath bs=64K seek=$last_64K_block count=1 conv=notrunc
sleep $delay

(set +x ; printf "\nList initial parameters of our disk(s)\n")
(set +x; echo  " ==================== lsblk -o ... $partpath" > /dev/kmsg)
lsblk -o NAME,KNAME,PATH,FSAVAIL,FSSIZE,FSTYPE,FSAVAIL "$partpath"
sleep $delay
#
(set +x; echo  " ==================== fdisk -l $partpath" > /dev/kmsg)
fdisk -l $partpath
sleep $delay

(set  +x ; echo " ")
rm -r "$mountpath""/"
#
(set +x; echo  "mkdir $mountpath")
mkdir $mountpath
if [ -f "$mountpath" ];
then
    set +x ; echo " Mount path "$mountpath" exists!" ; set -x
    #exit 9
fi    

#delay=1
#Make a file system on our block device
(set +x; echo " ===================== dd if=/dev/zero of=devfd1.img bs=1M count=2" > /dev/kmsg)
dd if=/dev/zero of=devfd1.img bs=1M count=2
sleep $delay
(set +x; echo " ===================== mkfs.msdos -F 12 -f 1 --mbr -v -n MADDISK1 devfd1.img" > /dev/kmsg)
mkfs.msdos -F 12 -f 1 --mbr -v -n MADDISK1 devfd1.img
sleep $delay

(set +x; echo  " ==================== dd if=devfd1.img of=$partpath bs=4096 count=8" > /dev/kmsg)
dd if=devfd1.img of=$partpath bs=4096 count=8
sleep $delay
(set +x; echo " ")
#
#Check the new file system on our block device
(set +x; echo  " ==================== dosfsck -t -V -v $partpath" > /dev/kmsg)
dosfsck -t -V -v $partpath
sleep $delay
(set  +x ; echo " ")
#

#Mount the new device to a mount path and verify
(set +x; echo  " ==================== mount -t msdos --source $partpath --target $mountpath" > /dev/kmsg)
mount -t msdos --source $partpath --target $mountpath
sleep $delay

(set +x; echo  " ==================== findmnt | grep $partpath" > /dev/kmsg)
findmnt | grep $partpath
sleep $delay
 
(set +x; printf "\nList UPDATED parameters of our disk(s)\n")
(set +x; echo  " ==================== lsblk ... $partpath" > /dev/kmsg)
lsblk -o NAME,KNAME,PATH,FSAVAIL,FSSIZE,FSTYPE,FSAVAIL "$partpath"
sleep $delay

#Make a new file to show in the directory
datadir=$mountpath"/datafiles"

(set +x; echo  " ==================== mkdir $datadir" > /dev/kmsg)
mkdir $datadir
sleep $delay
cd $datadir
pwd
(set +x; echo  " ==================== cp $scriptpath'grub.cfg'  grub1.cfg" > /dev/kmsg)
cp $scriptpath"grub.cfg"  grub1.cfg
sleep $delay
ls -l
(set +x; echo  " ==================== cat grub1.cfg" > /dev/kmsg)
cat grub1.cfg
sleep $delay
set  +x ; echo " " ; set -x

#Exercise the disk through fio
testfile=$datadir"/fiotest"
(set +x; echo  " ==================== fallocate -l 128K $testfile ....." > /dev/kmsg)
fallocate -l 128K $testfile
sleep $delay
(set +x; echo  " ==================== fio --filename=$testfile ....." > /dev/kmsg)
#valgrind --tool=memcheck --track-origins=yes -s --leak-check=full \ 
    fio --filename=$testfile \
        --size=32kb --rw=randrw --bs=8k --time_based --runtime=20 \
        --iodepth=1 --numjobs=1 --name=madjob1
sleep $delay

#Make another new file to show in the directory
cd $datadir
pwd
(set +x; echo  " ==================== cp $scriptpath'grub.cfg'  grub2.cfg" > /dev/kmsg)
cp $scriptpath"grub.cfg"  grub2.cfg
sleep $delay

(set +x; echo  " ==================== sync $mountpath" > /dev/kmsg)
sync -f $mountpath
sleep $delay

ls -l
(set +x; echo  " ==================== cat grub2.cfg" > /dev/kmsg)
cat grub2.cfg
sleep $delay

#Save the disk to a file
cd ~
savefile="devfd"$dx".sav"
(set +x; echo  " ==================== dd if=$partpath  of=$savefile" > /dev/kmsg)
dd if=$partpath  of=$savefile bs=4096 status=progress
sleep $delay
#
(set +x; echo  " ==================== umount $mountpath" > /dev/kmsg)
umount $mountpath
sleep $delay

(set +x; echo  " ==================== rmmod(s) ..." > /dev/kmsg)
rmmod $madmodule
rmmod $busmodule
pkill -f 'dmesg -wT'
 
lsmod | grep "mad"
sleep $delay

#Dump the device-file in hex
#The MBR and directory should appear at the beginning
#The copy of grub.cfg is near the bottom - after the random fio data 
clear
printf "\nLets look for an MBR, a directory, data file(s) and binary FIO data\n\n"

(set +x; echo  " ==================== hexdump -C -v $savefile ..." > /dev/kmsg)
hexdump -C -v $savefile | more 

set  +x
printf "\n=== maddisktest.sh fini ===================\n"
