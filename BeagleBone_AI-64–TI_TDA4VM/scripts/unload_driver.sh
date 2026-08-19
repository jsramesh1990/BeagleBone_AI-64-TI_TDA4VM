#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# Driver Unload Script
#
# Purpose:
#   - Stop packet-filter activity
#   - Remove packet_filter kernel module
#   - Verify module removal
#   - Capture unload-related kernel messages
#
# Usage:
#   sudo ./scripts/unload_driver.sh
#   sudo ./scripts/unload_driver.sh force
#   sudo ./scripts/unload_driver.sh status
#
# ============================================================

set -u


# ------------------------------------------------------------
# Project Root
# ------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_ROOT}"


# ------------------------------------------------------------
# Configuration
# ------------------------------------------------------------

MODULE_NAME="${MODULE_NAME:-packet_filter}"

LOG_DIR="${PROJECT_ROOT}/build/logs"

TIMESTAMP="$(date '+%Y%m%d_%H%M%S')"

LOG_FILE="${LOG_DIR}/unload_driver_${TIMESTAMP}.log"


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
    echo "       Packet Filter Driver Unload"
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
        echo "  sudo ./scripts/unload_driver.sh"
        echo
        exit 1
    fi
}


# ------------------------------------------------------------
# Create Log Directory
# ------------------------------------------------------------

create_log_directory()
{
    mkdir -p "${LOG_DIR}"

    touch "${LOG_FILE}"
}


# ------------------------------------------------------------
# Check Module
# ------------------------------------------------------------

module_loaded()
{
    lsmod |
        awk '{print $1}' |
        grep -qx "${MODULE_NAME}"
}


# ------------------------------------------------------------
# Module Status
# ------------------------------------------------------------

show_status()
{
    print_banner

    info "Checking driver status..."

    echo
    echo "Module: ${MODULE_NAME}"
    echo

    if module_loaded
    then
        success "${MODULE_NAME} is currently loaded."

        echo
        echo "Module information:"
        modinfo "${MODULE_NAME}" 2>/dev/null || true

        echo
        echo "Loaded module:"
        lsmod | grep -E "^${MODULE_NAME}[[:space:]]" || true

        echo
        echo "Module references:"
        lsmod |
            awk -v module="${MODULE_NAME}" \
            '$1 == module {print $3}'

    else
        success "${MODULE_NAME} is not loaded."
    fi
}


# ------------------------------------------------------------
# Find Module Devices
# ------------------------------------------------------------

find_module_devices()
{
    info "Searching for packet-filter device nodes..."

    local found=0

    if [ -d "/dev" ]
    then
        while IFS= read -r device
        do
            if [ -n "${device}" ]
            then
                echo "  ${device}"
                found=1
            fi
        done < <(
            find /dev \
                -maxdepth 1 \
                \( \
                    -iname "*packet*filter*" \
                    -o \
                    -iname "*pktfilter*" \
                \) \
                2>/dev/null
        )
    fi

    if [ "${found}" -eq 0 ]
    then
        echo "  No packet-filter device node found."
    fi
}


# ------------------------------------------------------------
# Find Module Processes
# ------------------------------------------------------------

find_module_processes()
{
    info "Searching for processes using packet-filter resources..."

    local found=0

    local pattern

    for pattern in \
        "packet_filter" \
        "packet-filter" \
        "packet_filter_ctl"
    do

        if pgrep -af "${pattern}" 2>/dev/null
        then
            found=1
        fi

    done

    if [ "${found}" -eq 0 ]
    then
        echo "  No packet-filter userspace process detected."
    fi
}


# ------------------------------------------------------------
# Stop Userspace Service
# ------------------------------------------------------------

stop_service()
{
    info "Checking packet-filter service..."

    local services=(
        "packet-filter"
        "packet_filter"
        "packet-filter.service"
        "packet_filter.service"
    )

    local service

    for service in "${services[@]}"
    do
        if systemctl list-unit-files \
            2>/dev/null |
            grep -q "^${service}"
        then

            info "Stopping service: ${service}"

            systemctl stop "${service}" \
                >> "${LOG_FILE}" 2>&1 || true

        fi
    done
}


# ------------------------------------------------------------
# Remove Device Nodes
# ------------------------------------------------------------

remove_device_nodes()
{
    info "Checking packet-filter device nodes..."

    local device

    while IFS= read -r device
    do
        if [ -n "${device}" ]
        then
            warning "Device node still exists: ${device}"
        fi

    done < <(
        find /dev \
            -maxdepth 1 \
            \( \
                -iname "*packet*filter*" \
                -o \
                -iname "*pktfilter*" \
            \) \
            2>/dev/null
    )
}


# ------------------------------------------------------------
# Sync Filesystems
# ------------------------------------------------------------

sync_filesystems()
{
    info "Synchronizing filesystems..."

    sync

    success "Filesystem synchronization completed."
}


# ------------------------------------------------------------
# Capture Kernel Log Before
# ------------------------------------------------------------

capture_dmesg_before()
{
    info "Capturing kernel messages before unload..."

    dmesg > "${LOG_FILE}.dmesg_before" 2>/dev/null || true
}


# ------------------------------------------------------------
# Unload Module
# ------------------------------------------------------------

unload_module()
{
    info "Attempting to unload ${MODULE_NAME}..."

    if ! module_loaded
    then
        success "${MODULE_NAME} is already unloaded."
        return 0
    fi


    if modprobe -r "${MODULE_NAME}" \
        >> "${LOG_FILE}" 2>&1
    then

        success "${MODULE_NAME} unloaded successfully."

        return 0

    else

        warning "modprobe -r failed."

    fi


    info "Trying rmmod..."

    if rmmod "${MODULE_NAME}" \
        >> "${LOG_FILE}" 2>&1
    then

        success "${MODULE_NAME} unloaded using rmmod."

        return 0

    fi


    error "Unable to unload ${MODULE_NAME}."

    return 1
}


# ------------------------------------------------------------
# Force Unload
# ------------------------------------------------------------

force_unload()
{
    info "Attempting forced module removal..."

    if ! module_loaded
    then
        success "${MODULE_NAME} is already unloaded."
        return 0
    fi


    warning "Force mode requested."

    warning "Force removal should only be used during development/debugging."

    if rmmod -f "${MODULE_NAME}" \
        >> "${LOG_FILE}" 2>&1
    then

        success "Forced module removal completed."

        return 0

    fi


    error "Forced module removal failed."

    return 1
}


# ------------------------------------------------------------
# Check Module Dependencies
# ------------------------------------------------------------

check_dependencies()
{
    info "Checking module dependencies..."

    local dependencies

    dependencies="$(
        modinfo -F depends "${MODULE_NAME}" \
        2>/dev/null || true
    )"

    if [ -n "${dependencies}" ]
    then

        echo
        echo "Dependencies:"
        echo "${dependencies}"

    else

        echo "No module dependencies reported."

    fi
}


# ------------------------------------------------------------
# Verify Unload
# ------------------------------------------------------------

verify_unload()
{
    info "Verifying driver unload..."

    sleep 1

    if module_loaded
    then

        error "${MODULE_NAME} is STILL loaded."

        echo
        echo "Current module state:"
        lsmod |
            grep -E "^${MODULE_NAME}[[:space:]]" || true

        return 1

    else

        success "${MODULE_NAME} is no longer loaded."

        return 0

    fi
}


# ------------------------------------------------------------
# Capture Kernel Log After
# ------------------------------------------------------------

capture_dmesg_after()
{
    info "Capturing kernel messages after unload..."

    dmesg > "${LOG_FILE}.dmesg_after" 2>/dev/null || true
}


# ------------------------------------------------------------
# Show Unload Messages
# ------------------------------------------------------------

show_unload_messages()
{
    echo
    echo "============================================================"
    echo "PACKET FILTER KERNEL MESSAGES"
    echo "============================================================"
    echo

    dmesg |
        grep -i \
            -E "packet_filter|packet-filter|packet filter" \
        2>/dev/null |
        tail -30 || true

    echo
}


# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------

cleanup()
{
    sync_filesystems
}


# ------------------------------------------------------------
# Normal Unload
# ------------------------------------------------------------

normal_unload()
{
    print_banner

    check_root

    create_log_directory

    info "Unload log:"
    echo "  ${LOG_FILE}"
    echo

    capture_dmesg_before

    show_status

    check_dependencies

    find_module_devices

    find_module_processes

    stop_service

    sync_filesystems

    if ! unload_module
    then

        error "Normal driver unload failed."

        echo
        echo "Possible causes:"
        echo "  1. Driver is busy."
        echo "  2. Userspace application is using the driver."
        echo "  3. Network hook is still active."
        echo "  4. Another kernel module depends on packet_filter."
        echo "  5. Driver cleanup path returned an error."
        echo

        echo "Check:"
        echo
        echo "  lsmod"
        echo "  lsof /dev/<packet-filter-device>"
        echo "  dmesg | tail -100"
        echo

        capture_dmesg_after

        return 1
    fi


    if ! verify_unload
    then
        capture_dmesg_after
        return 1
    fi


    cleanup

    capture_dmesg_after

    show_unload_messages

    remove_device_nodes

    echo
    echo "============================================================"
    echo "DRIVER UNLOAD COMPLETE"
    echo "============================================================"
    echo

    success "Module ${MODULE_NAME} successfully unloaded."

    echo
    echo "Log:"
    echo "  ${LOG_FILE}"

    echo

    return 0
}


# ------------------------------------------------------------
# Force Unload
# ------------------------------------------------------------

force_unload_driver()
{
    print_banner

    check_root

    create_log_directory

    warning "FORCE UNLOAD MODE"
    echo

    capture_dmesg_before

    show_status

    find_module_devices

    find_module_processes

    stop_service

    sync_filesystems


    if ! force_unload
    then

        error "Forced unload failed."

        capture_dmesg_after

        return 1
    fi


    if ! verify_unload
    then

        capture_dmesg_after

        return 1
    fi


    cleanup

    capture_dmesg_after

    show_unload_messages

    echo
    echo "============================================================"
    echo "FORCE UNLOAD COMPLETE"
    echo "============================================================"
    echo

    success "${MODULE_NAME} has been removed."

    echo
    echo "Log:"
    echo "  ${LOG_FILE}"

    echo

    return 0
}


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

main()
{
    case "${1:-unload}" in

        unload)
            normal_unload
            ;;

        force)
            force_unload_driver
            ;;

        status)
            check_root
            create_log_directory
            show_status
            ;;

        help|-h|--help)
            echo
            echo "Usage:"
            echo
            echo "  sudo ./scripts/unload_driver.sh"
            echo "      Normal driver unload"
            echo
            echo "  sudo ./scripts/unload_driver.sh force"
            echo "      Force driver unload"
            echo
            echo "  sudo ./scripts/unload_driver.sh status"
            echo "      Display driver status"
            echo
            ;;

        *)
            error "Unknown command: $1"
            echo
            echo "Use:"
            echo "  sudo ./scripts/unload_driver.sh help"
            echo
            exit 1
            ;;

    esac
}


# ------------------------------------------------------------
# Execute
# ------------------------------------------------------------

main "$@"

exit $?
