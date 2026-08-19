#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# Clean Script
#
# Removes:
#   - Kernel build artifacts
#   - Packet filter driver objects
#   - Generated kernel module
#   - Device Tree build artifacts
#   - Project build logs
#   - Temporary files
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
KERNEL_DIR="${PROJECT_ROOT}/kernel"
DRIVER_DIR="${KERNEL_DIR}/packet_filter"


# ------------------------------------------------------------
# Colors
# ------------------------------------------------------------

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'


# ------------------------------------------------------------
# Logging Functions
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
    echo "        BeagleBone AI-64 - TI TDA4VM"
    echo "        Packet Filter Clean System"
    echo "============================================================"
    echo
}


# ------------------------------------------------------------
# Clean Packet Filter Driver
# ------------------------------------------------------------

clean_driver()
{
    info "Cleaning packet filter kernel module..."

    if [ -d "${DRIVER_DIR}" ]
    then

        if [ -f "${DRIVER_DIR}/Makefile" ]
        then

            make \
                -C "${DRIVER_DIR}" \
                clean \
                2>/dev/null || true

        fi

        rm -f "${DRIVER_DIR}"/*.o
        rm -f "${DRIVER_DIR}"/*.ko
        rm -f "${DRIVER_DIR}"/*.mod
        rm -f "${DRIVER_DIR}"/*.mod.c
        rm -f "${DRIVER_DIR}"/modules.order
        rm -f "${DRIVER_DIR}"/Module.symvers
        rm -rf "${DRIVER_DIR}"/.tmp_versions

        find "${DRIVER_DIR}" \
            -type f \
            -name ".*.cmd" \
            -delete 2>/dev/null || true

        success "Packet filter driver cleaned."

    else

        warning "Packet filter directory not found."

    fi
}


# ------------------------------------------------------------
# Clean Kernel Build
# ------------------------------------------------------------

clean_kernel()
{
    info "Cleaning Linux kernel build artifacts..."

    if [ -d "${KERNEL_DIR}" ]
    then

        if [ -f "${KERNEL_DIR}/Makefile" ]
        then

            make \
                -C "${KERNEL_DIR}" \
                ARCH=arm64 \
                clean \
                2>/dev/null || true

            success "Linux kernel artifacts cleaned."

        else

            warning "Kernel Makefile not found."

        fi

    else

        warning "Kernel directory not found."

    fi
}


# ------------------------------------------------------------
# Clean Project Build Directory
# ------------------------------------------------------------

clean_build_directory()
{
    info "Cleaning project build directory..."

    if [ -d "${BUILD_DIR}" ]
    then

        rm -rf "${BUILD_DIR}"

        success "Project build directory removed."

    else

        info "Build directory does not exist."

    fi
}


# ------------------------------------------------------------
# Clean Temporary Files
# ------------------------------------------------------------

clean_temporary_files()
{
    info "Removing temporary files..."

    find "${PROJECT_ROOT}" \
        -type f \
        \( \
            -name "*~" \
            -o -name "*.tmp" \
            -o -name "*.swp" \
            -o -name "*.swo" \
        \) \
        -delete 2>/dev/null || true

    success "Temporary files removed."
}


# ------------------------------------------------------------
# Clean Generated Device Tree Files
# ------------------------------------------------------------

clean_device_tree()
{
    info "Cleaning generated Device Tree artifacts..."

    if [ -d "${KERNEL_DIR}" ]
    then

        find "${KERNEL_DIR}" \
            -type f \
            \( \
                -name "*.dtb" \
                -o -name "*.dtbo" \
            \) \
            -delete 2>/dev/null || true

        success "Device Tree artifacts cleaned."

    else

        warning "Kernel directory not found."

    fi
}


# ------------------------------------------------------------
# Clean Kernel Generated Configuration
#
# NOTE:
# This does NOT remove the source-controlled
# configs/kernel/packet_filter_defconfig.
# ------------------------------------------------------------

clean_kernel_config()
{
    info "Removing generated kernel configuration..."

    if [ -f "${KERNEL_DIR}/.config" ]
    then
        rm -f "${KERNEL_DIR}/.config"
        success "Generated .config removed."
    else
        info "No generated kernel .config found."
    fi
}


# ------------------------------------------------------------
# Deep Clean
# ------------------------------------------------------------

deep_clean()
{
    print_banner

    warning "Starting DEEP CLEAN."
    warning "Generated kernel configuration will also be removed."
    echo

    clean_driver
    clean_device_tree
    clean_kernel
    clean_kernel_config
    clean_build_directory
    clean_temporary_files

    echo
    success "Deep clean completed."
}


# ------------------------------------------------------------
# Normal Clean
# ------------------------------------------------------------

normal_clean()
{
    print_banner

    info "Starting normal clean..."

    clean_driver
    clean_build_directory
    clean_temporary_files

    echo
    success "Normal clean completed."
}


# ------------------------------------------------------------
# Kernel Clean Only
# ------------------------------------------------------------

kernel_clean()
{
    print_banner

    info "Cleaning kernel build artifacts..."

    clean_kernel
    clean_device_tree

    echo
    success "Kernel clean completed."
}


# ------------------------------------------------------------
# Driver Clean Only
# ------------------------------------------------------------

driver_clean()
{
    print_banner

    info "Cleaning packet filter driver..."

    clean_driver

    echo
    success "Driver clean completed."
}


# ------------------------------------------------------------
# Usage
# ------------------------------------------------------------

usage()
{
    echo
    echo "Usage:"
    echo
    echo "  ./scripts/clean.sh              Normal clean"
    echo "  ./scripts/clean.sh clean        Normal clean"
    echo "  ./scripts/clean.sh driver       Clean driver only"
    echo "  ./scripts/clean.sh kernel       Clean kernel only"
    echo "  ./scripts/clean.sh deep         Deep clean"
    echo "  ./scripts/clean.sh all          Deep clean"
    echo "  ./scripts/clean.sh help         Show help"
    echo
    echo "Normal clean removes:"
    echo "  - packet_filter.ko"
    echo "  - driver object files"
    echo "  - project build directory"
    echo "  - temporary files"
    echo
    echo "Deep clean additionally removes:"
    echo "  - Linux kernel build artifacts"
    echo "  - Device Tree artifacts"
    echo "  - generated kernel .config"
    echo
}


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

main()
{
    case "${1:-clean}" in

        clean)
            normal_clean
            ;;

        driver)
            driver_clean
            ;;

        kernel)
            kernel_clean
            ;;

        deep)
            deep_clean
            ;;

        all)
            deep_clean
            ;;

        help|-h|--help)
            usage
            ;;

        *)
            error "Unknown option: $1"
            usage
            exit 1
            ;;

    esac
}


# ------------------------------------------------------------
# Execute
# ------------------------------------------------------------

main "$@"
