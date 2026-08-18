#!/bin/sh
#
dmesg -C
clear
echo "Madsim simulation framework setup"
echo ""
echo "uname -r"
uname -r
echo ""
echo "insmod madsim kernel drivers"
#source madsetup.sh
source maddriverstack.sh
echo ""
#echo "lsmod"
#lsmod
echo ""
echo "dmesg"
dmesg
echo ""
echo "/usr/bin/madsimui"
/usr/bin/madsimui
echo ""
echo "lsmod"
lsmod
source setup-debug.sg
echo ""
if [ $bdev -ge 1 ];
then
    ls -l /dev/fd*
fi

sleep 5
rmmod $madmodule
sleep 1
rmmod $busmodule
