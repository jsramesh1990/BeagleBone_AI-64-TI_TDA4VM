#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# Configuration Script
#
# Responsibilities:
#   - Validate project configuration
#   - Configure Linux kernel
#   - Apply packet-filter defconfig
#   - Configure Device Tree options
#   - Validate packet-filter rules
#   - Prepare build directories
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

CONFIG_DIR="${PROJECT_ROOT}/configs"
KERNEL_CONFIG_DIR="${CONFIG_DIR}/kernel"
RULE_CONFIG_DIR="${CONFIG_DIR}/rules"

KERNEL_DIR="${PROJECT_ROOT}/kernel"

BUILD_DIR="${PROJECT_ROOT}/build"
CONFIG_BUILD_DIR="${BUILD_DIR}/config"
LOG_DIR="${BUILD_DIR}/logs"


# ------------------------------------------------------------
# Architecture
# ------------------------------------------------------------

ARCH="${ARCH:-arm64}"

CROSS_COMPILE="${CROSS_COMPILE:-aarch64-linux-gnu-}"


# ------------------------------------------------------------
# Kernel Configuration
# ------------------------------------------------------------

KERNEL_DEFCONFIG="${KERNEL_CONFIG_DIR}/packet_filter_defconfig"


# ------------------------------------------------------------
# Rule Configuration
# ------------------------------------------------------------

WHITELIST="${RULE_CONFIG_DIR}/whitelist.conf"
BLACKLIST="${RULE_CONFIG_DIR}/blacklist.conf"
MONITORING="${RULE_CONFIG_DIR}/monitoring.conf"


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
    echo "       Packet Filter Configuration"
    echo "============================================================"
    echo
}


# ------------------------------------------------------------
# Create Configuration Directories
# ------------------------------------------------------------

create_directories()
{
    info "Creating configuration directories..."

    mkdir -p "${CONFIG_BUILD_DIR}"
    mkdir -p "${LOG_DIR}"

    success "Configuration directories ready."
}


# ------------------------------------------------------------
# Check Kernel Source
# ------------------------------------------------------------

check_kernel()
{
    info "Checking Linux kernel source..."

    if [ ! -d "${KERNEL_DIR}" ]
    then
        error "Kernel directory does not exist:"
        error "${KERNEL_DIR}"
        exit 1
    fi

    if [ ! -f "${KERNEL_DIR}/Makefile" ]
    then
        error "Linux kernel Makefile not found."
        error "${KERNEL_DIR}/Makefile"
        exit 1
    fi

    success "Linux kernel source found."
}


# ------------------------------------------------------------
# Check Kernel Defconfig
# ------------------------------------------------------------

check_defconfig()
{
    info "Checking packet-filter kernel configuration..."

    if [ ! -f "${KERNEL_DEFCONFIG}" ]
    then
        error "Kernel defconfig not found:"
        error "${KERNEL_DEFCONFIG}"
        exit 1
    fi

    success "Found:"
    echo "  ${KERNEL_DEFCONFIG}"
}


# ------------------------------------------------------------
# Check Rule Files
# ------------------------------------------------------------

check_rule_files()
{
    info "Checking packet-filter rule files..."

    local missing=0

    if [ -f "${WHITELIST}" ]
    then
        success "Whitelist configuration found."
    else
        warning "Whitelist configuration missing:"
        warning "${WHITELIST}"
        missing=1
    fi

    if [ -f "${BLACKLIST}" ]
    then
        success "Blacklist configuration found."
    else
        warning "Blacklist configuration missing:"
        warning "${BLACKLIST}"
        missing=1
    fi

    if [ -f "${MONITORING}" ]
    then
        success "Monitoring configuration found."
    else
        warning "Monitoring configuration missing:"
        warning "${MONITORING}"
        missing=1
    fi

    return 0
}


# ------------------------------------------------------------
# Validate Rule Files
# ------------------------------------------------------------

validate_rule_files()
{
    info "Validating packet-filter rule files..."

    local file

    for file in \
        "${WHITELIST}" \
        "${BLACKLIST}" \
        "${MONITORING}"
    do

        if [ ! -f "${file}" ]
        then
            continue
        fi

        if grep -q $'\r' "${file}"
        then
            warning "Windows CRLF line endings detected:"
            warning "${file}"

            sed -i 's/\r$//' "${file}"

            success "Converted to Unix line endings."
        fi

    done

    success "Rule configuration validation completed."
}


# ------------------------------------------------------------
# Backup Existing Kernel Configuration
# ------------------------------------------------------------

backup_kernel_config()
{
    if [ -f "${KERNEL_DIR}/.config" ]
    then

        info "Existing kernel .config found."

        cp \
            "${KERNEL_DIR}/.config" \
            "${CONFIG_BUILD_DIR}/kernel.config.backup"

        success "Existing kernel configuration backed up."

    else

        info "No existing kernel .config found."

    fi
}


# ------------------------------------------------------------
# Apply Packet Filter Defconfig
# ------------------------------------------------------------

apply_defconfig()
{
    info "Applying packet-filter kernel configuration..."

    backup_kernel_config

    cp \
        "${KERNEL_DEFCONFIG}" \
        "${KERNEL_DIR}/.config"

    success "packet_filter_defconfig copied to kernel/.config."
}


# ------------------------------------------------------------
# Generate Kernel Configuration
# ------------------------------------------------------------

generate_kernel_config()
{
    info "Running kernel olddefconfig..."

    make \
        -C "${KERNEL_DIR}" \
        ARCH="${ARCH}" \
        CROSS_COMPILE="${CROSS_COMPILE}" \
        olddefconfig \
        2>&1 | tee \
        "${LOG_DIR}/kernel-config.log"

    success "Kernel configuration generated."
}


# ------------------------------------------------------------
# Verify Important Kernel Options
# ------------------------------------------------------------

verify_kernel_options()
{
    info "Checking important kernel configuration options..."

    local config="${KERNEL_DIR}/.config"

    if [ ! -f "${config}" ]
    then
        error "Kernel .config was not generated."
        exit 1
    fi


    echo
    echo "Kernel configuration:"
    echo "--------------------------------------------"


    check_option()
    {
        local option="$1"

        if grep -q "^${option}=" "${config}"
        then
            echo "  [OK] ${option}"
        elif grep -q "^# ${option} is not set" "${config}"
        then
            echo "  [--] ${option} disabled"
        else
            echo "  [??] ${option} not defined"
        fi
    }


    check_option "CONFIG_NET"
    check_option "CONFIG_INET"
    check_option "CONFIG_PACKET"
    check_option "CONFIG_NETFILTER"
    check_option "CONFIG_NETFILTER_ADVANCED"
    check_option "CONFIG_BPF"
    check_option "CONFIG_BPF_SYSCALL"
    check_option "CONFIG_DEBUG_KERNEL"
    check_option "CONFIG_KALLSYMS"
    check_option "CONFIG_MODULES"


    echo "--------------------------------------------"
}


# ------------------------------------------------------------
# Verify Driver Sources
# ------------------------------------------------------------

verify_driver_sources()
{
    info "Checking packet-filter driver sources..."

    local DRIVER_DIR="${KERNEL_DIR}/packet_filter"

    local files=(
        "Makefile"
        "ioctl_defs.h"
        "logging.c"
        "logging.h"
        "packet_filter.c"
        "packet_filter.h"
        "packet_parser.c"
        "packet_parser.h"
        "rule_engine.c"
        "rule_engine.h"
        "statistics.c"
        "statistics.h"
    )

    local file

    for file in "${files[@]}"
    do

        if [ -f "${DRIVER_DIR}/${file}" ]
        then
            echo "  [OK] ${file}"
        else
            warning "Missing driver file: ${file}"
        fi

    done

    success "Driver source verification completed."
}


# ------------------------------------------------------------
# Create Configuration Summary
# ------------------------------------------------------------

create_summary()
{
    local summary="${CONFIG_BUILD_DIR}/configuration-summary.txt"

    info "Creating configuration summary..."

    cat > "${summary}" <<EOF
============================================================
BeagleBone AI-64 - TI TDA4VM
Packet Filter Configuration Summary
============================================================

Project Root:
${PROJECT_ROOT}

Architecture:
${ARCH}

Cross Compiler:
${CROSS_COMPILE}

Kernel Directory:
${KERNEL_DIR}

Kernel Defconfig:
${KERNEL_DEFCONFIG}

Kernel Configuration:
${KERNEL_DIR}/.config

Rule Configuration:

Whitelist:
${WHITELIST}

Blacklist:
${BLACKLIST}

Monitoring:
${MONITORING}

Build Directory:
${BUILD_DIR}

Configuration Logs:
${LOG_DIR}/kernel-config.log

============================================================
EOF

    success "Configuration summary created:"
    echo "  ${summary}"
}


# ------------------------------------------------------------
# Display Configuration
# ------------------------------------------------------------

show_configuration()
{
    echo
    echo "============================================================"
    echo "              CURRENT CONFIGURATION"
    echo "============================================================"

    echo
    echo "Project Root:"
    echo "  ${PROJECT_ROOT}"

    echo
    echo "Architecture:"
    echo "  ${ARCH}"

    echo
    echo "Cross Compiler:"
    echo "  ${CROSS_COMPILE}"

    echo
    echo "Kernel:"
    echo "  ${KERNEL_DIR}"

    echo
    echo "Kernel Defconfig:"
    echo "  ${KERNEL_DEFCONFIG}"

    echo
    echo "Rules:"
    echo "  ${WHITELIST}"
    echo "  ${BLACKLIST}"
    echo "  ${MONITORING}"

    echo
    echo "Build:"
    echo "  ${BUILD_DIR}"

    echo
    echo "============================================================"
}


# ------------------------------------------------------------
# Main Configuration Flow
# ------------------------------------------------------------

configure()
{
    print_banner

    create_directories

    check_kernel

    check_defconfig

    check_rule_files

    validate_rule_files

    verify_driver_sources

    apply_defconfig

    generate_kernel_config

    verify_kernel_options

    create_summary

    show_configuration

    echo
    success "Project configuration completed successfully."
    echo
}


# ------------------------------------------------------------
# Show Help
# ------------------------------------------------------------

usage()
{
    echo
    echo "Usage:"
    echo
    echo "  ./scripts/configure.sh"
    echo "  ./scripts/configure.sh configure"
    echo "  ./scripts/configure.sh show"
    echo "  ./scripts/configure.sh verify"
    echo "  ./scripts/configure.sh help"
    echo
    echo "Commands:"
    echo
    echo "  configure   Configure kernel and project"
    echo "  show        Display current configuration"
    echo "  verify      Verify project configuration"
    echo "  help        Display this help"
    echo
}


# ------------------------------------------------------------
# Verification Only
# ------------------------------------------------------------

verify()
{
    print_banner

    create_directories

    check_kernel

    check_defconfig

    check_rule_files

    validate_rule_files

    verify_driver_sources

    echo
    success "Configuration verification completed."
}


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

main()
{
    case "${1:-configure}" in

        configure)
            configure
            ;;

        show)
            print_banner
            show_configuration
            ;;

        verify)
            verify
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
