#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# Driver Load / Unload Script
#
# Responsibilities:
#   - Check packet_filter.ko
#   - Load kernel module
#   - Verify module
#   - Create / verify runtime state
#   - Show kernel logs
#   - Unload kernel module
#   - Reload kernel module
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
# Configuration
# ------------------------------------------------------------

MODULE_NAME="packet_filter"

MODULE_FILE="${MODULE_FILE:-${PROJECT_ROOT}/build/driver/${MODULE_NAME}.ko}"

MODULE_INSTALL_PATH="/lib/modules/$(uname -r)/extra/${MODULE_NAME}.ko"

CONFIG_FILE="/etc/packet_filter/packet_filter.conf"

RULE_DIR="/etc/packet_filter/rules"

LOG_FILE="/var/log/packet_filter/driver.log"


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
    echo "       Packet Filter Driver Manager"
    echo "============================================================"
    echo
}


# ------------------------------------------------------------
# Root Check
# ------------------------------------------------------------

check_root()
{
    if [ "$(id -u)" -ne 0 ]
    then
        error "Root privileges are required."

        echo
        echo "Run:"
        echo
        echo "  sudo ./scripts/load_driver.sh load"
        echo

        exit 1
    fi
}


# ------------------------------------------------------------
# Check Module State
# ------------------------------------------------------------

is_loaded()
{
    if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
    then
        return 0
    fi

    return 1
}


# ------------------------------------------------------------
# Check Module File
# ------------------------------------------------------------

check_module_file()
{
    info "Checking packet filter kernel module..."

    if [ -f "${MODULE_FILE}" ]
    then
        success "Found:"
        echo "  ${MODULE_FILE}"
        return 0
    fi


    if [ -f "${MODULE_INSTALL_PATH}" ]
    then
        MODULE_FILE="${MODULE_INSTALL_PATH}"

        success "Found installed module:"
        echo "  ${MODULE_FILE}"

        return 0
    fi


    error "packet_filter.ko not found."

    echo
    echo "Expected:"
    echo
    echo "  ${PROJECT_ROOT}/build/driver/packet_filter.ko"
    echo
    echo "Build the project first:"
    echo
    echo "  ./scripts/build.sh"
    echo

    exit 1
}


# ------------------------------------------------------------
# Check Module Information
# ------------------------------------------------------------

show_module_info()
{
    info "Kernel module information..."

    if command -v modinfo >/dev/null 2>&1
    then
        modinfo "${MODULE_FILE}" 2>/dev/null || true
    else
        warning "modinfo is not available."
    fi
}


# ------------------------------------------------------------
# Check Dependencies
# ------------------------------------------------------------

check_dependencies()
{
    info "Checking module dependencies..."

    if command -v modprobe >/dev/null 2>&1
    then
        modprobe --show-depends "${MODULE_NAME}" 2>/dev/null || true
    else
        warning "modprobe not available."
    fi
}


# ------------------------------------------------------------
# Create Runtime Directories
# ------------------------------------------------------------

create_runtime_directories()
{
    mkdir -p "$(dirname "${LOG_FILE}")"

    mkdir -p "/run/packet_filter"
}


# ------------------------------------------------------------
# Load Using insmod
# ------------------------------------------------------------

load_with_insmod()
{
    info "Loading ${MODULE_NAME} using insmod..."

    if is_loaded
    then
        warning "${MODULE_NAME} is already loaded."
        return 0
    fi


    if ! command -v insmod >/dev/null 2>&1
    then
        error "insmod command not found."
        exit 1
    fi


    insmod "${MODULE_FILE}"


    if is_loaded
    then
        success "${MODULE_NAME} loaded successfully."
    else
        error "Module load failed."
        exit 1
    fi
}


# ------------------------------------------------------------
# Load Using modprobe
# ------------------------------------------------------------

load_with_modprobe()
{
    info "Loading ${MODULE_NAME} using modprobe..."

    if is_loaded
    then
        warning "${MODULE_NAME} is already loaded."
        return 0
    fi


    if ! command -v modprobe >/dev/null 2>&1
    then
        warning "modprobe not available."
        warning "Falling back to insmod."

        load_with_insmod
        return 0
    fi


    depmod -a 2>/dev/null || true


    if modprobe "${MODULE_NAME}"
    then

        if is_loaded
        then
            success "${MODULE_NAME} loaded successfully."
        else
            error "modprobe completed but module is not visible."
            exit 1
        fi

    else

        error "modprobe failed."
        exit 1

    fi
}


# ------------------------------------------------------------
# Load Driver
# ------------------------------------------------------------

load_driver()
{
    print_banner

    check_root

    check_module_file

    create_runtime_directories

    show_module_info

    check_dependencies

    echo

    if [ -f "/lib/modules/$(uname -r)/extra/${MODULE_NAME}.ko" ]
    then
        load_with_modprobe
    else
        load_with_insmod
    fi


    echo

    show_status

    save_driver_log

    echo

    show_recent_logs

    echo

    success "Packet filter driver load operation completed."
}


# ------------------------------------------------------------
# Unload Driver
# ------------------------------------------------------------

unload_driver()
{
    print_banner

    check_root

    info "Checking driver state..."

    if ! is_loaded
    then
        warning "${MODULE_NAME} is not loaded."
        return 0
    fi


    info "Unloading ${MODULE_NAME}..."

    if command -v modprobe >/dev/null 2>&1
    then

        if modprobe -r "${MODULE_NAME}"
        then
            success "${MODULE_NAME} unloaded successfully."
        else
            error "Unable to unload ${MODULE_NAME}."
            echo
            echo "The module may be in use."
            echo
            echo "Check:"
            echo
            echo "  lsmod | grep ${MODULE_NAME}"
            echo "  dmesg | tail -50"
            echo
            exit 1
        fi

    else

        if rmmod "${MODULE_NAME}"
        then
            success "${MODULE_NAME} unloaded successfully."
        else
            error "Unable to unload ${MODULE_NAME}."
            exit 1
        fi

    fi


    echo

    show_status

    echo

    success "Packet filter driver unload operation completed."
}


# ------------------------------------------------------------
# Reload Driver
# ------------------------------------------------------------

reload_driver()
{
    print_banner

    check_root

    info "Reloading packet filter driver..."

    if is_loaded
    then

        info "Removing existing module..."

        modprobe -r "${MODULE_NAME}" 2>/dev/null || \
            rmmod "${MODULE_NAME}"

        success "Existing module removed."

    else

        info "Module is not currently loaded."

    fi


    echo

    check_module_file

    create_runtime_directories

    echo

    load_with_insmod

    echo

    show_status

    save_driver_log

    echo

    show_recent_logs

    echo

    success "Packet filter driver reload completed."
}


# ------------------------------------------------------------
# Driver Status
# ------------------------------------------------------------

show_status()
{
    echo
    echo "============================================================"
    echo "                  DRIVER STATUS"
    echo "============================================================"

    echo
    echo "Module:"
    echo "  ${MODULE_NAME}"

    echo
    echo "Kernel:"
    uname -r

    echo
    echo "Architecture:"
    uname -m

    echo
    echo "Status:"

    if is_loaded
    then
        echo "  [LOADED]"
    else
        echo "  [NOT LOADED]"
    fi


    echo
    echo "Module Details:"

    if is_loaded
    then

        lsmod | head -1

        lsmod | grep "^${MODULE_NAME}" || true

    else

        echo "  Module is not loaded."

    fi


    echo
    echo "============================================================"
}


# ------------------------------------------------------------
# Show Driver Logs
# ------------------------------------------------------------

show_logs()
{
    print_banner

    info "Recent packet-filter kernel messages..."

    echo

    if command -v dmesg >/dev/null 2>&1
    then

        dmesg | grep -i \
            -E "packet_filter|packet-filter|packet filter" \
            | tail -100 || true

    else

        warning "dmesg command not available."

    fi
}


# ------------------------------------------------------------
# Show Recent Kernel Logs
# ------------------------------------------------------------

show_recent_logs()
{
    info "Recent driver kernel messages:"

    dmesg | tail -30 || true
}


# ------------------------------------------------------------
# Save Driver Log
# ------------------------------------------------------------

save_driver_log()
{
    create_runtime_directories

    {
        echo
        echo "============================================================"
        echo "Packet Filter Driver State"
        echo "Date: $(date)"
        echo "============================================================"

        echo
        echo "Kernel:"
        uname -a

        echo
        echo "Module:"
        lsmod | grep "^${MODULE_NAME}" || \
            echo "Module not loaded."

        echo
        echo "Kernel Messages:"
        dmesg | grep -i \
            -E "packet_filter|packet-filter|packet filter" \
            | tail -100 || true

    } >> "${LOG_FILE}"

    success "Driver log saved:"
    echo "  ${LOG_FILE}"
}


# ------------------------------------------------------------
# Check Driver Interface
# ------------------------------------------------------------

check_interface()
{
    print_banner

    info "Checking packet-filter kernel interfaces..."

    echo

    echo "Loaded module:"
    lsmod | grep "^${MODULE_NAME}" || \
        echo "  Module not loaded."


    echo

    echo "Proc entries:"
    find /proc \
        -maxdepth 2 \
        -iname "*packet*filter*" \
        2>/dev/null || true


    echo

    echo "Sysfs entries:"
    find /sys \
        -maxdepth 4 \
        -iname "*packet*filter*" \
        2>/dev/null || true


    echo

    echo "Device nodes:"
    find /dev \
        -maxdepth 1 \
        -iname "*packet*filter*" \
        2>/dev/null || true


    echo

    echo "============================================================"
}


# ------------------------------------------------------------
# Test Module Load
# ------------------------------------------------------------

test_driver()
{
    print_banner

    check_root

    info "Testing packet-filter driver..."

    echo

    if is_loaded
    then
        warning "Module is already loaded."
    else
        check_module_file
        load_with_insmod
    fi


    echo

    if is_loaded
    then
        success "Driver test: PASS"
    else
        error "Driver test: FAIL"
        exit 1
    fi


    echo

    show_logs
}


# ------------------------------------------------------------
# Usage
# ------------------------------------------------------------

usage()
{
    echo
    echo "Usage:"
    echo
    echo "  sudo ./scripts/load_driver.sh load"
    echo "  sudo ./scripts/load_driver.sh unload"
    echo "  sudo ./scripts/load_driver.sh reload"
    echo "  sudo ./scripts/load_driver.sh status"
    echo "  sudo ./scripts/load_driver.sh logs"
    echo "  sudo ./scripts/load_driver.sh interface"
    echo "  sudo ./scripts/load_driver.sh test"
    echo "  sudo ./scripts/load_driver.sh info"
    echo "  ./scripts/load_driver.sh help"
    echo
    echo "Commands:"
    echo
    echo "  load        Load packet_filter kernel module"
    echo "  unload      Remove packet_filter kernel module"
    echo "  reload      Remove and reload the module"
    echo "  status      Display driver status"
    echo "  logs        Display packet-filter kernel logs"
    echo "  interface   Check driver interfaces"
    echo "  test        Load and test the driver"
    echo "  info        Display module information"
    echo "  help        Display this help"
    echo
    echo "Override module location:"
    echo
    echo "  MODULE_FILE=/path/packet_filter.ko \\"
    echo "      sudo ./scripts/load_driver.sh load"
    echo
}


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

main()
{
    case "${1:-status}" in

        load)
            load_driver
            ;;

        unload)
            unload_driver
            ;;

        reload)
            reload_driver
            ;;

        status)
            print_banner
            show_status
            ;;

        logs)
            show_logs
            ;;

        interface)
            check_interface
            ;;

        test)
            test_driver
            ;;

        info)
            print_banner
            check_module_file
            show_module_info
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
