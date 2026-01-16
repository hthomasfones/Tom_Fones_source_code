SUMMARY = "Recipe for the madsimui test programs"
DESCRIPTION = "Recipe for the madsimui test programs"

LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS += "libaio"

SRC_URI = "file://madsimui.cpp  \
           file://madtest.cpp   \
           file://madtestb.cpp  \
           file://madtestc.cpp   \
           file://madsimui.h    \ 
           file://madtest.h     \
           file://madapplib.h   \ 
           file://maddefs.h     \ 
           file://madkonsts.h   \
           file://madlib.h      \ 
           file://madbusioctls.h \
           file://maddevioctls.h \
           "

PACKAGE_ARCH = "${MACHINE_ARCH}"

S = "${WORKDIR}"

do_compile() {
    mkdir -p ${S}
    ${CXX} ${CFLAGS} ${LDFLAGS} madsimui.cpp -o madsimui
    ${CXX} ${CFLAGS}  -I. ${LDFLAGS} madtestb.cpp -laio -o madtestb
    ${CXX} ${CFLAGS}  -I. ${LDFLAGS} madtestc.cpp -laio -o madtestc
    touch madsimui
}

RDEPENDS:${PN} += "libaio"
    
do_install() {
    install -d ${D}${bindir}   
    install -m 0755 madsimui ${D}${bindir}/madsimui
    install -m 0755 madtestb ${D}${bindir}/madtestb
    install -m 0755 madtestc ${D}${bindir}/madtestc
}         

FILES:${PN} += "${bindir}/madsimui"
FILES:${PN} += "${bindir}/madtestb"
FILES:${PN} += "${bindir}/madtestc"
