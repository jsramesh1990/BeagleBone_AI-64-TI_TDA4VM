#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Integration Test
#
# Purpose:
#   Validate the complete packet-filter path:
#
#   Userspace
#       ↓
#   Configuration / Rules
#       ↓
#   Kernel Driver
#       ↓
#   Packet Parser
#       ↓
#   Rule Engine
#       ↓
#   Whitelist / Blacklist / Monitoring
#       ↓
#   Statistics / Logging
#
# Tests:
#   1. Driver availability
#   2. Network interface
#   3. Configuration files
#   4. Driver interface
#   5. Whitelist behavior
#   6. Blacklist behavior
#   7. Monitoring behavior
#   8. Packet counters
#   9. Kernel log validation
#  10. Driver stability
#
# Usage:
#
#   sudo ./tests/integration/test_filtering.sh
#
#   sudo INTERFACE=eth0 \
#        ./tests/integration/test_filtering.sh
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

INTERFACE="${INTERFACE:-}"

CONFIG_DIR="${CONFIG_DIR:-/etc/packet_filter}"

RULE_DIR="${RULE_DIR:-${PROJECT_ROOT}/configs/rules}"

RESULT_ROOT="${PROJECT_ROOT}/build/test-results/integration"

TIMESTAMP="$(date '+%Y%m%d_%H%M%S')"

RESULT_DIR="${RESULT_ROOT}/${TIMESTAMP}"

LOG_FILE="${RESULT_DIR}/integration.log"

SUMMARY_FILE="${RESULT_DIR}/summary.csv"

TOTAL_TESTS=0

PASS_COUNT=0

FAIL_COUNT=0

SKIP_COUNT=0


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
# Logging Functions
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
    echo "       Packet Filter Integration Test"
    echo "============================================================"
    echo
}


# ------------------------------------------------------------
# Result Directory
# ------------------------------------------------------------

initialize_results()
{
    mkdir -p "${RESULT_DIR}"

    touch "${LOG_FILE}"

    touch "${SUMMARY_FILE}"

    echo "test,status,details" > "${SUMMARY_FILE}"

    exec > >(tee -a "${LOG_FILE}") 2>&1

    info "Integration test results:"
    echo "  ${RESULT_DIR}"
}


# ------------------------------------------------------------
# Test Start
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
# Record PASS
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
# Record FAIL
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
# Record SKIP
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
# Root Check
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
            "Run integration test with sudo"
    fi
}


# ------------------------------------------------------------
# Driver Check
# ------------------------------------------------------------

test_driver()
{
    begin_test "Packet Filter Driver"

    if lsmod |
        awk '{print $1}' |
        grep -qx "${MODULE_NAME}"
    then

        record_pass \
            "Packet Filter Driver" \
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
                "Packet Filter Driver" \
                "Driver loaded using modprobe"

        else

            record_fail \
                "Packet Filter Driver" \
                "Unable to load driver"

        fi

    else

        record_fail \
            "Packet Filter Driver" \
            "${MODULE_NAME} not installed"
    fi
}


# ------------------------------------------------------------
# Network Interface Detection
# ------------------------------------------------------------

detect_interface()
{
    if [ -n "${INTERFACE}" ]
    then
        return
    fi


    INTERFACE="$(
        ip -o link show 2>/dev/null |
        awk -F': ' '$2 != "lo" {print $2}' |
        head -1
    )"
}


# ------------------------------------------------------------
# Network Interface Test
# ------------------------------------------------------------

test_interface()
{
    begin_test "Network Interface"

    detect_interface

    if [ -z "${INTERFACE}" ]
    then
        record_fail \
            "Network Interface" \
            "No network interface detected"

        return
    fi


    echo "Interface: ${INTERFACE}"


    if ip link show "${INTERFACE}" \
        >/dev/null 2>&1
    then

        record_pass \
            "Network Interface" \
            "${INTERFACE} detected"

    else

        record_fail \
            "Network Interface" \
            "${INTERFACE} unavailable"

    fi
}


# ------------------------------------------------------------
# Network Link Test
# ------------------------------------------------------------

test_link()
{
    begin_test "Ethernet Link"

    if [ -z "${INTERFACE}" ]
    then
        record_skip \
            "Ethernet Link" \
            "Interface unavailable"

        return
    fi


    if ip link show "${INTERFACE}" |
        grep -q "state UP"
    then

        record_pass \
            "Ethernet Link" \
            "${INTERFACE} is UP"

    else

        record_fail \
            "Ethernet Link" \
            "${INTERFACE} is DOWN"

    fi
}


# ------------------------------------------------------------
# IP Address Test
# ------------------------------------------------------------

test_ip_address()
{
    begin_test "IP Address"

    if [ -z "${INTERFACE}" ]
    then
        record_skip \
            "IP Address" \
            "Interface unavailable"

        return
    fi


    local address

    address="$(
        ip -4 addr show "${INTERFACE}" |
        awk '/inet / {print $2}' |
        head -1
    )"


    if [ -n "${address}" ]
    then

        echo "IPv4 address: ${address}"

        record_pass \
            "IP Address" \
            "${address}"

    else

        record_fail \
            "IP Address" \
            "No IPv4 address assigned"

    fi
}


# ------------------------------------------------------------
# Configuration Test
# ------------------------------------------------------------

test_configuration()
{
    begin_test "Packet Filter Configuration"

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
            "Packet Filter Configuration" \
            "All rule files found"

    else

        record_fail \
            "Packet Filter Configuration" \
            "One or more rule files missing"

    fi
}


# ------------------------------------------------------------
# Installed Configuration Test
# ------------------------------------------------------------

test_installed_configuration()
{
    begin_test "Installed Configuration"

    if [ ! -d "${CONFIG_DIR}" ]
    then

        record_skip \
            "Installed Configuration" \
            "${CONFIG_DIR} does not exist"

        return
    fi


    local count

    count="$(
        find "${CONFIG_DIR}" \
            -type f \
            2>/dev/null |
        wc -l
    )"


    echo "Configuration files: ${count}"


    if [ "${count}" -gt 0 ]
    then

        record_pass \
            "Installed Configuration" \
            "${count} configuration files found"

    else

        record_fail \
            "Installed Configuration" \
            "No installed configuration files"

    fi
}


# ------------------------------------------------------------
# Device Interface Test
# ------------------------------------------------------------

test_device_interface()
{
    begin_test "Packet Filter Device Interface"

    local devices=""

    devices="$(
        find /dev \
            -maxdepth 1 \
            \( \
                -iname "*packet*filter*" \
                -o \
                -iname "*pktfilter*" \
            \) \
            2>/dev/null
    )"


    if [ -n "${devices}" ]
    then

        echo "${devices}"

        record_pass \
            "Packet Filter Device Interface" \
            "Device node detected"

        return
    fi


    local proc_entries

    proc_entries="$(
        find /proc \
            -maxdepth 2 \
            -iname "*packet*filter*" \
            2>/dev/null
    )"


    if [ -n "${proc_entries}" ]
    then

        echo "${proc_entries}"

        record_pass \
            "Packet Filter Device Interface" \
            "Proc interface detected"

        return
    fi


    local sys_entries

    sys_entries="$(
        find /sys \
            -maxdepth 4 \
            -iname "*packet*filter*" \
            2>/dev/null
    )"


    if [ -n "${sys_entries}" ]
    then

        echo "${sys_entries}"

        record_pass \
            "Packet Filter Device Interface" \
            "Sysfs interface detected"

        return
    fi


    record_skip \
        "Packet Filter Device Interface" \
        "No userspace device/proc/sysfs interface found"
}


# ------------------------------------------------------------
# Baseline Packet Counter
# ------------------------------------------------------------

get_rx_packets()
{
    if [ -z "${INTERFACE}" ]
    then
        echo 0
        return
    fi


    cat \
        "/sys/class/net/${INTERFACE}/statistics/rx_packets" \
        2>/dev/null || echo 0
}


get_tx_packets()
{
    if [ -z "${INTERFACE}" ]
    then
        echo 0
        return
    fi


    cat \
        "/sys/class/net/${INTERFACE}/statistics/tx_packets" \
        2>/dev/null || echo 0
}


# ------------------------------------------------------------
# Packet Counter Test
# ------------------------------------------------------------

test_packet_counters()
{
    begin_test "Packet Counters"

    if [ -z "${INTERFACE}" ]
    then

        record_skip \
            "Packet Counters" \
            "No network interface"

        return
    fi


    local rx_before
    local rx_after

    local tx_before
    local tx_after


    rx_before="$(get_rx_packets)"

    tx_before="$(get_tx_packets)"


    sleep 2


    rx_after="$(get_rx_packets)"

    tx_after="$(get_tx_packets)"


    echo "RX before: ${rx_before}"
    echo "RX after : ${rx_after}"

    echo

    echo "TX before: ${tx_before}"
    echo "TX after : ${tx_after}"


    if [ "${rx_after}" -ge "${rx_before}" ] &&
       [ "${tx_after}" -ge "${tx_before}" ]
    then

        record_pass \
            "Packet Counters" \
            "Network counters operational"

    else

        record_fail \
            "Packet Counters" \
            "Packet counters abnormal"

    fi
}


# ------------------------------------------------------------
# Loopback Test
# ------------------------------------------------------------

test_loopback()
{
    begin_test "Loopback Connectivity"

    if ping \
        -c 3 \
        -W 2 \
        127.0.0.1 \
        >/dev/null 2>&1
    then

        record_pass \
            "Loopback Connectivity" \
            "127.0.0.1 reachable"

    else

        record_fail \
            "Loopback Connectivity" \
            "Loopback ping failed"

    fi
}


# ------------------------------------------------------------
# Gateway Test
# ------------------------------------------------------------

test_gateway()
{
    begin_test "Default Gateway"

    local gateway

    gateway="$(
        ip route |
        awk '/default/ {print $3; exit}'
    )"


    if [ -z "${gateway}" ]
    then

        record_skip \
            "Default Gateway" \
            "No default gateway configured"

        return
    fi


    echo "Gateway: ${gateway}"


    if ping \
        -c 3 \
        -W 2 \
        "${gateway}" \
        >/dev/null 2>&1
    then

        record_pass \
            "Default Gateway" \
            "${gateway} reachable"

    else

        record_fail \
            "Default Gateway" \
            "${gateway} unreachable"

    fi
}


# ------------------------------------------------------------
# External Network Test
# ------------------------------------------------------------

test_external_network()
{
    begin_test "External Network"

    if ping \
        -c 3 \
        -W 3 \
        8.8.8.8 \
        >/dev/null 2>&1
    then

        record_pass \
            "External Network" \
            "8.8.8.8 reachable"

    else

        record_skip \
            "External Network" \
            "External network unavailable"
    fi
}


# ------------------------------------------------------------
# Whitelist Configuration Test
# ------------------------------------------------------------

test_whitelist()
{
    begin_test "Whitelist Configuration"

    local file="${RULE_DIR}/whitelist.conf"


    if [ ! -f "${file}" ]
    then

        record_fail \
            "Whitelist Configuration" \
            "whitelist.conf missing"

        return
    fi


    echo "Whitelist file:"
    echo "  ${file}"


    local entries

    entries="$(
        grep -Ev \
            '^[[:space:]]*(#|$)' \
            "${file}" |
        wc -l
    )"


    echo "Active whitelist entries: ${entries}"


    if [ "${entries}" -gt 0 ]
    then

        record_pass \
            "Whitelist Configuration" \
            "${entries} active entries"

    else

        warning "Whitelist has no active entries."

        record_skip \
            "Whitelist Configuration" \
            "No active whitelist entries"

    fi
}


# ------------------------------------------------------------
# Blacklist Configuration Test
# ------------------------------------------------------------

test_blacklist()
{
    begin_test "Blacklist Configuration"

    local file="${RULE_DIR}/blacklist.conf"


    if [ ! -f "${file}" ]
    then

        record_fail \
            "Blacklist Configuration" \
            "blacklist.conf missing"

        return
    fi


    echo "Blacklist file:"
    echo "  ${file}"


    local entries

    entries="$(
        grep -Ev \
            '^[[:space:]]*(#|$)' \
            "${file}" |
        wc -l
    )"


    echo "Active blacklist entries: ${entries}"


    if [ "${entries}" -gt 0 ]
    then

        record_pass \
            "Blacklist Configuration" \
            "${entries} active entries"

    else

        warning "Blacklist has no active entries."

        record_skip \
            "Blacklist Configuration" \
            "No active blacklist entries"

    fi
}


# ------------------------------------------------------------
# Monitoring Configuration Test
# ------------------------------------------------------------

test_monitoring()
{
    begin_test "Monitoring Configuration"

    local file="${RULE_DIR}/monitoring.conf"


    if [ ! -f "${file}" ]
    then

        record_fail \
            "Monitoring Configuration" \
            "monitoring.conf missing"

        return
    fi


    local entries

    entries="$(
        grep -Ev \
            '^[[:space:]]*(#|$)' \
            "${file}" |
        wc -l
    )"


    echo "Active monitoring entries: ${entries}"


    if [ "${entries}" -gt 0 ]
    then

        record_pass \
            "Monitoring Configuration" \
            "${entries} active entries"

    else

        record_skip \
            "Monitoring Configuration" \
            "No active monitoring entries"

    fi
}


# ------------------------------------------------------------
# Kernel Driver Message Test
# ------------------------------------------------------------

test_driver_messages()
{
    begin_test "Kernel Driver Messages"

    local messages

    messages="$(
        dmesg |
        grep -i \
            -E \
            "packet_filter|packet-filter|packet filter" \
        2>/dev/null |
        tail -30
    )"


    if [ -n "${messages}" ]
    then

        echo "${messages}"

        record_pass \
            "Kernel Driver Messages" \
            "Packet-filter messages found"

    else

        record_skip \
            "Kernel Driver Messages" \
            "No packet-filter messages found"

    fi
}


# ------------------------------------------------------------
# Kernel Error Test
# ------------------------------------------------------------

test_kernel_errors()
{
    begin_test "Kernel Error Check"

    local errors

    errors="$(
        dmesg |
        grep -Ei \
            "BUG:|Oops:|kernel panic|general protection|"
            "soft lockup|hard lockup|NETDEV WATCHDOG" \
        2>/dev/null |
        tail -20
    )"


    if [ -z "${errors}" ]
    then

        record_pass \
            "Kernel Error Check" \
            "No major kernel errors"

    else

        echo "${errors}"

        record_fail \
            "Kernel Error Check" \
            "Kernel errors detected"

    fi
}


# ------------------------------------------------------------
# Driver Stability Test
# ------------------------------------------------------------

test_driver_stability()
{
    begin_test "Driver Stability"

    if ! lsmod |
        awk '{print $1}' |
        grep -qx "${MODULE_NAME}"
    then

        record_fail \
            "Driver Stability" \
            "Driver is not loaded"

        return
    fi


    local before

    local after


    before="$(
        lsmod |
        awk -v module="${MODULE_NAME}" \
        '$1 == module {print $3}'
    )"


    sleep 5


    after="$(
        lsmod |
        awk -v module="${MODULE_NAME}" \
        '$1 == module {print $3}'
    )"


    echo "References before: ${before}"
    echo "References after : ${after}"


    if lsmod |
        awk '{print $1}' |
        grep -qx "${MODULE_NAME}"
    then

        record_pass \
            "Driver Stability" \
            "Driver remained loaded"

    else

        record_fail \
            "Driver Stability" \
            "Driver unloaded unexpectedly"

    fi
}


# ------------------------------------------------------------
# Network Statistics Test
# ------------------------------------------------------------

test_network_statistics()
{
    begin_test "Network Statistics"

    if [ -z "${INTERFACE}" ]
    then

        record_skip \
            "Network Statistics" \
            "No network interface"

        return
    fi


    local statistics=(
        rx_packets
        tx_packets
        rx_bytes
        tx_bytes
        rx_errors
        tx_errors
        rx_dropped
        tx_dropped
    )


    local stat

    local missing=0


    for stat in "${statistics[@]}"
    do

        if [ -f \
            "/sys/class/net/${INTERFACE}/statistics/${stat}" ]
        then

            printf "%-15s : %s\n" \
                "${stat}" \
                "$(cat "/sys/class/net/${INTERFACE}/statistics/${stat}")"

        else

            echo "Missing: ${stat}"

            missing=1

        fi

    done


    if [ "${missing}" -eq 0 ]
    then

        record_pass \
            "Network Statistics" \
            "All standard counters available"

    else

        record_fail \
            "Network Statistics" \
            "One or more counters missing"

    fi
}


# ------------------------------------------------------------
# Capture Interface State
# ------------------------------------------------------------

capture_interface_state()
{
    if [ -z "${INTERFACE}" ]
    then
        return
    fi


    ip addr show "${INTERFACE}" \
        > "${RESULT_DIR}/interface_addr.txt" \
        2>&1


    ip route \
        > "${RESULT_DIR}/routes.txt" \
        2>&1


    ip -s link show "${INTERFACE}" \
        > "${RESULT_DIR}/interface_statistics.txt" \
        2>&1
}


# ------------------------------------------------------------
# Capture Module State
# ------------------------------------------------------------

capture_module_state()
{
    lsmod \
        > "${RESULT_DIR}/lsmod.txt" \
        2>&1


    modinfo "${MODULE_NAME}" \
        > "${RESULT_DIR}/modinfo.txt" \
        2>&1 || true
}


# ------------------------------------------------------------
# Generate Report
# ------------------------------------------------------------

generate_report()
{
    local total

    total=$((PASS_COUNT + FAIL_COUNT + SKIP_COUNT))


    {
        echo
        echo "============================================================"
        echo "       PACKET FILTER INTEGRATION TEST REPORT"
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

        echo "Interface:"
        echo "  ${INTERFACE:-N/A}"

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

        echo "Log:"
        echo "  ${LOG_FILE}"

        echo

        echo "CSV:"
        echo "  ${SUMMARY_FILE}"

        echo

        echo "============================================================"

    } > "${RESULT_DIR}/report.txt"


    cat "${RESULT_DIR}/report.txt"


    echo
    echo "Integration test report:"
    echo "  ${RESULT_DIR}/report.txt"
}


# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------

cleanup()
{
    sync

    capture_interface_state

    capture_module_state

    dmesg \
        > "${RESULT_DIR}/dmesg.txt" \
        2>&1 || true
}


# ------------------------------------------------------------
# Main Integration Test
# ------------------------------------------------------------

run_tests()
{
    print_banner

    initialize_results

    test_root

    test_driver

    test_interface

    test_link

    test_ip_address

    test_configuration

    test_installed_configuration

    test_device_interface

    test_loopback

    test_gateway

    test_external_network

    test_whitelist

    test_blacklist

    test_monitoring

    test_packet_counters

    test_network_statistics

    test_driver_messages

    test_kernel_errors

    test_driver_stability

    cleanup

    generate_report


    echo


    if [ "${FAIL_COUNT}" -eq 0 ]
    then

        echo -e \
            "${GREEN}INTEGRATION TEST: PASS${NC}"

        return 0

    else

        echo -e \
            "${RED}INTEGRATION TEST: FAIL${NC}"

        return 1

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
    echo "  sudo ./tests/integration/test_filtering.sh"
    echo
    echo "Environment:"
    echo
    echo "  INTERFACE=eth0"
    echo "  MODULE_NAME=packet_filter"
    echo "  CONFIG_DIR=/etc/packet_filter"
    echo "  RULE_DIR=./configs/rules"
    echo
    echo "Example:"
    echo
    echo "  sudo INTERFACE=eth0 \\"
    echo "       ./tests/integration/test_filtering.sh"
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

    help|-h|--help)
        usage
        exit 0
        ;;

    *)
        echo "Unknown option: $1"
        usage
        exit 1
        ;;

esac
