# Modifying the kernel build

COMPATIBLE_MACHINE_arm64 = "qemuarm64"
##COMPATIBLE_MACHINE:qemuarm64 = "qemuarm64"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://0001-export-dma-contiguous.patch  \
                  file://0001-correct-wb-null-pointer.patch "
###                    file://0001-correct-copy-overflow-check.patch \
###                  "
SRC_URI:append = " file://defconfig"

# Ensure the kernel is built with debug symbols
EXTRA_OEMAKE:append = " KCFLAGS='-g2 -O1' "

KERNEL_CMDLINE = "console=ttyS0,115200 loglevel=5"

IMAGE_INSTALL:append = " gdb dtc"

KERNEL_IMAGETYPES += "vmlinux"
KERNEL_EXTRA_SYMLINKS += "System.map"

INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"

