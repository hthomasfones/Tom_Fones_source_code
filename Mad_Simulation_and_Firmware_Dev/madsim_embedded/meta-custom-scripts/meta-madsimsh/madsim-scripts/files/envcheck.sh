#!/bin/sh
mount | column -t
 
 lsblk -o NAME,SIZE,ROTA,FSTYPE,MOUNTPOINT

echo "== Kernel ERR+ =="; dmesg -T -l err,crit,alert,emerg | tail -200
echo "== journalctl =="; journalctl -p 3 -b --no-pager
echo "== Failed units =="; systemctl --failed 2>/dev/null || true
echo "== Block devices =="; lsblk -o NAME,SIZE,ROTA,FSTYPE,MOUNTPOINT

### Step over a "perl-shebang" problem
#  Normalize line endings on the filter (prevents shebang parse glitches)
sed -i 's/\r$//' /usr/kernel-selftest/kselftest/prefix.pl
sed -i '1s/^\xEF\xBB\xBF//' /usr/kernel-selftest/kselftest/prefix.pl
#
mv /usr/kernel-selftest/kselftest/prefix.pl /usr/kernel-selftest/kselftest/prefix.pl.perl
printf '%s\n' '#!/bin/sh' 'exec /usr/bin/perl /usr/kernel-selftest/kselftest/prefix.pl.perl "$@"' \
  >/usr/kernel-selftest/kselftest/prefix.pl
chmod +x /usr/kernel-selftest/kselftest/prefix.pl
mkdir -p /lib/firmware

# Many tests expect debugfs
mount -t debugfs nodev /sys/kernel/debug 2>/dev/null || true
#
/usr/kernel-selftest/run_kselftest.sh -c "rtc"


# selftests: firmware: fw_run_tests.sh
# modprobe: FATAL: Module test_firmware not found in directory /lib/modules/5.15.124-yocto-standard
# You must have the following enabled in your kernel:
# CONFIG_TEST_FIRMWARE=y
# CONFIG_FW_LOADER=y
# CONFIG_FW_LOADER_USER_HELPER=y
# CONFIG_IKCONFIG=y
# CONFIG_IKCONFIG_PROC=y
#ok 1 selftests: firmware: fw_run_tests.sh # SKIP

