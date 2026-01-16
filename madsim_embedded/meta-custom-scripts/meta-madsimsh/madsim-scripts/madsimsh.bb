SUMMARY = "Installs the madsim-setup bash script"
DESCRIPTION = "Bash script for kernel module environment setup"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://madsim-debug.sh \
           file://madsetup.sh \
           file://madenv.sh \
           file://madinsmods.sh \
           file://madappintro.sh \
           file://madrawblk.sh \
           file://madresults.sh \
           file://mad2drvrstest.sh \
           file://madalldevicestest.sh \
           file://madbufrdiotest.sh \
           file://madcachetest.sh \
           file://maddisktest.sh \
           file://maddriverstack.sh \
           file://madhotplugtest.sh \
           file://madmmaptest.sh \
           file://madmsitest.sh \
           file://madrandomiotest.sh \
           file://setup-debug.sh \
           file://envcheck.sh \
           file://grub.cfg       \
           "

PACKAGE_ARCH = "${MACHINE_ARCH}"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}/scripts/
    install -m 0755 ${S}/madsim-debug.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madsetup.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madenv.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madinsmods.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madrawblk.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madappintro.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madresults.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/mad2drvrstest.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madalldevicestest.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madbufrdiotest.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madcachetest.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/maddisktest.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/maddriverstack.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madhotplugtest.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madmmaptest.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madmsitest.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/madrandomiotest.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/setup-debug.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/envcheck.sh ${D}${bindir}/scripts/
    install -m 0755 ${S}/grub.cfg ${D}${bindir}/scripts/
}

FILES:${PN} += "${bindir}/scripts/madsim-debug.sh"
FILES:${PN} +=  "${bindir}/scripts/madsetup.sh"
FILES:${PN} +=  "${bindir}/scripts/madenv.sh"
FILES:${PN} +=  "${bindir}/scripts/madinsmods.sh"
FILES:${PN} +=  "${bindir}/scripts/madappintro.sh"
FILES:${PN} +=  "${bindir}/scripts/madrawblk.sh"
FILES:${PN} +=  "${bindir}/scripts/madresults.sh"
FILES:${PN} +=  "${bindir}/scripts/mad2drvrstest.sh"
FILES:${PN} +=  "${bindir}/scripts/madalldevicestest.sh"
FILES:${PN} +=  "${bindir}/scripts/madbufrdiotest.sh"
FILES:${PN} +=  "${bindir}/scripts/madcachetest.sh"
FILES:${PN} +=  "${bindir}/scripts/maddisktest.sh"
FILES:${PN} +=  "${bindir}/scripts/maddriverstack.sh"
FILES:${PN} +=  "${bindir}/scripts/madhotplugtest.sh"
FILES:${PN} +=  "${bindir}/scripts/madmmaptest.sh"
FILES:${PN} +=  "${bindir}/scripts/madmsitest.sh"
FILES:${PN} +=  "${bindir}/scripts/madrandomiotest.sh"
FILES:${PN} += "${bindir}/scripts/setup-debug.sh"
FILES:${PN} += "${bindir}/scripts/envcheck.sh"
FILES:${PN} += "${bindir}/scripts/grub.cfg"
