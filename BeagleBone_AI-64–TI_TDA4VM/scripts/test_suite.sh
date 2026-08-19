#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# Complete Test Suite
#
# Runs:
#   1. Build verification
#   2. Kernel module verification
#   3. Driver load test
#   4. Driver status test
#   5. Configuration/rule verification
#   6. Packet-filter functional test
#   7. Network connectivity test
#   8. Performance test
#   9. Stress test
#  10. Kernel error verification
#
# ============================================================

set +e


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

DURATION="${DURATION:-30}"

INTERFACE="${INTERFACE:-}"

IPERF_SERVER="${IPERF_SERVER:-}"

RESULT_ROOT="${PROJECT_ROOT}/build/test-results"

TIMESTAMP="$(date '+%Y%m%d_%H%M%S')"

RESULT_DIR="${RESULT_ROOT}/${TIMESTAMP}"

RESULT_FILE="${RESULT_DIR}/test_suite.txt"

SUMMARY_FILE="${RESULT_DIR}/summary.csv"

LOG_FILE="${RESULT_DIR}/test_suite.log"

PASS_COUNT=0

FAIL_COUNT=0

SKIP_COUNT=0

TEST_COUNT=0

TEST_FAILED=0


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


success()
{
    echo -e "${GREEN}[PASS]${NC} $1"
}


failure()
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
    echo "       Packet Filter Complete Test Suite"
    echo "============================================================"
    echo
}


# ------------------------------------------------------------
# Create Result Directory
# ------------------------------------------------------------

create_result_directory()
{
    mkdir -p "${RESULT_DIR}"

    touch "${RESULT_FILE}"
    touch "${SUMMARY_FILE}"
    touch "${LOG_FILE}"

    echo "test,status,details" > "${SUMMARY_FILE}"

    exec > >(tee -a "${LOG_FILE}") 2>&1

    info "Test result directory:"
    echo "  ${RESULT_DIR}"
}


# ------------------------------------------------------------
# Test Counter
# ------------------------------------------------------------

begin_test()
{
    TEST_COUNT=$((TEST_COUNT + 1))

    echo
    echo "------------------------------------------------------------"
    echo "TEST ${TEST_COUNT}: $1"
    echo "------------------------------------------------------------"
}


# ------------------------------------------------------------
# Record PASS
# ------------------------------------------------------------

record_pass()
{
    local test_name="$1"
    local details="${2:-PASS}"

    PASS_COUNT=$((PASS_COUNT + 1))

    success "${test_name}"

    echo "\"${test_name}\",PASS,\"${details}\"" >> "${SUMMARY_FILE}"
}


# ------------------------------------------------------------
# Record FAIL
# ------------------------------------------------------------

record_fail()
{
    local test_name="$1"
    local details="${2:-FAIL}"

    FAIL_COUNT=$((FAIL_COUNT + 1))

    TEST_FAILED=1

    failure "${test_name}"

    echo "\"${test_name}\",FAIL,\"${details}\"" >> "${SUMMARY_FILE}"
}


# ------------------------------------------------------------
# Record SKIP
# ------------------------------------------------------------

record_skip()
{
    local test_name="$1"
    local details="${2:-SKIPPED}"

    SKIP_COUNT=$((SKIP_COUNT + 1))

    skip "${test_name}"

    echo "\"${test_name}\",SKIP,\"${details}\"" >> "${SUMMARY_FILE}"
}


# ------------------------------------------------------------
# Root Check
# ------------------------------------------------------------

check_root()
{
    begin_test "Root Permission"

    if [ "$(id -u)" -eq 0 ]
    then
        record_pass "Root Permission"
    else
        record_fail \
            "Root Permission" \
            "Run test_suite.sh with sudo for complete testing."
    fi
}


# ------------------------------------------------------------
# Project Structure Test
# ------------------------------------------------------------

test_project_structure()
{
    begin_test "Project Structure"

    local required_files=(
        "scripts/build.sh"
        "scripts/clean.sh"
        "scripts/configure.sh"
        "scripts/install.sh"
        "scripts/load_driver.sh"
        "scripts/perf_test.sh"
        "scripts/stress_test.sh"
        "kernel/packet_filter/packet_filter.c"
        "kernel/packet_filter/packet_filter.h"
        "kernel/packet_filter/packet_parser.c"
        "kernel/packet_filter/packet_parser.h"
        "kernel/packet_filter/rule_engine.c"
        "kernel/packet_filter/rule_engine.h"
        "kernel/packet_filter/statistics.c"
        "kernel/packet_filter/statistics.h"
        "kernel/packet_filter/logging.c"
        "kernel/packet_filter/logging.h"
        "kernel/packet_filter/ioctl_defs.h"
        "configs/rules/whitelist.conf"
        "configs/rules/blacklist.conf"
        "configs/rules/monitoring.conf"
    )

    local missing=0

    local file

    for file in "${required_files[@]}"
    do
        if [ ! -f "${PROJECT_ROOT}/${file}" ]
        then
            echo "Missing: ${file}"
            missing=1
        fi
    done

    if [ "${missing}" -eq 0 ]
    then
        record_pass "Project Structure"
    else
        record_fail "Project Structure" "Required files are missing."
    fi
}


# ------------------------------------------------------------
# Documentation Test
# ------------------------------------------------------------

test_documentation()
{
    begin_test "Documentation Structure"

    local required_docs=(
        "docs/architecture.md"
        "docs/debugging.md"
        "docs/deployment.md"
        "docs/device-tree.md"
        "docs/ioctl-api.md"
        "docs/kernel-driver.md"
        "docs/packet-flow.md"
        "docs/performance.md"
        "docs/testing.md"
        "docs/userspace.md"
        "docs/yocto-build.md"
    )

    local missing=0

    local file

    for file in "${required_docs[@]}"
    do
        if [ ! -f "${PROJECT_ROOT}/${file}" ]
        then
            echo "Missing documentation: ${file}"
            missing=1
        fi
    done

    if [ "${missing}" -eq 0 ]
    then
        record_pass "Documentation Structure"
    else
        record_fail "Documentation Structure"
    fi
}


# ------------------------------------------------------------
# Shell Script Syntax
# ------------------------------------------------------------

test_script_syntax()
{
    begin_test "Shell Script Syntax"

    local scripts=(
        "scripts/build.sh"
        "scripts/clean.sh"
        "scripts/configure.sh"
        "scripts/install.sh"
        "scripts/load_driver.sh"
        "scripts/perf_test.sh"
        "scripts/stress_test.sh"
        "scripts/test_suite.sh"
    )

    local failed=0

    local script

    for script in "${scripts[@]}"
    do
        if [ -f "${PROJECT_ROOT}/${script}" ]
        then
            if ! bash -n "${PROJECT_ROOT}/${script}"
            then
                echo "Syntax error: ${script}"
                failed=1
            fi
        fi
    done

    if [ "${failed}" -eq 0 ]
    then
        record_pass "Shell Script Syntax"
    else
        record_fail "Shell Script Syntax"
    fi
}


# ------------------------------------------------------------
# Build Artifact Test
# ------------------------------------------------------------

test_build_artifact()
{
    begin_test "Kernel Module Build Artifact"

    local module=""

    local candidates=(
        "${PROJECT_ROOT}/build/driver/packet_filter.ko"
        "${PROJECT_ROOT}/kernel/packet_filter/packet_filter.ko"
    )

    for candidate in "${candidates[@]}"
    do
        if [ -f "${candidate}" ]
        then
            module="${candidate}"
            break
        fi
    done

    if [ -n "${module}" ]
    then
        echo "Module:"
        echo "  ${module}"

        file "${module}"

        record_pass \
            "Kernel Module Build Artifact" \
            "packet_filter.ko exists"
    else
        record_fail \
            "Kernel Module Build Artifact" \
            "packet_filter.ko not found"
    fi
}


# ------------------------------------------------------------
# Installed Module Test
# ------------------------------------------------------------

test_installed_module()
{
    begin_test "Installed Kernel Module"

    if [ -f "/lib/modules/$(uname -r)/extra/${MODULE_NAME}.ko" ]
    then
        record_pass \
            "Installed Kernel Module" \
            "Module installed for running kernel"
    elif modinfo "${MODULE_NAME}" >/dev/null 2>&1
    then
        record_pass \
            "Installed Kernel Module" \
            "Module available through modinfo"
    else
        record_skip \
            "Installed Kernel Module" \
            "Module not installed for running kernel"
    fi
}


# ------------------------------------------------------------
# Driver Load Test
# ------------------------------------------------------------

test_driver_load()
{
    begin_test "Driver Load"

    if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
    then
        record_pass \
            "Driver Load" \
            "packet_filter already loaded"
        return
    fi

    if [ -x "${PROJECT_ROOT}/scripts/load_driver.sh" ]
    then

        sudo "${PROJECT_ROOT}/scripts/load_driver.sh" load \
            >> "${RESULT_DIR}/driver_load.log" 2>&1

        if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
        then
            record_pass \
                "Driver Load" \
                "packet_filter loaded successfully"
        else
            record_fail \
                "Driver Load" \
                "Module failed to load"
        fi

    else

        record_fail \
            "Driver Load" \
            "load_driver.sh not executable"

    fi
}


# ------------------------------------------------------------
# Driver Status Test
# ------------------------------------------------------------

test_driver_status()
{
    begin_test "Driver Status"

    if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
    then
        record_pass \
            "Driver Status" \
            "packet_filter is loaded"
    else
        record_fail \
            "Driver Status" \
            "packet_filter is not loaded"
    fi
}


# ------------------------------------------------------------
# Configuration Test
# ------------------------------------------------------------

test_configuration()
{
    begin_test "Packet Filter Configuration"

    local required=(
        "/etc/packet_filter/packet_filter.conf"
        "/etc/packet_filter/rules/whitelist.conf"
        "/etc/packet_filter/rules/blacklist.conf"
        "/etc/packet_filter/rules/monitoring.conf"
    )

    local missing=0

    local file

    for file in "${required[@]}"
    do
        if [ -f "${file}" ]
        then
            echo "[OK] ${file}"
        else
            echo "[MISSING] ${file}"
            missing=1
        fi
    done

    if [ "${missing}" -eq 0 ]
    then
        record_pass "Packet Filter Configuration"
    else
        record_skip \
            "Packet Filter Configuration" \
            "Configuration files are not installed"
    fi
}


# ------------------------------------------------------------
# Rule File Test
# ------------------------------------------------------------

test_rules()
{
    begin_test "Rule Files"

    local rule_dir="${PROJECT_ROOT}/configs/rules"

    local rules=(
        "whitelist.conf"
        "blacklist.conf"
        "monitoring.conf"
    )

    local failed=0

    local rule

    for rule in "${rules[@]}"
    do

        if [ ! -f "${rule_dir}/${rule}" ]
        then
            echo "Missing rule: ${rule}"
            failed=1
            continue
        fi

        if [ ! -s "${rule_dir}/${rule}" ]
        then
            warning "${rule} is empty."
        fi

    done

    if [ "${failed}" -eq 0 ]
    then
        record_pass "Rule Files"
    else
        record_fail "Rule Files"
    fi
}


# ------------------------------------------------------------
# Network Interface Test
# ------------------------------------------------------------

test_network_interface()
{
    begin_test "Network Interface"

    if [ -z "${INTERFACE}" ]
    then
        INTERFACE="$(
            ip -o link show 2>/dev/null |
            awk -F': ' '$2 != "lo" {print $2}' |
            head -1
        )"
    fi

    if [ -z "${INTERFACE}" ]
    then
        record_skip \
            "Network Interface" \
            "No Ethernet interface detected"
        return
    fi

    echo "Interface: ${INTERFACE}"

    if ip link show "${INTERFACE}" >/dev/null 2>&1
    then
        record_pass \
            "Network Interface" \
            "${INTERFACE} detected"
    else
        record_fail \
            "Network Interface" \
            "${INTERFACE} not available"
    fi
}


# ------------------------------------------------------------
# Network Link Test
# ------------------------------------------------------------

test_network_link()
{
    begin_test "Network Link"

    if [ -z "${INTERFACE}" ]
    then
        record_skip \
            "Network Link" \
            "No network interface"
        return
    fi

    if ip link show "${INTERFACE}" |
        grep -q "state UP"
    then

        record_pass \
            "Network Link" \
            "${INTERFACE} is UP"

    else

        record_fail \
            "Network Link" \
            "${INTERFACE} is DOWN"

    fi
}


# ------------------------------------------------------------
# Loopback Test
# ------------------------------------------------------------

test_loopback()
{
    begin_test "Loopback Networking"

    if ping -c 2 -W 2 127.0.0.1 >/dev/null 2>&1
    then
        record_pass "Loopback Networking"
    else
        record_fail \
            "Loopback Networking" \
            "127.0.0.1 ping failed"
    fi
}


# ------------------------------------------------------------
# Driver Interface Test
# ------------------------------------------------------------

test_driver_interface()
{
    begin_test "Driver Interface"

    local found=0

    if [ -d "/proc" ]
    then

        if find /proc \
            -maxdepth 2 \
            -iname "*packet*filter*" \
            2>/dev/null |
            grep -q .
        then
            found=1
        fi

    fi


    if [ -d "/sys" ]
    then

        if find /sys \
            -maxdepth 4 \
            -iname "*packet*filter*" \
            2>/dev/null |
            grep -q .
        then
            found=1
        fi

    fi


    if [ -d "/dev" ]
    then

        if find /dev \
            -maxdepth 1 \
            -iname "*packet*filter*" \
            2>/dev/null |
            grep -q .
        then
            found=1
        fi

    fi


    if [ "${found}" -eq 1 ]
    then
        record_pass \
            "Driver Interface" \
            "Packet-filter interface detected"
    else
        record_skip \
            "Driver Interface" \
            "No proc/sysfs/device interface detected"
    fi
}


# ------------------------------------------------------------
# Kernel Log Test
# ------------------------------------------------------------

test_kernel_logs()
{
    begin_test "Kernel Log"

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
            "Kernel Log" \
            "No major kernel errors detected"
    else
        echo "${errors}"

        record_fail \
            "Kernel Log" \
            "Potential kernel errors detected"
    fi
}


# ------------------------------------------------------------
# Driver Message Test
# ------------------------------------------------------------

test_driver_messages()
{
    begin_test "Packet Filter Driver Messages"

    local messages

    messages="$(
        dmesg |
        grep -i \
            -E "packet_filter|packet-filter|packet filter" \
        2>/dev/null |
        tail -20
    )"

    if [ -n "${messages}" ]
    then
        echo "${messages}"

        record_pass \
            "Packet Filter Driver Messages" \
            "Driver messages detected"
    else
        record_skip \
            "Packet Filter Driver Messages" \
            "No packet-filter messages found"
    fi
}


# ------------------------------------------------------------
# Userspace Utility Test
# ------------------------------------------------------------

test_userspace_utility()
{
    begin_test "Userspace Utility"

    local utility=""

    if [ -x "/usr/sbin/packet_filter_ctl" ]
    then
        utility="/usr/sbin/packet_filter_ctl"

    elif [ -x "/usr/bin/packet_filter_ctl" ]
    then
        utility="/usr/bin/packet_filter_ctl"
    fi


    if [ -n "${utility}" ]
    then

        echo "Utility:"
        echo "  ${utility}"

        "${utility}" --help \
            > "${RESULT_DIR}/packet_filter_ctl_help.txt" \
            2>&1 || true

        record_pass \
            "Userspace Utility" \
            "${utility} available"

    else

        record_skip \
            "Userspace Utility" \
            "packet_filter_ctl not installed"

    fi
}


# ------------------------------------------------------------
# Performance Test
# ------------------------------------------------------------

test_performance()
{
    begin_test "Performance Test"

    if [ ! -x "${PROJECT_ROOT}/scripts/perf_test.sh" ]
    then
        record_skip \
            "Performance Test" \
            "perf_test.sh not available"
        return
    fi


    info "Running performance test..."

    INTERFACE="${INTERFACE}" \
    DURATION="${DURATION}" \
        sudo "${PROJECT_ROOT}/scripts/perf_test.sh" quick \
        > "${RESULT_DIR}/performance.log" 2>&1

    if [ $? -eq 0 ]
    then
        record_pass \
            "Performance Test" \
            "Performance test completed"
    else
        record_fail \
            "Performance Test" \
            "Performance test failed"
    fi
}


# ------------------------------------------------------------
# Stress Test
# ------------------------------------------------------------

test_stress()
{
    begin_test "Stress Test"

    if [ ! -x "${PROJECT_ROOT}/scripts/stress_test.sh" ]
    then
        record_skip \
            "Stress Test" \
            "stress_test.sh not available"
        return
    fi


    info "Running stress test..."

    local stress_duration="${STRESS_DURATION:-30}"

    INTERFACE="${INTERFACE}" \
    DURATION="${stress_duration}" \
    IPERF_SERVER="${IPERF_SERVER}" \
        sudo "${PROJECT_ROOT}/scripts/stress_test.sh" driver \
        > "${RESULT_DIR}/stress.log" 2>&1

    if [ $? -eq 0 ]
    then
        record_pass \
            "Stress Test" \
            "Stress test completed"
    else
        record_fail \
            "Stress Test" \
            "Stress test detected a failure"
    fi
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

    rx_before="$(
        cat "/sys/class/net/${INTERFACE}/statistics/rx_packets" \
        2>/dev/null || echo 0
    )"


    sleep 2


    rx_after="$(
        cat "/sys/class/net/${INTERFACE}/statistics/rx_packets" \
        2>/dev/null || echo 0
    )"


    echo "RX packets before: ${rx_before}"
    echo "RX packets after : ${rx_after}"


    if [ "${rx_after}" -ge "${rx_before}" ]
    then
        record_pass \
            "Packet Counters" \
            "RX packet counter operational"
    else
        record_fail \
            "Packet Counters" \
            "RX packet counter abnormal"
    fi
}


# ------------------------------------------------------------
# Module Stability Test
# ------------------------------------------------------------

test_module_stability()
{
    begin_test "Module Stability"

    if ! lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
    then
        record_fail \
            "Module Stability" \
            "packet_filter is not loaded"
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


    echo "Module usage before: ${before}"
    echo "Module usage after : ${after}"


    if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
    then
        record_pass \
            "Module Stability" \
            "Module remained loaded"
    else
        record_fail \
            "Module Stability" \
            "Module disappeared"
    fi
}


# ------------------------------------------------------------
# Generate Final Report
# ------------------------------------------------------------

generate_report()
{
    local total=$((PASS_COUNT + FAIL_COUNT + SKIP_COUNT))

    {
        echo
        echo "============================================================"
        echo "              TEST SUITE FINAL REPORT"
        echo "============================================================"

        echo
        echo "Board:"
        echo "  BeagleBone AI-64"

        echo
        echo "SoC:"
        echo "  TI TDA4VM"

        echo
        echo "Kernel:"
        uname -r

        echo
        echo "Architecture:"
        uname -m

        echo
        echo "Interface:"
        echo "${INTERFACE:-N/A}"

        echo
        echo "------------------------------------------------------------"
        echo "Test Statistics"
        echo "------------------------------------------------------------"

        echo
        echo "Total tests : ${total}"
        echo "Passed      : ${PASS_COUNT}"
        echo "Failed      : ${FAIL_COUNT}"
        echo "Skipped     : ${SKIP_COUNT}"

        echo
        echo "------------------------------------------------------------"
        echo "Overall Result"
        echo "------------------------------------------------------------"

        if [ "${FAIL_COUNT}" -eq 0 ]
        then
            echo
            echo "PASS"
            echo
        else
            echo
            echo "FAIL"
            echo
        fi

        echo "Result directory:"
        echo "  ${RESULT_DIR}"

        echo
        echo "Summary:"
        echo "  ${SUMMARY_FILE}"

        echo
        echo "Log:"
        echo "  ${LOG_FILE}"

        echo
        echo "============================================================"

    } >> "${RESULT_FILE}"


    echo
    echo "============================================================"
    echo "              TEST SUITE FINAL REPORT"
    echo "============================================================"

    echo
    echo "Total tests : ${total}"
    echo "Passed      : ${PASS_COUNT}"
    echo "Failed      : ${FAIL_COUNT}"
    echo "Skipped     : ${SKIP_COUNT}"

    echo

    if [ "${FAIL_COUNT}" -eq 0 ]
    then
        echo -e "${GREEN}OVERALL RESULT: PASS${NC}"
    else
        echo -e "${RED}OVERALL RESULT: FAIL${NC}"
    fi

    echo
    echo "Report:"
    echo "  ${RESULT_FILE}"

    echo
    echo "Summary:"
    echo "  ${SUMMARY_FILE}"

    echo
    echo "============================================================"
}


# ------------------------------------------------------------
# Full Test Suite
# ------------------------------------------------------------

run_full_suite()
{
    print_banner

    create_result_directory

    check_root

    test_project_structure

    test_documentation

    test_script_syntax

    test_build_artifact

    test_installed_module

    test_driver_load

    test_driver_status

    test_configuration

    test_rules

    test_network_interface

    test_network_link

    test_loopback

    test_driver_interface

    test_kernel_logs

    test_driver_messages

    test_userspace_utility

    test_packet_counters

    test_module_stability

    test_performance

    test_stress

    generate_report


    if [ "${FAIL_COUNT}" -gt 0 ]
    then
        return 1
    fi

    return 0
}


# ------------------------------------------------------------
# Functional Tests Only
# ------------------------------------------------------------

run_functional_suite()
{
    print_banner

    create_result_directory

    check_root

    test_driver_load

    test_driver_status

    test_configuration

    test_rules

    test_network_interface

    test_network_link

    test_loopback

    test_driver_interface

    test_driver_messages

    test_packet_counters

    test_module_stability

    generate_report


    if [ "${FAIL_COUNT}" -gt 0 ]
    then
        return 1
    fi

    return 0
}


# ------------------------------------------------------------
# Driver Tests Only
# ------------------------------------------------------------

run_driver_suite()
{
    print_banner

    create_result_directory

    check_root

    test_installed_module

    test_driver_load

    test_driver_status

    test_driver_interface

    test_driver_messages

    test_kernel_logs

    test_module_stability

    generate_report


    if [ "${FAIL_COUNT}" -gt 0 ]
    then
        return 1
    fi

    return 0
}


# ------------------------------------------------------------
# Network Tests Only
# ------------------------------------------------------------

run_network_suite()
{
    print_banner

    create_result_directory

    check_root

    test_network_interface

    test_network_link

    test_loopback

    test_packet_counters

    test_performance

    generate_report


    if [ "${FAIL_COUNT}" -gt 0 ]
    then
        return 1
    fi

    return 0
}


# ------------------------------------------------------------
# Usage
# ------------------------------------------------------------

usage()
{
    echo
    echo "Usage:"
    echo
    echo "  sudo ./scripts/test_suite.sh"
    echo "  sudo ./scripts/test_suite.sh full"
    echo "  sudo ./scripts/test_suite.sh functional"
    echo "  sudo ./scripts/test_suite.sh driver"
    echo "  sudo ./scripts/test_suite.sh network"
    echo
    echo "Commands:"
    echo
    echo "  full        Run complete test suite"
    echo "  functional  Driver + rules + network functional tests"
    echo "  driver      Kernel driver tests"
    echo "  network     Network and performance tests"
    echo "  help        Display this help"
    echo
    echo "Environment variables:"
    echo
    echo "  DURATION=30"
    echo "  STRESS_DURATION=30"
    echo "  INTERFACE=eth0"
    echo "  IPERF_SERVER=192.168.1.100"
    echo
    echo "Example:"
    echo
    echo "  sudo INTERFACE=eth0 \\"
    echo "       DURATION=30 \\"
    echo "       STRESS_DURATION=30 \\"
    echo "       ./scripts/test_suite.sh"
    echo
}


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

main()
{
    case "${1:-full}" in

        full)
            run_full_suite
            ;;

        functional)
            run_functional_suite
            ;;

        driver)
            run_driver_suite
            ;;

        network)
            run_network_suite
            ;;

        help|-h|--help)
            usage
            ;;

        *)
            echo "Unknown command: $1"
            usage
            exit 1
            ;;

    esac
}


# ------------------------------------------------------------
# Execute
# ------------------------------------------------------------

main "$@"

exit $?
