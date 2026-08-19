#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# Build Script
#
# Flow:
#   Configuration
#       ↓
#   Kernel Configuration
#       ↓
#   Linux Kernel Build
#       ↓
#   Device Tree Build
#       ↓
#   Packet Filter Driver Build
#       ↓
#   Yocto Image Build
#       ↓
#   Deployable Image
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
# Project Configuration
# ------------------------------------------------------------

PROJECT_NAME="BeagleBone_AI-64–TI_TDA4VM"

BUILD_DIR="${PROJECT_ROOT}/build"
KERNEL_DIR="${PROJECT_ROOT}/kernel"
DRIVER_DIR="${KERNEL_DIR}/packet_filter"
CONFIG_DIR="${PROJECT_ROOT}/configs"
RULE_DIR="${PROJECT_ROOT}/configs/rules"

YOCTO_DIR="${PROJECT_ROOT}/yocto"


# ------------------------------------------------------------
# Build Configuration
# ------------------------------------------------------------

ARCH="${ARCH:-arm64}"

CROSS_COMPILE="${CROSS_COMPILE:-aarch64-linux-gnu-}"

BUILD_TYPE="${BUILD_TYPE:-debug}"


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
    echo "        ${PROJECT_NAME}"
    echo "        Linux / Yocto Build System"
    echo "============================================================"
    echo
}


# ------------------------------------------------------------
# Check Required Commands
# ------------------------------------------------------------

check_commands()
{
    info "Checking required build tools..."

    local commands=(
        make
        gcc
        git
    )

    for cmd in "${commands[@]}"
    do
        if ! command -v "${cmd}" >/dev/null 2>&1
        then
            error "Required command not found: ${cmd}"
            exit 1
        fi
    done

    success "Required build tools are available."
}


# ------------------------------------------------------------
# Check Cross Compiler
# ------------------------------------------------------------

check_cross_compiler()
{
    info "Checking ARM64 cross compiler..."

    if command -v "${CROSS_COMPILE}gcc" >/dev/null 2>&1
    then
        success "Cross compiler found: ${CROSS_COMPILE}gcc"
    else
        warning "ARM64 cross compiler not found."

        warning "Expected compiler:"
        warning "${CROSS_COMPILE}gcc"

        warning "Install the appropriate ARM64 cross compiler"
        warning "or provide CROSS_COMPILE manually."

        return 1
    fi
}


# ------------------------------------------------------------
# Create Build Directories
# ------------------------------------------------------------

create_directories()
{
    info "Creating build directories..."

    mkdir -p "${BUILD_DIR}"
    mkdir -p "${BUILD_DIR}/kernel"
    mkdir -p "${BUILD_DIR}/driver"
    mkdir -p "${BUILD_DIR}/dtb"
    mkdir -p "${BUILD_DIR}/logs"

    success "Build directories created."
}


# ------------------------------------------------------------
# Check Project Structure
# ------------------------------------------------------------

check_project_structure()
{
    info "Checking project structure..."

    local required_dirs=(
        "${KERNEL_DIR}"
        "${DRIVER_DIR}"
        "${CONFIG_DIR}"
        "${RULE_DIR}"
    )

    for dir in "${required_dirs[@]}"
    do
        if [ ! -d "${dir}" ]
        then
            error "Required directory missing:"
            error "${dir}"
            exit 1
        fi
    done

    success "Project structure is valid."
}


# ------------------------------------------------------------
# Check Packet Filter Sources
# ------------------------------------------------------------

check_driver_sources()
{
    info "Checking packet filter driver sources..."

    local sources=(
        "packet_filter.c"
        "packet_filter.h"
        "packet_parser.c"
        "packet_parser.h"
        "rule_engine.c"
        "rule_engine.h"
        "statistics.c"
        "statistics.h"
        "logging.c"
        "logging.h"
        "ioctl_defs.h"
    )

    for file in "${sources[@]}"
    do
        if [ ! -f "${DRIVER_DIR}/${file}" ]
        then
            error "Missing driver source: ${file}"
            exit 1
        fi
    done

    success "All packet filter sources are present."
}


# ------------------------------------------------------------
# Check Rule Configuration
# ------------------------------------------------------------

check_rules()
{
    info "Checking packet filter rule configuration..."

    local rules=(
        "whitelist.conf"
        "blacklist.conf"
        "monitoring.conf"
    )

    for file in "${rules[@]}"
    do
        if [ ! -f "${RULE_DIR}/${file}" ]
        then
            warning "Rule file missing: ${file}"
        else
            success "Found rule file: ${file}"
        fi
    done
}


# ------------------------------------------------------------
# Kernel Configuration
# ------------------------------------------------------------

configure_kernel()
{
    info "Configuring Linux kernel..."

    if [ ! -d "${KERNEL_DIR}" ]
    then
        error "Kernel source directory not found."
        return 1
    fi

    if [ -f "${CONFIG_DIR}/kernel/packet_filter_defconfig" ]
    then

        info "Using packet filter kernel configuration."

        cp \
            "${CONFIG_DIR}/kernel/packet_filter_defconfig" \
            "${KERNEL_DIR}/.config"

        make \
            -C "${KERNEL_DIR}" \
            ARCH="${ARCH}" \
            CROSS_COMPILE="${CROSS_COMPILE}" \
            olddefconfig

    else

        warning "packet_filter_defconfig not found."

        warning "Expected:"
        warning "${CONFIG_DIR}/kernel/packet_filter_defconfig"

    fi

    success "Kernel configuration completed."
}


# ------------------------------------------------------------
# Build Linux Kernel
# ------------------------------------------------------------

build_kernel()
{
    info "Building Linux kernel..."

    make \
        -C "${KERNEL_DIR}" \
        ARCH="${ARCH}" \
        CROSS_COMPILE="${CROSS_COMPILE}" \
        -j"$(nproc)" \
        Image \
        2>&1 | tee "${BUILD_DIR}/logs/kernel-build.log"

    success "Linux kernel build completed."
}


# ------------------------------------------------------------
# Build Device Tree
# ------------------------------------------------------------

build_device_tree()
{
    info "Building Device Tree..."

    make \
        -C "${KERNEL_DIR}" \
        ARCH="${ARCH}" \
        CROSS_COMPILE="${CROSS_COMPILE}" \
        -j"$(nproc)" \
        dtbs \
        2>&1 | tee "${BUILD_DIR}/logs/device-tree-build.log"

    success "Device Tree build completed."
}


# ------------------------------------------------------------
# Build Packet Filter Driver
# ------------------------------------------------------------

build_driver()
{
    info "Building packet filter kernel module..."

    if [ ! -f "${DRIVER_DIR}/Makefile" ]
    then
        error "Driver Makefile not found."
        error "${DRIVER_DIR}/Makefile"
        exit 1
    fi

    make \
        -C "${DRIVER_DIR}" \
        KERNEL_SRC="${KERNEL_DIR}" \
        ARCH="${ARCH}" \
        CROSS_COMPILE="${CROSS_COMPILE}" \
        2>&1 | tee "${BUILD_DIR}/logs/driver-build.log"

    success "Packet filter driver build completed."


    if [ -f "${DRIVER_DIR}/packet_filter.ko" ]
    then
        cp \
            "${DRIVER_DIR}/packet_filter.ko" \
            "${BUILD_DIR}/driver/"

        success "packet_filter.ko generated."
    else
        warning "packet_filter.ko was not generated."
    fi
}


# ------------------------------------------------------------
# Validate Driver Module
# ------------------------------------------------------------

validate_driver()
{
    info "Validating kernel module..."

    if [ -f "${BUILD_DIR}/driver/packet_filter.ko" ]
    then

        file \
            "${BUILD_DIR}/driver/packet_filter.ko"

        success "Kernel module validation completed."

    else

        warning "Kernel module not available for validation."

    fi
}


# ------------------------------------------------------------
# Build Yocto Image
# ------------------------------------------------------------

build_yocto()
{
    if [ ! -d "${YOCTO_DIR}" ]
    then
        warning "Yocto directory not found."
        warning "Skipping Yocto image build."
        return 0
    fi

    info "Starting Yocto build..."

    if [ -f "${YOCTO_DIR}/oe-init-build-env" ]
    then

        cd "${YOCTO_DIR}"

        source ./oe-init-build-env build

        bitbake core-image-minimal \
            2>&1 | tee \
            "${BUILD_DIR}/logs/yocto-build.log"

        cd "${PROJECT_ROOT}"

        success "Yocto image build completed."

    else

        warning "Yocto environment setup script not found."
        warning "Skipping Yocto image build."

    fi
}


# ------------------------------------------------------------
# Generate Build Summary
# ------------------------------------------------------------

build_summary()
{
    echo
    echo "============================================================"
    echo "                    BUILD SUMMARY"
    echo "============================================================"

    echo
    echo "Project:"
    echo "  ${PROJECT_NAME}"

    echo
    echo "Architecture:"
    echo "  ${ARCH}"

    echo
    echo "Cross Compiler:"
    echo "  ${CROSS_COMPILE}"

    echo
    echo "Build Directory:"
    echo "  ${BUILD_DIR}"

    echo
    echo "Kernel:"
    if [ -f "${KERNEL_DIR}/arch/${ARCH}/boot/Image" ]
    then
        echo "  ${KERNEL_DIR}/arch/${ARCH}/boot/Image"
    else
        echo "  Not generated"
    fi

    echo
    echo "Device Tree:"
    echo "  ${KERNEL_DIR}/arch/${ARCH}/boot/dts/"

    echo
    echo "Packet Filter:"
    if [ -f "${BUILD_DIR}/driver/packet_filter.ko" ]
    then
        echo "  ${BUILD_DIR}/driver/packet_filter.ko"
    else
        echo "  Not generated"
    fi

    echo
    echo "Rule Configuration:"
    echo "  ${RULE_DIR}/whitelist.conf"
    echo "  ${RULE_DIR}/blacklist.conf"
    echo "  ${RULE_DIR}/monitoring.conf"

    echo
    echo "Build Logs:"
    echo "  ${BUILD_DIR}/logs/"

    echo
    echo "============================================================"
}


# ------------------------------------------------------------
# Main Build Flow
# ------------------------------------------------------------

main()
{
    print_banner

    check_commands

    check_cross_compiler || true

    check_project_structure

    check_driver_sources

    check_rules

    create_directories

    configure_kernel

    build_kernel

    build_device_tree

    build_driver

    validate_driver

    build_yocto

    build_summary

    success "Complete build flow finished."
}


# ------------------------------------------------------------
# Execute
# ------------------------------------------------------------

main "$@"
