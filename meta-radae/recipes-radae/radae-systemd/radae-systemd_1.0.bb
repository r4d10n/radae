SUMMARY = "RADAE Systemd Service Files"
DESCRIPTION = "Systemd service, configuration, and management scripts for RADAE"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

RDEPENDS:${PN} = " \
    radae-app \
    systemd \
    bash \
    logrotate \
"

SRC_URI = " \
    file://radae.service \
    file://radae-tx.service \
    file://radae-rx.service \
    file://radae.conf \
    file://radae-env \
    file://radae-ptt.sh \
    file://radae-monitor.sh \
    file://radae.logrotate \
    file://radae-watchdog.service \
    file://radae-watchdog.sh \
"

S = "${WORKDIR}"

inherit systemd

SYSTEMD_SERVICE:${PN} = "radae.service"
SYSTEMD_AUTO_ENABLE = "disable"

do_install() {
    # Install systemd service files
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/radae.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/radae-tx.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/radae-rx.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/radae-watchdog.service ${D}${systemd_system_unitdir}/

    # Install configuration
    install -d ${D}${sysconfdir}/radae
    install -m 0644 ${WORKDIR}/radae.conf ${D}${sysconfdir}/radae/
    install -m 0644 ${WORKDIR}/radae-env ${D}${sysconfdir}/radae/

    # Install helper scripts
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/radae-ptt.sh ${D}${sbindir}/
    install -m 0755 ${WORKDIR}/radae-monitor.sh ${D}${sbindir}/
    install -m 0755 ${WORKDIR}/radae-watchdog.sh ${D}${sbindir}/

    # Install logrotate configuration
    install -d ${D}${sysconfdir}/logrotate.d
    install -m 0644 ${WORKDIR}/radae.logrotate ${D}${sysconfdir}/logrotate.d/radae

    # Create log directory
    install -d ${D}${localstatedir}/log/radae

    # Create runtime directory
    install -d ${D}${localstatedir}/lib/radae
}

do_install:append() {
    # Create systemd tmpfiles configuration for runtime directory
    install -d ${D}${sysconfdir}/tmpfiles.d
    cat > ${D}${sysconfdir}/tmpfiles.d/radae.conf << 'EOF'
# RADAE runtime directory
d /run/radae 0755 radae radae -
d /var/log/radae 0755 radae radae -
EOF
}

FILES:${PN} = " \
    ${systemd_system_unitdir}/radae.service \
    ${systemd_system_unitdir}/radae-tx.service \
    ${systemd_system_unitdir}/radae-rx.service \
    ${systemd_system_unitdir}/radae-watchdog.service \
    ${sysconfdir}/radae/radae.conf \
    ${sysconfdir}/radae/radae-env \
    ${sysconfdir}/logrotate.d/radae \
    ${sysconfdir}/tmpfiles.d/radae.conf \
    ${sbindir}/radae-ptt.sh \
    ${sbindir}/radae-monitor.sh \
    ${sbindir}/radae-watchdog.sh \
    ${localstatedir}/log/radae \
    ${localstatedir}/lib/radae \
"

# Allow configuration override
CONFFILES:${PN} = " \
    ${sysconfdir}/radae/radae.conf \
    ${sysconfdir}/radae/radae-env \
"

pkg_postinst:${PN}() {
    if [ -z "$D" ]; then
        # Create radae user/group if not exists
        if ! grep -q radae /etc/group; then
            groupadd -r radae
        fi
        if ! id radae > /dev/null 2>&1; then
            useradd -r -g radae -G audio,gpio -d /var/lib/radae \
                    -s /bin/false -c "RADAE Service User" radae
        fi

        # Set permissions
        chown -R radae:radae /var/log/radae
        chown -R radae:radae /var/lib/radae
        chmod 755 /var/log/radae
        chmod 755 /var/lib/radae

        # Reload systemd
        systemctl daemon-reload || true

        echo "RADAE systemd services installed."
        echo "Enable with: systemctl enable radae.service"
        echo "Start with: systemctl start radae.service"
    fi
}

pkg_prerm:${PN}() {
    if [ -z "$D" ]; then
        # Stop and disable service before removal
        systemctl stop radae.service radae-tx.service radae-rx.service \
                  radae-watchdog.service 2>/dev/null || true
        systemctl disable radae.service radae-tx.service radae-rx.service \
                  radae-watchdog.service 2>/dev/null || true
    fi
}
