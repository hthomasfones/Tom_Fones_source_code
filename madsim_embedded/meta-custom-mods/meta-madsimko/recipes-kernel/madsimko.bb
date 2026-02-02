SUMMARY = "Recipe for the Madsim simulation framework"
DESCRIPTION = "Recipe for the Madsim simulation framework"

LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

PACKAGE_ARCH = "${MACHINE_ARCH}"

#Compile serially
PARALLEL_MAKE = ""

inherit module
DEPENDS += "virtual/kernel"
RDEPENDS_${PN} += "kernel-module-configfs"

#EXTRA_OEMAKE += "KCFLAGS='-Wno-error'"
#EXTRA_OEMAKE += "KCFLAGS=-Wno-error=inline"
EXTRA_OEMAKE += " V=1 KBUILD_VERBOSE=1"

SRC_URI = "file://madbusmain.c    \
           file://mbdevthread.c   \
           file://madbus.h        \
           file://maddefs.h       \
           file://madbusioctls.h  \   
           file://madkonsts.h     \   
           file://simdrvrlib.h    \ 
           file://sim_aliases.h   \ 
           file://maddevbmain.c   \
           file://maddevbio.c     \
           file://maddevb_zoned.c  \
           file://maddevb_blk_utils.c \
           file://maddevcmain.c   \
           file://maddevcio.c     \
           file://maddrvrdefs.c   \
           file://maddrvrdefs.h   \
           file://maddevb.h       \
           file://maddevc.h       \
           file://maddevioctls.h  \   
           file://Makefile"

S = "${WORKDIR}"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.
RPROVIDES:${PN} += " kernel-module-madbus"
RPROVIDES:${PN} += " kernel-module-maddevb"
RPROVIDES:${PN} += " kernel-module-maddevc"

do_install() {
    install -d ${D}/lib/modules/${KERNEL_VERSION}/extra/
    install -m 0644 madbus.ko ${D}/lib/modules/${KERNEL_VERSION}/extra/
    install -m 0644 maddevb.ko ${D}/lib/modules/${KERNEL_VERSION}/extra/
    install -m 0644 maddevc.ko ${D}/lib/modules/${KERNEL_VERSION}/extra/
}

FILES:kernel-module-madbus = "lib/modules/${KERNEL_VERSION}/extra/madbus.ko"
FILES:kernel-module-maddevb = "lib/modules/${KERNEL_VERSION}/extra/maddevb.ko"
FILES:kernel-module-maddevc = "lib/modules/${KERNEL_VERSION}/extra/maddevc.ko"

INSANE_SKIP:${PN} += "moduleref"
