SUMMARY = "Recipe for the ucrash program"
DESCRIPTION = "Recipe for the ucrash program"

LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://ucrash.cpp file://ucrash.h"

PACKAGE_ARCH = "${MACHINE_ARCH}"

S = "${WORKDIR}"

do_compile() {
    mkdir -p ${S}
    ${CXX} ${CFLAGS} ${LDFLAGS} ucrash.cpp -o ucrash
    touch ucrash
}
    
do_install() {
    install -d ${D}${bindir}   
    install -m 0755 ucrash ${D}${bindir}/ucrash
}         

FILES:${PN} += "${bindir}/ucrash"
