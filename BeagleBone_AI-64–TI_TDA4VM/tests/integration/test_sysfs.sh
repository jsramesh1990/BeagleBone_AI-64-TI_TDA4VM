#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# SYSFS Integration Test
#
# Purpose:
#   Validate the packet-filter driver's sysfs integration,
#   attributes, permissions, runtime state and statistics.
#
# Expected flow:
#
#   Userspace
#       |
#       v
#   /sys/class/packet_filter/
#       |
#       +-- status
#       +-- enabled
#       +-- statistics
#       +-- rules
#       +-- packets_accepted
#       +-- packets_dropped
#       +-- packets_monitored
#       |
#       v
#   packet_filter kernel driver
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

SYSFS_ROOT="${SYSFS_ROOT:-/sys/class/packet_filter}"

RESULT_ROOT="${PROJECT_ROOT}/build/test-results/sysfs"

TIMESTAMP="$(date '+%Y%m%d_%H%M%S')"

RESULT_DIR="${RESULT_ROOT}/${TIMESTAMP}"

LOG_FILE="${RESULT_DIR}/sysfs_test.log"

SUMMARY_FILE="${RESULT_DIR}/summary.csv"

TOTAL_TESTS=0
PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

# ------------------------------------------------------------
# Expected Attribute Names
#
# These are common attributes for this project.
#
# The script does not fail if an optional attribute is absent.
# ------------------------------------------------------------

EXPECTED_ATTRIBUTES=(
    status
    enabled
    statistics
    rules
    packets_accepted
    packets_dropped
    packets_monitored
)

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

# ------------------------------------------------------------
# Banner
# ------------------------------------------------------------

print_banner()
{
    echo
    echo "============================================================"
    echo "       BeagleBone AI-64 - TI TDA4VM"
    echo "       Packet Filter SYSFS Integration Test"
    echo "============================================================"
    echo
}

# ------------------------------------------------------------
# Initialize Results
# ------------------------------------------------------------

initialize_results()
{
    mkdir -p "${RESULT_DIR}"

    touch "${LOG_FILE}"

    touch "${SUMMARY_FILE}"

    echo "test,status,details" > "${SUMMARY_FILE}"

    exec > >(tee -a "${LOG_FILE}") 2>&1

    info "SYSFS test results:"
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
# Result Functions
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

record_fail()
{
    local name="$1"
    local details="${2:-FAIL}"

    FAIL_COUNT=$((FAIL_COUNT + 1))

    fail "${name}"

    echo "\"${name}\",FAIL,\"${details}\"" \
        >> "${SUMMARY_FILE}"
}

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
# SYSFS Mounted
# ------------------------------------------------------------

test_sysfs_mount()
{
    begin_test "SYSFS Mount"

    if mountpoint -q /sys
    then
        record_pass \
            "SYSFS Mount" \
            "/sys is mounted"
    else
        record_fail \
            "SYSFS Mount" \
            "/sys is not mounted"
    fi
}

# ------------------------------------------------------------
# Kernel Module
# ------------------------------------------------------------

test_module()
{
    begin_test "Packet Filter Kernel Module"

    if lsmod |
        awk '{print $1}' |
        grep -qx "${MODULE_NAME}"
    then
        record_pass \
            "Packet Filter Kernel Module" \
            "${MODULE_NAME} loaded"
        return
    fi

    if modinfo "${MODULE_NAME}" >/dev/null 2>&1
    then

        if modprobe "${MODULE_NAME}" >/dev/null 2>&1
        then

            record_pass \
                "Packet Filter Kernel Module" \
                "Module loaded using modprobe"

        else

            record_fail \
                "Packet Filter Kernel Module" \
                "Unable to load module"

        fi

    else

        record_fail \
            "Packet Filter Kernel Module" \
            "Module not installed"

    fi
}

# ------------------------------------------------------------
# SYSFS Root Directory
# ------------------------------------------------------------

test_sysfs_root()
{
    begin_test "Packet Filter SYSFS Root"

    if [ -d "${SYSFS_ROOT}" ]
    then

        echo "SYSFS root:"
        echo "  ${SYSFS_ROOT}"

        record_pass \
            "Packet Filter SYSFS Root" \
            "SYSFS class directory exists"

    else

        record_fail \
            "Packet Filter SYSFS Root" \
            "${SYSFS_ROOT} not found"

    fi
}

# ------------------------------------------------------------
# List SYSFS Entries
# ------------------------------------------------------------

test_list_entries()
{
    begin_test "SYSFS Attribute Discovery"

    if [ ! -d "${SYSFS_ROOT}" ]
    then

        record_skip \
            "SYSFS Attribute Discovery" \
            "SYSFS root unavailable"

        return
    fi

    local count

    count="$(
        find "${SYSFS_ROOT}" \
            -maxdepth 2 \
            -type f \
            2>/dev/null |
        wc -l
    )"

    echo "SYSFS files found: ${count}"

    find "${SYSFS_ROOT}" \
        -maxdepth 2 \
        -type f \
        -print \
        2>/dev/null |
        sort

    if [ "${count}" -gt 0 ]
    then

        record_pass \
            "SYSFS Attribute Discovery" \
            "${count} SYSFS files found"

    else

        record_fail \
            "SYSFS Attribute Discovery" \
            "No SYSFS attributes found"

    fi
}

# ------------------------------------------------------------
# Find Attribute
# ------------------------------------------------------------

find_attribute()
{
    local attribute="$1"

    if [ -f "${SYSFS_ROOT}/${attribute}" ]
    then
        echo "${SYSFS_ROOT}/${attribute}"
        return 0
    fi

    local path

    path="$(
        find "${SYSFS_ROOT}" \
            -maxdepth 3 \
            -type f \
            -name "${attribute}" \
            2>/dev/null |
        head -1
    )"

    if [ -n "${path}" ]
    then
        echo "${path}"
        return 0
    fi

    return 1
}

# ------------------------------------------------------------
# Expected Attribute Test
# ------------------------------------------------------------

test_expected_attributes()
{
    begin_test "Expected SYSFS Attributes"

    if [ ! -d "${SYSFS_ROOT}" ]
    then

        record_skip \
            "Expected SYSFS Attributes" \
            "SYSFS root unavailable"

        return
    fi

    local found=0
    local missing=0
    local attribute
    local path

    for attribute in "${EXPECTED_ATTRIBUTES[@]}"
    do

        path="$(find_attribute "${attribute}")"

        if [ -n "${path}" ]
        then

            echo "[FOUND]   ${attribute}"
            echo "          ${path}"

            found=$((found + 1))

        else

            echo "[MISSING] ${attribute}"

            missing=$((missing + 1))

        fi

    done

    echo
    echo "Found  : ${found}"
    echo "Missing: ${missing}"

    if [ "${found}" -gt 0 ]
    then

        record_pass \
            "Expected SYSFS Attributes" \
            "${found} expected attributes found; ${missing} missing"

    else

        record_fail \
            "Expected SYSFS Attributes" \
            "No expected attributes found"

    fi
}

# ------------------------------------------------------------
# Read Attribute
# ------------------------------------------------------------

read_attribute()
{
    local attribute="$1"

    local path

    path="$(find_attribute "${attribute}")"

    if [ -z "${path}" ]
    then
        return 1
    fi

    cat "${path}" 2>/dev/null

    return $?
}

# ------------------------------------------------------------
# Attribute Read Test
# ------------------------------------------------------------

test_attribute_reads()
{
    begin_test "SYSFS Attribute Read"

    if [ ! -d "${SYSFS_ROOT}" ]
    then

        record_skip \
            "SYSFS Attribute Read" \
            "SYSFS root unavailable"

        return
    fi

    local readable=0
    local unreadable=0
    local attribute
    local path
    local value

    for attribute in "${EXPECTED_ATTRIBUTES[@]}"
    do

        path="$(find_attribute "${attribute}")"

        if [ -z "${path}" ]
        then
            continue
        fi

        if [ -r "${path}" ]
        then

            value="$(cat "${path}" 2>/dev/null)"

            echo "${attribute}:"
            echo "  ${value}"

            readable=$((readable + 1))

        else

            echo "${attribute}: NOT READABLE"

            unreadable=$((unreadable + 1))

        fi

    done

    echo
    echo "Readable  : ${readable}"
    echo "Unreadable: ${unreadable}"

    if [ "${readable}" -gt 0 ] &&
       [ "${unreadable}" -eq 0 ]
    then

        record_pass \
            "SYSFS Attribute Read" \
            "${readable} attributes readable"

    elif [ "${readable}" -gt 0 ]
    then

        record_fail \
            "SYSFS Attribute Read" \
            "${unreadable} attributes unreadable"

    else

        record_skip \
            "SYSFS Attribute Read" \
            "No expected readable attributes"

    fi
}

# ------------------------------------------------------------
# Attribute Permissions
# ------------------------------------------------------------

test_attribute_permissions()
{
    begin_test "SYSFS Attribute Permissions"

    if [ ! -d "${SYSFS_ROOT}" ]
    then

        record_skip \
            "SYSFS Attribute Permissions" \
            "SYSFS root unavailable"

        return
    fi

    local files

    files="$(
        find "${SYSFS_ROOT}" \
            -type f \
            2>/dev/null
    )"

    if [ -z "${files}" ]
    then

        record_skip \
            "SYSFS Attribute Permissions" \
            "No attributes found"

        return
    fi

    local invalid=0
    local file
    local mode

    while IFS= read -r file
    do

        [ -z "${file}" ] && continue

        mode="$(stat -c '%a' "${file}" 2>/dev/null)"

        echo "$(basename "${file}") : ${mode}"

        case "${mode}" in
            4??|6??|7??)
                ;;
            *)
                invalid=$((invalid + 1))
                ;;
        esac

    done <<< "${files}"

    if [ "${invalid}" -eq 0 ]
    then

        record_pass \
            "SYSFS Attribute Permissions" \
            "SYSFS permissions valid"

    else

        record_fail \
            "SYSFS Attribute Permissions" \
            "${invalid} suspicious permissions"

    fi
}

# ------------------------------------------------------------
# Status Attribute
# ------------------------------------------------------------

test_status()
{
    begin_test "SYSFS Status"

    local path

    path="$(find_attribute status)"

    if [ -z "${path}" ]
    then

        record_skip \
            "SYSFS Status" \
            "status attribute not implemented"

        return
    fi

    local value

    value="$(cat "${path}" 2>/dev/null)"

    echo "Status:"
    echo "${value}"

    if [ -n "${value}" ]
    then

        record_pass \
            "SYSFS Status" \
            "Status attribute readable"

    else

        record_fail \
            "SYSFS Status" \
            "Status attribute empty"

    fi
}

# ------------------------------------------------------------
# Enabled Attribute
# ------------------------------------------------------------

test_enabled()
{
    begin_test "SYSFS Enabled State"

    local path

    path="$(find_attribute enabled)"

    if [ -z "${path}" ]
    then

        record_skip \
            "SYSFS Enabled State" \
            "enabled attribute not implemented"

        return
    fi

    local value

    value="$(cat "${path}" 2>/dev/null)"

    echo "Enabled: ${value}"

    case "${value}" in
        0|1)
            record_pass \
                "SYSFS Enabled State" \
                "Valid enabled value: ${value}"
            ;;

        enabled|disabled|true|false|on|off)
            record_pass \
                "SYSFS Enabled State" \
                "Valid enabled state: ${value}"
            ;;

        *)
            record_fail \
                "SYSFS Enabled State" \
                "Unexpected value: ${value}"
            ;;
    esac
}

# ------------------------------------------------------------
# Statistics Attributes
# ------------------------------------------------------------

test_statistics()
{
    begin_test "SYSFS Statistics"

    local found=0
    local attribute
    local path
    local value

    for attribute in \
        packets_accepted \
        packets_dropped \
        packets_monitored
    do

        path="$(find_attribute "${attribute}")"

        if [ -z "${path}" ]
        then
            continue
        fi

        value="$(cat "${path}" 2>/dev/null)"

        echo "${attribute}: ${value}"

        found=$((found + 1))

        if [[ "${value}" =~ ^[0-9]+$ ]]
        then

            echo "  Numeric counter: valid"

        else

            warning \
                "${attribute} is not a numeric counter"

        fi

    done

    if [ "${found}" -gt 0 ]
    then

        record_pass \
            "SYSFS Statistics" \
            "${found} statistics attributes found"

    else

        record_skip \
            "SYSFS Statistics" \
            "Statistics attributes not implemented"

    fi
}

# ------------------------------------------------------------
# Statistics Stability
# ------------------------------------------------------------

test_statistics_stability()
{
    begin_test "SYSFS Statistics Stability"

    local attributes=(
        packets_accepted
        packets_dropped
        packets_monitored
    )

    local tested=0
    local failures=0
    local attribute
    local path
    local first
    local second

    for attribute in "${attributes[@]}"
    do

        path="$(find_attribute "${attribute}")"

        if [ -z "${path}" ]
        then
            continue
        fi

        first="$(cat "${path}" 2>/dev/null)"

        sleep 1

        second="$(cat "${path}" 2>/dev/null)"

        echo "${attribute}:"
        echo "  Before: ${first}"
        echo "  After : ${second}"

        tested=$((tested + 1))

        if [[ "${first}" =~ ^[0-9]+$ ]] &&
           [[ "${second}" =~ ^[0-9]+$ ]]
        then

            if [ "${second}" -lt "${first}" ]
            then

                failures=$((failures + 1))

                echo "  ERROR: counter decreased"

            fi

        fi

    done

    if [ "${tested}" -eq 0 ]
    then

        record_skip \
            "SYSFS Statistics Stability" \
            "No statistics attributes available"

    elif [ "${failures}" -eq 0 ]
    then

        record_pass \
            "SYSFS Statistics Stability" \
            "Counters remained stable"

    else

        record_fail \
            "SYSFS Statistics Stability" \
            "${failures} counters decreased"

    fi
}

# ------------------------------------------------------------
# Rules Attribute
# ------------------------------------------------------------

test_rules()
{
    begin_test "SYSFS Rules"

    local path

    path="$(find_attribute rules)"

    if [ -z "${path}" ]
    then

        record_skip \
            "SYSFS Rules" \
            "rules attribute not implemented"

        return
    fi

    echo "Rules:"
    cat "${path}" 2>/dev/null

    if [ -r "${path}" ]
    then

        record_pass \
            "SYSFS Rules" \
            "Rules attribute readable"

    else

        record_fail \
            "SYSFS Rules" \
            "Rules attribute not readable"

    fi
}

# ------------------------------------------------------------
# SYSFS Ownership
# ------------------------------------------------------------

test_ownership()
{
    begin_test "SYSFS Ownership"

    if [ ! -d "${SYSFS_ROOT}" ]
    then

        record_skip \
            "SYSFS Ownership" \
            "SYSFS root unavailable"

        return
    fi

    local owner

    owner="$(stat -c '%U:%G' "${SYSFS_ROOT}" 2>/dev/null)"

    echo "Owner: ${owner}"

    if [ -n "${owner}" ]
    then

        record_pass \
            "SYSFS Ownership" \
            "SYSFS ownership readable"

    else

        record_fail \
            "SYSFS Ownership" \
            "Unable to read ownership"

    fi
}

# ------------------------------------------------------------
# Device Class Discovery
# ------------------------------------------------------------

test_class_discovery()
{
    begin_test "SYSFS Class Discovery"

    local classes=(
        /sys/class/packet_filter
        /sys/class/net
        /sys/class/misc
        /sys/class
    )

    local found=0
    local path

    for path in "${classes[@]}"
    do

        if [ -d "${path}" ]
        then

            echo "[FOUND] ${path}"

            found=$((found + 1))

        else

            echo "[MISSING] ${path}"

        fi

    done

    if [ "${found}" -gt 0 ]
    then

        record_pass \
            "SYSFS Class Discovery" \
            "${found} SYSFS classes available"

    else

        record_fail \
            "SYSFS Class Discovery" \
            "SYSFS classes unavailable"

    fi
}

# ------------------------------------------------------------
# Kernel Messages
# ------------------------------------------------------------

test_kernel_messages()
{
    begin_test "SYSFS Kernel Messages"

    local messages

    messages="$(
        dmesg |
        grep -Ei \
            "packet_filter|packet-filter|sysfs" \
        2>/dev/null |
        tail -50
    )"

    if [ -n "${messages}" ]
    then

        echo "${messages}"

        record_pass \
            "SYSFS Kernel Messages" \
            "Relevant kernel messages found"

    else

        record_skip \
            "SYSFS Kernel Messages" \
            "No relevant kernel messages found"

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
# Runtime SYSFS Snapshot
# ------------------------------------------------------------

capture_sysfs()
{
    if [ ! -d "${SYSFS_ROOT}" ]
    then
        return
    fi

    find "${SYSFS_ROOT}" \
        -maxdepth 3 \
        -type f \
        -print \
        2>/dev/null |
        sort \
        > "${RESULT_DIR}/sysfs_files.txt"

    local file
    local output

    while IFS= read -r file
    do

        [ -z "${file}" ] && continue

        output="${RESULT_DIR}/values"

        mkdir -p "${output}"

        cat "${file}" \
            > "${output}/$(basename "${file}")" \
            2>&1 || true

    done < "${RESULT_DIR}/sysfs_files.txt"
}

# ------------------------------------------------------------
# Module Snapshot
# ------------------------------------------------------------

capture_module()
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
        echo "       PACKET FILTER SYSFS TEST REPORT"
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

        echo "SYSFS Root:"
        echo "  ${SYSFS_ROOT}"

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
# Main
# ------------------------------------------------------------

run_tests()
{
    print_banner

    initialize_results

    test_root

    test_sysfs_mount

    test_module

    test_sysfs_root

    test_class_discovery

    test_list_entries

    test_expected_attributes

    test_attribute_reads

    test_attribute_permissions

    test_ownership

    test_status

    test_enabled

    test_statistics

    test_statistics_stability

    test_rules

    test_kernel_messages

    test_kernel_errors

    capture_sysfs

    capture_module

    generate_report

    echo

    if [ "${FAIL_COUNT}" -eq 0 ]
    then

        echo -e \
            "${GREEN}SYSFS INTEGRATION TEST: PASS${NC}"

        return 0

    else

        echo -e \
            "${RED}SYSFS INTEGRATION TEST: FAIL${NC}"

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

    echo "SYSFS root:"
    echo "  ${SYSFS_ROOT}"

    if [ -d "${SYSFS_ROOT}" ]
    then

        echo -e "${GREEN}SYSFS: AVAILABLE${NC}"

        echo
        echo "Attributes:"

        find "${SYSFS_ROOT}" \
            -maxdepth 3 \
            -type f \
            -print \
            2>/dev/null |
            sort

    else

        echo -e "${YELLOW}SYSFS: NOT FOUND${NC}"

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
    echo "  sudo ./tests/integration/test_sysfs.sh"
    echo "      Run complete SYSFS integration test"
    echo
    echo "  sudo ./tests/integration/test_sysfs.sh status"
    echo "      Display SYSFS status"
    echo
    echo "Environment variables:"
    echo
    echo "  MODULE_NAME=packet_filter"
    echo "  SYSFS_ROOT=/sys/class/packet_filter"
    echo
}

# ------------------------------------------------------------
# Main Entry
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
