SUMMARY = "Linux kernel packet filtering driver"
DESCRIPTION = "Custom packet filtering kernel driver for BeagleBone AI-64 TI TDA4VM"
HOMEPAGE = "https://github.com/js.ramesh1990/BeagleBone_AI-64-TI_TDA4VM"
SECTION = "kernel/modules"

LICENSE = "CLOSED"

inherit module systemd

SRC_URI = " \
    file://0001-packet-filter-driver.patch \
    file://packet-filter.service \
"

S = "${WORKDIR}"

DEPENDS += "virtual/kernel"

RPROVIDES:${PN} += "kernel-module-packet-filter"

KERNEL_MODULE_AUTOLOAD += "packet_filter"

SYSTEMD_SERVICE:${PN} = "packet-filter.service"

SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_compile() {
    oe_runmake \
        -C ${STAGING_KERNEL_BUILDDIR} \
        M=${S} \
        ARCH=${ARCH} \
        CROSS_COMPILE=${TARGET_PREFIX} \
        modules
}

do_install() {

    install -d ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra

    install -m 0644 \
        ${S}/packet_filter.ko \
        ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra/

    install -d ${D}${systemd_system_unitdir}

    install -m 0644 \
        ${WORKDIR}/packet-filter.service \
        ${D}${systemd_system_unitdir}/packet-filter.service
}

FILES:${PN} += " \
    ${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra/packet_filter.ko \
    ${systemd_system_unitdir}/packet-filter.service \
"

SYSTEMD_PACKAGES = "${PN}"
