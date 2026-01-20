SUMMARY = "Recipe for the kernel crash module - krash.ko"
DESCRIPTION = "Recipe for the kernel crash module - krash.ko"

LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

PACKAGE_ARCH = "${MACHINE_ARCH}"

DEPENDS = "virtual/kernel"
inherit module

SRC_URI = "file://krash.c  \
           file://krash.h  \
           file://ucrash.h  \
           file://Makefile"

S = "${WORKDIR}"
#SOURCE = "${WORKDIR}/build"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.
RPROVIDES:${PN} += " kernel-module-krash"

do_install() {
    install -d ${D}/lib/modules/${KERNEL_VERSION}/extra/
    install -m 0644 krash.ko ${D}/lib/modules/${KERNEL_VERSION}/extra/
}

FILES:kernel-module-krash = "lib/modules/${KERNEL_VERSION}/extra/krash.ko"

INSANE_SKIP:${PN} += "moduleref"

#IMAGE_INSTALL:append = " kernel-module-krash"
