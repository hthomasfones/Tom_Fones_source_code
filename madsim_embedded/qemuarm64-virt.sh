#!/bin/sh
#
echo on
home="/home/htfones/"
imagedir="$home/GIT/yocto/poky/build-arm64/tmp/deploy/images/qemuarm64/"
krnlver="5.15"

sudo /usr/bin/qemu-system-aarch64 -M virt -cpu cortex-a72 -m 8192 -smp 4 \
                        -kernel $imagedir/Image                          \
                        -nographic                                       \
                        -drive file=$imagedir/core-image-full-cmdline-qemuarm64.ext4,if=none,id=hd0,format=raw \
                        -device virtio-blk-device,drive=hd0              \
                        -gdb tcp::1234 -S                                \
                        -append "console=ttyAMA0,115200 loglevel=7 nokaslr    \
                                 kasan.quarantine=off kasan_multi_shot=1      \
                                 root=/dev/vda rw rootfstype=ext4 rootdelay=2"   

#/core-image-full-cmdline-qemuarm64-20250813184252.rootfs.ext4
