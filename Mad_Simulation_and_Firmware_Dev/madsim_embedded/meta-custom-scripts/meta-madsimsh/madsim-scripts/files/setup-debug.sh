#!/bin/sh
#
set -x
clear
pwd
uname -r
echo ""
(set +x;echo "........... Here are the module load addresses for gdb ......... ")
#(set +x;echo " ========== cat /sys/module/madbus/sections/.text")
cat /sys/module/madbus/sections/.text
#(set +x;echo " ========== cat /sys/module/maddevb/sections/.text")
cat /sys/module/maddevb/sections/.text
#(set +x;echo " ========= cat /sys/module/maddevc/sections/.text")
cat /sys/module/maddevc/sections/.text
(set +x; echo "Pause to enable breaking into gdb if necessary")
sleep 3
dmesg -wT > /var/log/maddisk.log &

