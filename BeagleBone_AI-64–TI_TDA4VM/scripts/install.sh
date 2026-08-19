#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# Installation Script
#
# Installs:
#   - packet_filter.ko
#   - Packet filter configuration files
#   - Runtime directories
#   - Userspace utility
#   - Systemd service
#
# Target:
#   BeagleBone AI-64 / TI TDA4VM
#
# ============================================================

set -e


# ------------------------------------------------------------
# Project Root
# ------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_ROOT}"


# ------------------------------------------------------------
# Project Paths
# ------------------------------------------------------------

BUILD_DIR="${PROJECT_ROOT}/build"
DRIVER_DIR="${PROJECT_ROOT}/kernel/packet_filter"

RULE_DIR="${PROJECT_ROOT}/configs/rules"

DRIVER_MODULE="${BUILD_DIR}/driver/packet_filter.ko"


# ------------------------------------------------------------
# Default Target RootFS
# ------------------------------------------------------------

TARGET_ROOTFS="${TARGET_ROOTFS:-/}"


# ------------------------------------------------------------
# Installation Directories
# ------------------------------------------------------------

MODULE_DIR="/lib/modules"

CONFIG_DIR="/etc/packet_filter"

RULE_INSTALL_DIR="${CONFIG_DIR}/rules"

BIN_DIR="/usr/bin"

SBIN_DIR="/usr/sbin"

SYSTEMD_DIR="/etc/systemd/system"

LOG_DIR="/var/log/packet_filter"

RUN_DIR="/run/packet_filter"


# ------------------------------------------------------------
# Colors
# ------------------------------------------------------------

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'


# ------------------------------------------------------------
# Logging
# ------------------------------------------------------------

info()
{
    echo -e "${BLUE}[INFO]${NC} $1"
}


success()
{
    echo -e "${GREEN}[OK]${NC} $1"
}


warning()
{
    echo -e "${YELLOW}[WARNING]${NC} $1"
}


error()
{
    echo -e "${RED}[ERROR]${NC} $1"
}


# ------------------------------------------------------------
# Banner
# ------------------------------------------------------------

print_banner()
{
    echo
    echo "============================================================"
    echo "       BeagleBone AI-64 - TI TDA4VM"
    echo "       Packet Filter Installation"
    echo "============================================================"
    echo
}


# ------------------------------------------------------------
# RootFS Path Helper
# ------------------------------------------------------------

rootfs_path()
{
    local path="$1"

    if [ "${TARGET_ROOTFS}" = "/" ]
    then
        echo "${path}"
    else
        echo "${TARGET_ROOTFS}${path}"
    fi
}


# ------------------------------------------------------------
# Check Root Permissions
# ------------------------------------------------------------

check_permissions()
{
    if [ "${TARGET_ROOTFS}" = "/" ]
    then

        if [ "$(id -u)" -ne 0 ]
        then
            error "Installation to the running system requires root."
            echo
            echo "Run:"
            echo
            echo "  sudo ./scripts/install.sh"
            echo
            exit 1
        fi

    else

        if [ "$(id -u)" -ne 0 ]
        then
            warning "Installing into target RootFS without root."
            warning "Permission errors may occur."
        fi

    fi
}


# ------------------------------------------------------------
# Check Kernel Module
# ------------------------------------------------------------

check_module()
{
    info "Checking packet filter kernel module..."

    if [ ! -f "${DRIVER_MODULE}" ]
    then
        error "packet_filter.ko not found:"
        error "${DRIVER_MODULE}"
        echo
        echo "Build the project first:"
        echo
        echo "  ./scripts/build.sh"
        echo
        exit 1
    fi

    success "Found packet_filter.ko"

    echo
    echo "Module:"
    file "${DRIVER_MODULE}"
}


# ------------------------------------------------------------
# Get Target Kernel Version
# ------------------------------------------------------------

get_kernel_version()
{
    local version=""

    if [ -n "${KERNEL_VERSION:-}" ]
    then
        version="${KERNEL_VERSION}"
    elif [ -d "${PROJECT_ROOT}/kernel/include/config" ]
    then
        version="$(make -s -C "${PROJECT_ROOT}/kernel" kernelversion 2>/dev/null || true)"
    fi

    if [ -z "${version}" ] && [ "${TARGET_ROOTFS}" = "/" ]
    then
        version="$(uname -r)"
    fi

    if [ -z "${version}" ]
    then
        error "Unable to determine target kernel version."
        echo
        echo "Specify it explicitly:"
        echo
        echo "  KERNEL_VERSION=<version> ./scripts/install.sh"
        echo
        exit 1
    fi

    echo "${version}"
}


# ------------------------------------------------------------
# Create Installation Directories
# ------------------------------------------------------------

create_directories()
{
    info "Creating installation directories..."

    mkdir -p "$(rootfs_path "${CONFIG_DIR}")"
    mkdir -p "$(rootfs_path "${RULE_INSTALL_DIR}")"
    mkdir -p "$(rootfs_path "${LOG_DIR}")"
    mkdir -p "$(rootfs_path "${RUN_DIR}")"

    success "Runtime directories created."
}


# ------------------------------------------------------------
# Install Kernel Module
# ------------------------------------------------------------

install_module()
{
    local kernel_version

    kernel_version="$(get_kernel_version)"

    local module_path
    module_path="$(rootfs_path "${MODULE_DIR}/${kernel_version}/extra")"

    info "Installing kernel module..."

    mkdir -p "${module_path}"

    install \
        -m 0644 \
        "${DRIVER_MODULE}" \
        "${module_path}/packet_filter.ko"

    success "Installed:"
    echo "  ${module_path}/packet_filter.ko"

    if [ "${TARGET_ROOTFS}" = "/" ]
    then

        info "Running depmod..."

        depmod "${kernel_version}" || true

        success "Kernel module dependency database updated."

    else

        warning "Target RootFS installation detected."
        warning "Run depmod on the target after boot."

    fi
}


# ------------------------------------------------------------
# Install Rule Configuration
# ------------------------------------------------------------

install_rules()
{
    info "Installing packet-filter rules..."

    local installed=0

    if [ -f "${RULE_DIR}/whitelist.conf" ]
    then

        install \
            -m 0644 \
            "${RULE_DIR}/whitelist.conf" \
            "$(rootfs_path "${RULE_INSTALL_DIR}/whitelist.conf")"

        success "Installed whitelist.conf"

        installed=$((installed + 1))

    else

        warning "whitelist.conf not found."

    fi


    if [ -f "${RULE_DIR}/blacklist.conf" ]
    then

        install \
            -m 0644 \
            "${RULE_DIR}/blacklist.conf" \
            "$(rootfs_path "${RULE_INSTALL_DIR}/blacklist.conf")"

        success "Installed blacklist.conf"

        installed=$((installed + 1))

    else

        warning "blacklist.conf not found."

    fi


    if [ -f "${RULE_DIR}/monitoring.conf" ]
    then

        install \
            -m 0644 \
            "${RULE_DIR}/monitoring.conf" \
            "$(rootfs_path "${RULE_INSTALL_DIR}/monitoring.conf")"

        success "Installed monitoring.conf"

        installed=$((installed + 1))

    else

        warning "monitoring.conf not found."

    fi

    echo
    info "Rule files installed: ${installed}"
}


# ------------------------------------------------------------
# Create Default Runtime Configuration
# ------------------------------------------------------------

create_runtime_config()
{
    local config_file

    config_file="$(rootfs_path "${CONFIG_DIR}/packet_filter.conf")"

    info "Creating runtime configuration..."

    cat > "${config_file}" <<EOF
#
# BeagleBone AI-64 Packet Filter
# Runtime Configuration
#

# Packet filter
enabled=1

# Default action
default_action=allow

# Logging
logging=1

# Statistics
statistics=1

# Rules
whitelist=${RULE_INSTALL_DIR}/whitelist.conf
blacklist=${RULE_INSTALL_DIR}/blacklist.conf
monitoring=${RULE_INSTALL_DIR}/monitoring.conf
EOF

    chmod 0644 "${config_file}"

    success "Runtime configuration created:"
    echo "  ${config_file}"
}


# ------------------------------------------------------------
# Install Userspace Utility
# ------------------------------------------------------------

install_userspace_tool()
{
    local source=""

    local candidates=(
        "${PROJECT_ROOT}/userspace/packet_filter_ctl"
        "${PROJECT_ROOT}/userspace/packet_filter"
        "${PROJECT_ROOT}/tools/packet_filter_ctl"
    )

    for candidate in "${candidates[@]}"
    do
        if [ -f "${candidate}" ]
        then
            source="${candidate}"
            break
        fi
    done

    if [ -z "${source}" ]
    then
        warning "Packet-filter userspace utility not found."
        warning "Skipping userspace utility installation."
        return 0
    fi

    info "Installing userspace packet-filter utility..."

    install \
        -m 0755 \
        "${source}" \
        "$(rootfs_path "${SBIN_DIR}/packet_filter_ctl")"

    success "Userspace utility installed."
}


# ------------------------------------------------------------
# Install Systemd Service
# ------------------------------------------------------------

install_systemd_service()
{
    local service_file

    service_file="$(rootfs_path "${SYSTEMD_DIR}/packet-filter.service")"

    info "Installing packet-filter systemd service..."

    mkdir -p "$(dirname "${service_file}")"

    cat > "${service_file}" <<'EOF'
[Unit]
Description=BeagleBone AI-64 Packet Filter
After=network-pre.target
Before=network.target
Wants=network-pre.target

[Service]
Type=oneshot
RemainAfterExit=yes

ExecStart=/sbin/modprobe packet_filter
ExecStop=/sbin/rmmod packet_filter

ExecStartPost=/bin/sh -c '/usr/sbin/packet_filter_ctl load-rules || true'

[Install]
WantedBy=multi-user.target
EOF

    chmod 0644 "${service_file}"

    success "Systemd service installed:"
    echo "  ${service_file}"
}


# ------------------------------------------------------------
# Enable Service On Running Target
# ------------------------------------------------------------

enable_service()
{
    if [ "${TARGET_ROOTFS}" != "/" ]
    then
        warning "Target RootFS detected."
        warning "Systemd service was installed but not enabled."
        warning "Enable it after boot with:"
        echo
        echo "  systemctl enable packet-filter.service"
        echo
        return 0
    fi

    if ! command -v systemctl >/dev/null 2>&1
    then
        warning "systemctl not available."
        return 0
    fi

    info "Enabling packet-filter service..."

    systemctl daemon-reload

    systemctl enable packet-filter.service

    success "packet-filter.service enabled."
}


# ------------------------------------------------------------
# Install Module Alias
# ------------------------------------------------------------

install_module_config()
{
    local modules_file

    modules_file="$(rootfs_path "/etc/modules-load.d/packet_filter.conf")"

    info "Installing module-load configuration..."

    mkdir -p "$(dirname "${modules_file}")"

    cat > "${modules_file}" <<EOF
# Packet Filter kernel module
packet_filter
EOF

    chmod 0644 "${modules_file}"

    success "Module-load configuration installed."
}


# ------------------------------------------------------------
# Install Permissions
# ------------------------------------------------------------

set_permissions()
{
    info "Setting packet-filter permissions..."

    chmod 0755 \
        "$(rootfs_path "${CONFIG_DIR}")" \
        "$(rootfs_path "${RULE_INSTALL_DIR}")" \
        "$(rootfs_path "${LOG_DIR}")" \
        "$(rootfs_path "${RUN_DIR}")"

    chmod 0644 \
        "$(rootfs_path "${RULE_INSTALL_DIR}")"/*.conf \
        2>/dev/null || true

    success "Permissions configured."
}


# ------------------------------------------------------------
# Verify Installation
# ------------------------------------------------------------

verify_installation()
{
    local kernel_version

    kernel_version="$(get_kernel_version)"

    echo
    echo "============================================================"
    echo "              INSTALLATION VERIFICATION"
    echo "============================================================"

    echo
    echo "Kernel module:"

    if [ -f "$(rootfs_path "${MODULE_DIR}/${kernel_version}/extra/packet_filter.ko")" ]
    then
        echo "  [OK] packet_filter.ko"
    else
        echo "  [FAIL] packet_filter.ko"
    fi


    echo
    echo "Configuration:"

    if [ -f "$(rootfs_path "${CONFIG_DIR}/packet_filter.conf")" ]
    then
        echo "  [OK] packet_filter.conf"
    else
        echo "  [FAIL] packet_filter.conf"
    fi


    echo
    echo "Rules:"

    for rule in \
        whitelist.conf \
        blacklist.conf \
        monitoring.conf
    do

        if [ -f "$(rootfs_path "${RULE_INSTALL_DIR}/${rule}")" ]
        then
            echo "  [OK] ${rule}"
        else
            echo "  [--] ${rule}"
        fi

    done


    echo
    echo "Systemd:"

    if [ -f "$(rootfs_path "${SYSTEMD_DIR}/packet-filter.service")" ]
    then
        echo "  [OK] packet-filter.service"
    else
        echo "  [FAIL] packet-filter.service"
    fi


    echo
    echo "============================================================"
}


# ------------------------------------------------------------
# Installation Summary
# ------------------------------------------------------------

installation_summary()
{
    local kernel_version

    kernel_version="$(get_kernel_version)"

    echo
    echo "============================================================"
    echo "             INSTALLATION SUMMARY"
    echo "============================================================"

    echo
    echo "Target RootFS:"
    echo "  ${TARGET_ROOTFS}"

    echo
    echo "Kernel Version:"
    echo "  ${kernel_version}"

    echo
    echo "Kernel Module:"
    echo "  ${MODULE_DIR}/${kernel_version}/extra/packet_filter.ko"

    echo
    echo "Configuration:"
    echo "  ${CONFIG_DIR}/packet_filter.conf"

    echo
    echo "Rules:"
    echo "  ${RULE_INSTALL_DIR}/whitelist.conf"
    echo "  ${RULE_INSTALL_DIR}/blacklist.conf"
    echo "  ${RULE_INSTALL_DIR}/monitoring.conf"

    echo
    echo "Systemd:"
    echo "  ${SYSTEMD_DIR}/packet-filter.service"

    echo
    echo "Logs:"
    echo "  ${LOG_DIR}"

    echo
    echo "============================================================"
}


# ------------------------------------------------------------
# Normal Installation
# ------------------------------------------------------------

install_all()
{
    print_banner

    check_permissions

    check_module

    create_directories

    install_module

    install_rules

    create_runtime_config

    install_module_config

    install_userspace_tool

    install_systemd_service

    set_permissions

    enable_service

    verify_installation

    installation_summary

    echo
    success "Packet filter installation completed."
    echo
}


# ------------------------------------------------------------
# Install Module Only
# ------------------------------------------------------------

install_module_only()
{
    print_banner

    check_permissions

    check_module

    install_module

    echo
    success "Kernel module installation completed."
}


# ------------------------------------------------------------
# Install Rules Only
# ------------------------------------------------------------

install_rules_only()
{
    print_banner

    check_permissions

    create_directories

    install_rules

    create_runtime_config

    set_permissions

    echo
    success "Rule configuration installation completed."
}


# ------------------------------------------------------------
# Usage
# ------------------------------------------------------------

usage()
{
    echo
    echo "Usage:"
    echo
    echo "  ./scripts/install.sh"
    echo "  ./scripts/install.sh install"
    echo "  ./scripts/install.sh module"
    echo "  ./scripts/install.sh rules"
    echo "  ./scripts/install.sh verify"
    echo "  ./scripts/install.sh help"
    echo
    echo "Target RootFS:"
    echo
    echo "  TARGET_ROOTFS=/mnt/rootfs ./scripts/install.sh"
    echo
    echo "Kernel Version:"
    echo
    echo "  KERNEL_VERSION=6.x.x ./scripts/install.sh"
    echo
}


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

main()
{
    case "${1:-install}" in

        install)
            install_all
            ;;

        module)
            install_module_only
            ;;

        rules)
            install_rules_only
            ;;

        verify)
            print_banner
            check_permissions
            check_module
            verify_installation
            ;;

        help|-h|--help)
            usage
            ;;

        *)
            error "Unknown command: $1"
            usage
            exit 1
            ;;

    esac
}


# ------------------------------------------------------------
# Execute
# ------------------------------------------------------------

main "$@"
