# Modifying the device-tree blob

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://virt_full.dts"

# Copy the DTS into the kernel source folder where the build expects it,
do_patch:prepend() {
   install -Dm0644 ${WORKDIR}/virt_full.dts \
                   ${S}/arch/arm64/boot/dts/arm/virt_full.dts
}        

do_configure:append() {
    if ! grep -q -E '(^|[[:space:]])virt_full\.dtb([[:space:]]|$)' ${S}/arch/arm64/boot/dts/arm/Makefile; then
        echo 'dtb-$(CONFIG_ARCH_VIRT) += virt_full.dtb' >> ${S}/arch/arm64/boot/dts/arm/Makefile
    fi
}

# Adding not replacing 
KERNEL_DEVICETREE:qemuarm64 = "arm/virt_full.dtb"

# Kernel DTC warnings to relax (optional)
DTC_FLAGS += " -Wno-unit_address_vs_reg -Wno-simple_bus_reg"

