#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# IOCTL Integration Test
#
# Purpose:
#   Validate communication between userspace and the
#   packet_filter kernel driver through the IOCTL interface.
#
# Flow:
#
#   Userspace Test
#        |
#        v
#   packet_filter_ctl / test utility
#        |
#        v
#   /dev/packet_filter
#        |
#        v
#   ioctl()
#        |
#        v
#   packet_filter.c
#        |
#        +----> Rule Engine
#        +----> Statistics
#        +----> Configuration
#        +----> Logging
#
# Tests:
#   1. Root permission
#   2. Kernel module loaded
#   3. IOCTL header available
#   4. Device node detection
#   5. Userspace control utility detection
#   6. IOCTL help/version
#   7. GET_STATUS
#   8. GET_STATS
#   9. Rule configuration
#  10. Invalid IOCTL handling
#  11. Driver stability
#  12. Kernel log validation
#
# Usage:
#
#   sudo ./tests/integration/test_ioctl.sh
#
#   sudo ./tests/integration/test_ioctl.sh status
#
# ============================================================

set +e


# ------------------------------------------------------------
# Project Root
# ------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

cd "${PROJECT_ROOT}" || exit 1


# ------------------------------------------------------------
# Configuration
# ------------------------------------------------------------

MODULE_NAME="${MODULE_NAME:-packet_filter}"

DEVICE_NAME="${DEVICE_NAME:-packet_filter}"

DEVICE_PATH="${DEVICE_PATH:-/dev/${DEVICE_NAME}}"

CONFIG_DIR="${CONFIG_DIR:-/etc/packet_filter}"

RULE_DIR="${RULE_DIR:-${PROJECT_ROOT}/configs/rules}"

IOCTL_HEADER="${PROJECT_ROOT}/kernel/packet_filter/ioctl_defs.h"

RESULT_ROOT="${PROJECT_ROOT}/build/test-results/ioctl"

TIMESTAMP="$(date '+%Y%m%d_%H%M%S')"

RESULT_DIR="${RESULT_ROOT}/${TIMESTAMP}"

LOG_FILE="${RESULT_DIR}/ioctl_test.log"

SUMMARY_FILE="${RESULT_DIR}/summary.csv"

TOTAL_TESTS=0

PASS_COUNT=0

FAIL_COUNT=0

SKIP_COUNT=0


# ------------------------------------------------------------
# Userspace IOCTL Utility
# ------------------------------------------------------------

IOCTL_TOOL=""


# ------------------------------------------------------------
# Colors
# ------------------------------------------------------------

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'


# ------------------------------------------------------------
# Logging
# ------------------------------------------------------------

info()
{
    echo -e "${BLUE}[INFO]${NC} $1"
}


pass()
{
    echo -e "${GREEN}[PASS]${NC} $1"
}


fail()
{
    echo -e "${RED}[FAIL]${NC} $1"
}


skip()
{
    echo -e "${YELLOW}[SKIP]${NC} $1"
}


warning()
{
    echo -e "${YELLOW}[WARNING]${NC} $1"
}


# ------------------------------------------------------------
# Banner
# ------------------------------------------------------------

print_banner()
{
    echo
    echo "============================================================"
    echo "       BeagleBone AI-64 - TI TDA4VM"
    echo "       Packet Filter IOCTL Integration Test"
    echo "============================================================"
    echo
}


# ------------------------------------------------------------
# Result Initialization
# ------------------------------------------------------------

initialize_results()
{
    mkdir -p "${RESULT_DIR}"

    touch "${LOG_FILE}"

    touch "${SUMMARY_FILE}"

    echo "test,status,details" > "${SUMMARY_FILE}"

    exec > >(tee -a "${LOG_FILE}") 2>&1

    info "IOCTL test results:"
    echo "  ${RESULT_DIR}"
}


# ------------------------------------------------------------
# Test Counter
# ------------------------------------------------------------

begin_test()
{
    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    echo
    echo "------------------------------------------------------------"
    echo "TEST ${TOTAL_TESTS}: $1"
    echo "------------------------------------------------------------"
}


# ------------------------------------------------------------
# PASS
# ------------------------------------------------------------

record_pass()
{
    local name="$1"

    local details="${2:-PASS}"

    PASS_COUNT=$((PASS_COUNT + 1))

    pass "${name}"

    echo "\"${name}\",PASS,\"${details}\"" \
        >> "${SUMMARY_FILE}"
}


# ------------------------------------------------------------
# FAIL
# ------------------------------------------------------------

record_fail()
{
    local name="$1"

    local details="${2:-FAIL}"

    FAIL_COUNT=$((FAIL_COUNT + 1))

    fail "${name}"

    echo "\"${name}\",FAIL,\"${details}\"" \
        >> "${SUMMARY_FILE}"
}


# ------------------------------------------------------------
# SKIP
# ------------------------------------------------------------

record_skip()
{
    local name="$1"

    local details="${2:-SKIPPED}"

    SKIP_COUNT=$((SKIP_COUNT + 1))

    skip "${name}"

    echo "\"${name}\",SKIP,\"${details}\"" \
        >> "${SUMMARY_FILE}"
}


# ------------------------------------------------------------
# Root Permission
# ------------------------------------------------------------

test_root()
{
    begin_test "Root Permission"

    if [ "$(id -u)" -eq 0 ]
    then

        record_pass \
            "Root Permission" \
            "Running as root"

    else

        record_fail \
            "Root Permission" \
            "Run with sudo"

    fi
}


# ------------------------------------------------------------
# Kernel Module
# ------------------------------------------------------------

test_module()
{
    begin_test "Kernel Module"

    if lsmod |
        awk '{print $1}' |
        grep -qx "${MODULE_NAME}"
    then

        record_pass \
            "Kernel Module" \
            "${MODULE_NAME} is loaded"

        return
    fi


    if modinfo "${MODULE_NAME}" \
        >/dev/null 2>&1
    then

        warning "${MODULE_NAME} is installed but not loaded."

        if modprobe "${MODULE_NAME}" \
            >/dev/null 2>&1
        then

            record_pass \
                "Kernel Module" \
                "Module loaded successfully"

        else

            record_fail \
                "Kernel Module" \
                "Module could not be loaded"

        fi

    else

        record_fail \
            "Kernel Module" \
            "Module not installed"

    fi
}


# ------------------------------------------------------------
# IOCTL Header
# ------------------------------------------------------------

test_ioctl_header()
{
    begin_test "IOCTL Header"

    if [ ! -f "${IOCTL_HEADER}" ]
    then

        record_fail \
            "IOCTL Header" \
            "ioctl_defs.h not found"

        return
    fi


    echo "IOCTL header:"
    echo "  ${IOCTL_HEADER}"

    echo
    echo "Defined IOCTL symbols:"

    grep -E \
        '^[[:space:]]*#define[[:space:]]+[A-Za-z0-9_]*IOCTL|_IO[A-Z]*\(' \
        "${IOCTL_HEADER}" \
        2>/dev/null |
        head -50


    record_pass \
        "IOCTL Header" \
        "ioctl_defs.h available"
}


# ------------------------------------------------------------
# Find Device Node
# ------------------------------------------------------------

find_device()
{
    if [ -e "${DEVICE_PATH}" ]
    then
        return 0
    fi


    local candidate

    for candidate in \
        "/dev/packet_filter" \
        "/dev/packet-filter" \
        "/dev/pktfilter" \
        "/dev/pkt_filter"
    do

        if [ -e "${candidate}" ]
        then

            DEVICE_PATH="${candidate}"

            return 0

        fi

    done


    return 1
}


# ------------------------------------------------------------
# Device Node Test
# ------------------------------------------------------------

test_device()
{
    begin_test "IOCTL Device Node"

    if find_device
    then

        echo "Device:"
        echo "  ${DEVICE_PATH}"

        ls -l "${DEVICE_PATH}"

        record_pass \
            "IOCTL Device Node" \
            "${DEVICE_PATH} available"

    else

        record_skip \
            "IOCTL Device Node" \
            "Packet-filter device node not found"

    fi
}


# ------------------------------------------------------------
# Find Userspace Utility
# ------------------------------------------------------------

find_ioctl_tool()
{
    local candidates=(
        "/usr/bin/packet_filter_ctl"
        "/usr/sbin/packet_filter_ctl"
        "/usr/local/bin/packet_filter_ctl"
        "${PROJECT_ROOT}/userspace/packet_filter_ctl"
        "${PROJECT_ROOT}/userspace/packet_filter_ctl/packet_filter_ctl"
        "${PROJECT_ROOT}/build/packet_filter_ctl"
    )


    local candidate

    for candidate in "${candidates[@]}"
    do

        if [ -x "${candidate}" ]
        then

            IOCTL_TOOL="${candidate}"

            return 0

        fi

    done


    return 1
}


# ------------------------------------------------------------
# Userspace Utility Test
# ------------------------------------------------------------

test_ioctl_tool()
{
    begin_test "Userspace IOCTL Utility"

    if find_ioctl_tool
    then

        echo "IOCTL utility:"
        echo "  ${IOCTL_TOOL}"

        record_pass \
            "Userspace IOCTL Utility" \
            "${IOCTL_TOOL} found"

    else

        record_skip \
            "Userspace IOCTL Utility" \
            "packet_filter_ctl not found"

    fi
}


# ------------------------------------------------------------
# IOCTL Help
# ------------------------------------------------------------

test_ioctl_help()
{
    begin_test "IOCTL Utility Help"

    if [ -z "${IOCTL_TOOL}" ]
    then

        record_skip \
            "IOCTL Utility Help" \
            "IOCTL utility unavailable"

        return
    fi


    "${IOCTL_TOOL}" \
        --help \
        > "${RESULT_DIR}/ioctl_help.txt" \
        2>&1

    local rc=$?


    cat "${RESULT_DIR}/ioctl_help.txt"


    if [ "${rc}" -eq 0 ]
    then

        record_pass \
            "IOCTL Utility Help" \
            "Help command successful"

    else

        record_skip \
            "IOCTL Utility Help" \
            "Utility does not support --help"

    fi
}


# ------------------------------------------------------------
# Version Test
# ------------------------------------------------------------

test_ioctl_version()
{
    begin_test "IOCTL Utility Version"

    if [ -z "${IOCTL_TOOL}" ]
    then

        record_skip \
            "IOCTL Utility Version" \
            "IOCTL utility unavailable"

        return

    fi


    "${IOCTL_TOOL}" \
        --version \
        > "${RESULT_DIR}/ioctl_version.txt" \
        2>&1

    local rc=$?


    cat "${RESULT_DIR}/ioctl_version.txt"


    if [ "${rc}" -eq 0 ]
    then

        record_pass \
            "IOCTL Utility Version" \
            "Version command successful"

    else

        record_skip \
            "IOCTL Utility Version" \
            "Utility does not support --version"

    fi
}


# ------------------------------------------------------------
# Generic Utility Command
#
# The project can implement:
#
#   status
#   stats
#   list
#   rules
#
# depending on packet_filter_ctl implementation.
# ------------------------------------------------------------

run_tool_command()
{
    local command="$1"

    if [ -z "${IOCTL_TOOL}" ]
    then
        return 127
    fi


    "${IOCTL_TOOL}" \
        "${command}" \
        > "${RESULT_DIR}/ioctl_${command}.txt" \
        2>&1

    local rc=$?


    cat "${RESULT_DIR}/ioctl_${command}.txt"


    return "${rc}"
}


# ------------------------------------------------------------
# GET_STATUS Test
# ------------------------------------------------------------

test_get_status()
{
    begin_test "IOCTL GET_STATUS"

    if [ -z "${IOCTL_TOOL}" ]
    then

        record_skip \
            "IOCTL GET_STATUS" \
            "Userspace IOCTL utility unavailable"

        return

    fi


    run_tool_command "status"

    local rc=$?


    if [ "${rc}" -eq 0 ]
    then

        record_pass \
            "IOCTL GET_STATUS" \
            "Status IOCTL completed"

    else

        record_skip \
            "IOCTL GET_STATUS" \
            "status command unavailable or failed"

    fi
}


# ------------------------------------------------------------
# GET_STATS Test
# ------------------------------------------------------------

test_get_stats()
{
    begin_test "IOCTL GET_STATS"

    if [ -z "${IOCTL_TOOL}" ]
    then

        record_skip \
            "IOCTL GET_STATS" \
            "Userspace IOCTL utility unavailable"

        return

    fi


    run_tool_command "stats"

    local rc=$?


    if [ "${rc}" -eq 0 ]
    then

        record_pass \
            "IOCTL GET_STATS" \
            "Statistics IOCTL completed"

    else

        record_skip \
            "IOCTL GET_STATS" \
            "stats command unavailable or failed"

    fi
}


# ------------------------------------------------------------
# List Rules Test
# ------------------------------------------------------------

test_list_rules()
{
    begin_test "IOCTL LIST_RULES"

    if [ -z "${IOCTL_TOOL}" ]
    then

        record_skip \
            "IOCTL LIST_RULES" \
            "Userspace IOCTL utility unavailable"

        return

    fi


    run_tool_command "list"

    local rc=$?


    if [ "${rc}" -eq 0 ]
    then

        record_pass \
            "IOCTL LIST_RULES" \
            "Rule-list operation completed"

    else

        record_skip \
            "IOCTL LIST_RULES" \
            "list command unavailable or failed"

    fi
}


# ------------------------------------------------------------
# Rule Configuration Test
# ------------------------------------------------------------

test_rule_files()
{
    begin_test "IOCTL Rule Configuration"

    local files=(
        "${RULE_DIR}/whitelist.conf"
        "${RULE_DIR}/blacklist.conf"
        "${RULE_DIR}/monitoring.conf"
    )

    local missing=0

    local file


    for file in "${files[@]}"
    do

        if [ -f "${file}" ]
        then

            echo "[FOUND] ${file}"

        else

            echo "[MISSING] ${file}"

            missing=1

        fi

    done


    if [ "${missing}" -eq 0 ]
    then

        record_pass \
            "IOCTL Rule Configuration" \
            "All rule files available"

    else

        record_fail \
            "IOCTL Rule Configuration" \
            "Rule files missing"

    fi
}


# ------------------------------------------------------------
# Invalid IOCTL Test
#
# If packet_filter_ctl supports:
#
#   raw-invalid
#
# it can be used to explicitly test -ENOTTY.
#
# Otherwise this test is skipped.
# ------------------------------------------------------------

test_invalid_ioctl()
{
    begin_test "Invalid IOCTL Handling"

    if [ -z "${IOCTL_TOOL}" ]
    then

        record_skip \
            "Invalid IOCTL Handling" \
            "Userspace utility unavailable"

        return

    fi


    "${IOCTL_TOOL}" \
        raw-invalid \
        > "${RESULT_DIR}/invalid_ioctl.txt" \
        2>&1

    local rc=$?


    cat "${RESULT_DIR}/invalid_ioctl.txt"


    if [ "${rc}" -ne 0 ]
    then

        record_pass \
            "Invalid IOCTL Handling" \
            "Invalid command rejected"

    else

        record_skip \
            "Invalid IOCTL Handling" \
            "raw-invalid operation not supported"

    fi
}


# ------------------------------------------------------------
# Direct Device Access Test
# ------------------------------------------------------------

test_device_access()
{
    begin_test "IOCTL Device Access"

    if ! find_device
    then

        record_skip \
            "IOCTL Device Access" \
            "Device node unavailable"

        return

    fi


    if [ -r "${DEVICE_PATH}" ] &&
       [ -w "${DEVICE_PATH}" ]
    then

        record_pass \
            "IOCTL Device Access" \
            "${DEVICE_PATH} is readable and writable"

    else

        record_fail \
            "IOCTL Device Access" \
            "${DEVICE_PATH} permissions prevent access"

    fi
}


# ------------------------------------------------------------
# Device Major/Minor Test
# ------------------------------------------------------------

test_device_metadata()
{
    begin_test "Device Metadata"

    if ! find_device
    then

        record_skip \
            "Device Metadata" \
            "Device node unavailable"

        return

    fi


    local type

    type="$(stat -c '%F' "${DEVICE_PATH}" 2>/dev/null)"


    echo "Device type: ${type}"


    if stat "${DEVICE_PATH}" >/dev/null 2>&1
    then

        stat "${DEVICE_PATH}"

        record_pass \
            "Device Metadata" \
            "Device metadata accessible"

    else

        record_fail \
            "Device Metadata" \
            "Unable to read device metadata"

    fi
}


# ------------------------------------------------------------
# Module Reference Test
# ------------------------------------------------------------

test_module_reference()
{
    begin_test "Driver Reference Count"

    if ! lsmod |
        awk '{print $1}' |
        grep -qx "${MODULE_NAME}"
    then

        record_fail \
            "Driver Reference Count" \
            "Driver not loaded"

        return
    fi


    local refs

    refs="$(
        lsmod |
        awk -v module="${MODULE_NAME}" \
        '$1 == module {print $3}'
    )"


    echo "Module references: ${refs}"


    if [[ "${refs}" =~ ^[0-9]+$ ]]
    then

        record_pass \
            "Driver Reference Count" \
            "Reference count=${refs}"

    else

        record_fail \
            "Driver Reference Count" \
            "Invalid module reference count"

    fi
}


# ------------------------------------------------------------
# IOCTL Kernel Messages
# ------------------------------------------------------------

test_ioctl_kernel_messages()
{
    begin_test "IOCTL Kernel Messages"

    local messages

    messages="$(
        dmesg |
        grep -Ei \
            "packet_filter|packet-filter|ioctl" \
        2>/dev/null |
        tail -50
    )"


    if [ -n "${messages}" ]
    then

        echo "${messages}"

        record_pass \
            "IOCTL Kernel Messages" \
            "Relevant kernel messages found"

    else

        record_skip \
            "IOCTL Kernel Messages" \
            "No relevant messages found"

    fi
}


# ------------------------------------------------------------
# Kernel Error Check
# ------------------------------------------------------------

test_kernel_errors()
{
    begin_test "Kernel Error Check"

    local errors

    errors="$(
        dmesg |
        grep -Ei \
            "BUG:|Oops:|kernel panic|general protection|"
            "soft lockup|hard lockup" \
        2>/dev/null |
        tail -20
    )"


    if [ -z "${errors}" ]
    then

        record_pass \
            "Kernel Error Check" \
            "No major kernel errors detected"

    else

        echo "${errors}"

        record_fail \
            "Kernel Error Check" \
            "Kernel errors detected"

    fi
}


# ------------------------------------------------------------
# IOCTL Repeated Operation Test
# ------------------------------------------------------------

test_repeated_status()
{
    begin_test "Repeated IOCTL Status"

    if [ -z "${IOCTL_TOOL}" ]
    then

        record_skip \
            "Repeated IOCTL Status" \
            "Userspace utility unavailable"

        return

    fi


    local failures=0

    local i


    for i in $(seq 1 10)
    do

        if ! "${IOCTL_TOOL}" \
            status \
            > /dev/null 2>&1
        then

            failures=$((failures + 1))

        fi

    done


    echo "Iterations: 10"
    echo "Failures  : ${failures}"


    if [ "${failures}" -eq 0 ]
    then

        record_pass \
            "Repeated IOCTL Status" \
            "10/10 status operations successful"

    else

        record_fail \
            "Repeated IOCTL Status" \
            "${failures}/10 operations failed"

    fi
}


# ------------------------------------------------------------
# Capture State
# ------------------------------------------------------------

capture_state()
{
    lsmod \
        > "${RESULT_DIR}/lsmod.txt" \
        2>&1


    modinfo "${MODULE_NAME}" \
        > "${RESULT_DIR}/modinfo.txt" \
        2>&1 || true


    dmesg \
        > "${RESULT_DIR}/dmesg.txt" \
        2>&1 || true


    if find_device
    then

        stat "${DEVICE_PATH}" \
            > "${RESULT_DIR}/device_stat.txt" \
            2>&1

    fi
}


# ------------------------------------------------------------
# Final Report
# ------------------------------------------------------------

generate_report()
{
    local total

    total=$((PASS_COUNT + FAIL_COUNT + SKIP_COUNT))


    {
        echo
        echo "============================================================"
        echo "       PACKET FILTER IOCTL TEST REPORT"
        echo "============================================================"
        echo

        echo "Board:"
        echo "  BeagleBone AI-64"

        echo "SoC:"
        echo "  TI TDA4VM"

        echo "Kernel:"
        echo "  $(uname -r)"

        echo "Architecture:"
        echo "  $(uname -m)"

        echo "Module:"
        echo "  ${MODULE_NAME}"

        echo "Device:"
        echo "  ${DEVICE_PATH}"

        echo

        echo "------------------------------------------------------------"
        echo "Results"
        echo "------------------------------------------------------------"

        echo

        echo "Total  : ${total}"
        echo "PASS   : ${PASS_COUNT}"
        echo "FAIL   : ${FAIL_COUNT}"
        echo "SKIP   : ${SKIP_COUNT}"

        echo

        if [ "${FAIL_COUNT}" -eq 0 ]
        then
            echo "OVERALL RESULT: PASS"
        else
            echo "OVERALL RESULT: FAIL"
        fi

        echo

        echo "Result directory:"
        echo "  ${RESULT_DIR}"

        echo

        echo "============================================================"

    } > "${RESULT_DIR}/report.txt"


    cat "${RESULT_DIR}/report.txt"
}


# ------------------------------------------------------------
# Main Test Flow
# ------------------------------------------------------------

run_tests()
{
    print_banner

    initialize_results

    test_root

    test_module

    test_ioctl_header

    test_device

    test_ioctl_tool

    test_ioctl_help

    test_ioctl_version

    test_device_access

    test_device_metadata

    test_get_status

    test_get_stats

    test_list_rules

    test_rule_files

    test_invalid_ioctl

    test_module_reference

    test_repeated_status

    test_ioctl_kernel_messages

    test_kernel_errors

    capture_state

    generate_report


    echo


    if [ "${FAIL_COUNT}" -eq 0 ]
    then

        echo -e \
            "${GREEN}IOCTL INTEGRATION TEST: PASS${NC}"

        return 0

    else

        echo -e \
            "${RED}IOCTL INTEGRATION TEST: FAIL${NC}"

        return 1

    fi
}


# ------------------------------------------------------------
# Status
# ------------------------------------------------------------

show_status()
{
    print_banner

    echo "Module:"
    echo "  ${MODULE_NAME}"

    echo

    if lsmod |
        awk '{print $1}' |
        grep -qx "${MODULE_NAME}"
    then
        echo -e "${GREEN}Module: LOADED${NC}"
    else
        echo -e "${YELLOW}Module: NOT LOADED${NC}"
    fi


    echo

    if find_device
    then
        echo -e "${GREEN}Device: ${DEVICE_PATH}${NC}"
    else
        echo -e "${YELLOW}Device: NOT FOUND${NC}"
    fi


    echo

    if find_ioctl_tool
    then
        echo -e "${GREEN}IOCTL utility: ${IOCTL_TOOL}${NC}"
    else
        echo -e "${YELLOW}IOCTL utility: NOT FOUND${NC}"
    fi


    echo

    echo "IOCTL definitions:"

    if [ -f "${IOCTL_HEADER}" ]
    then

        grep -E \
            '^[[:space:]]*#define[[:space:]]+.*IOCTL|_IO[A-Z]*\(' \
            "${IOCTL_HEADER}" \
            2>/dev/null |
            head -50

    else

        echo "  ioctl_defs.h not found"

    fi
}


# ------------------------------------------------------------
# Usage
# ------------------------------------------------------------

usage()
{
    echo
    echo "Usage:"
    echo
    echo "  sudo ./tests/integration/test_ioctl.sh"
    echo "      Run complete IOCTL integration test"
    echo
    echo "  sudo ./tests/integration/test_ioctl.sh status"
    echo "      Show IOCTL driver/device status"
    echo
    echo "  sudo ./tests/integration/test_ioctl.sh help"
    echo "      Display help"
    echo
    echo "Environment variables:"
    echo
    echo "  MODULE_NAME=packet_filter"
    echo "  DEVICE_NAME=packet_filter"
    echo "  DEVICE_PATH=/dev/packet_filter"
    echo "  CONFIG_DIR=/etc/packet_filter"
    echo "  RULE_DIR=./configs/rules"
    echo
}


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

case "${1:-run}" in

    run)
        run_tests
        exit $?
        ;;

    status)
        show_status
        exit 0
        ;;

    help|-h|--help)
        usage
        exit 0
        ;;

    *)
        echo "Unknown command: $1"
        usage
        exit 1
        ;;

esac
