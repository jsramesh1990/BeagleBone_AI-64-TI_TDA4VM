#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# Stress Test Script
#
# Purpose:
#   - Generate sustained CPU load
#   - Generate sustained network traffic
#   - Exercise packet-filter rules
#   - Monitor packet drops
#   - Monitor memory usage
#   - Monitor CPU usage
#   - Monitor interrupts / SoftIRQs
#   - Detect kernel errors during stress
#   - Generate a complete stress-test report
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

MODULE_NAME="packet_filter"

DURATION="${DURATION:-60}"

INTERFACE="${INTERFACE:-}"

IPERF_SERVER="${IPERF_SERVER:-}"

IPERF_PORT="${IPERF_PORT:-5201}"

CPU_WORKERS="${CPU_WORKERS:-}"

MEMORY_MB="${MEMORY_MB:-128}"

UDP_BANDWIDTH="${UDP_BANDWIDTH:-100M}"

PACKET_SIZE="${PACKET_SIZE:-1024}"

SAMPLE_INTERVAL="${SAMPLE_INTERVAL:-1}"

RESULT_ROOT="${PROJECT_ROOT}/build/stress"

TIMESTAMP="$(date '+%Y%m%d_%H%M%S')"

RESULT_DIR="${RESULT_ROOT}/${TIMESTAMP}"

RESULT_FILE="${RESULT_DIR}/stress_test.txt"

MONITOR_FILE="${RESULT_DIR}/monitor.csv"

DMESG_BEFORE="${RESULT_DIR}/dmesg_before.txt"

DMESG_AFTER="${RESULT_DIR}/dmesg_after.txt"

CPU_LOG="${RESULT_DIR}/cpu.log"

MEMORY_LOG="${RESULT_DIR}/memory.log"

NETWORK_LOG="${RESULT_DIR}/network.log"

IRQ_LOG="${RESULT_DIR}/interrupts.log"

TRAFFIC_LOG="${RESULT_DIR}/traffic.log"


# ------------------------------------------------------------
# Runtime State
# ------------------------------------------------------------

CPU_PIDS=()

MEMORY_PID=""

TRAFFIC_PID=""

MONITOR_PID=""

TEST_FAILED=0

START_TIME=""

END_TIME=""


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
    echo "       BeagleBone AI-64 - TI TDA4VM"
    echo "       Packet Filter Stress Test"
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
        warning "Root privileges are recommended."

        echo
        echo "Run:"
        echo
        echo "  sudo ./scripts/stress_test.sh"
        echo
    fi
}


# ------------------------------------------------------------
# Create Result Directory
# ------------------------------------------------------------

create_result_directory()
{
    mkdir -p "${RESULT_DIR}"

    touch "${RESULT_FILE}"
    touch "${MONITOR_FILE}"

    success "Stress-test result directory:"
    echo "  ${RESULT_DIR}"
}


# ------------------------------------------------------------
# Detect CPU Workers
# ------------------------------------------------------------

detect_cpu_workers()
{
    if [ -n "${CPU_WORKERS}" ]
    then
        return
    fi

    CPU_WORKERS="$(nproc 2>/dev/null || echo 1)"

    # Leave one CPU available for kernel/network monitoring
    if [ "${CPU_WORKERS}" -gt 1 ]
    then
        CPU_WORKERS=$((CPU_WORKERS - 1))
    fi
}


# ------------------------------------------------------------
# Detect Network Interface
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

    if [ -z "${INTERFACE}" ]
    then
        warning "No non-loopback network interface detected."
    fi
}


# ------------------------------------------------------------
# Check Required Commands
# ------------------------------------------------------------

check_commands()
{
    info "Checking required commands..."

    local commands=(
        awk
        date
        dmesg
        free
        grep
        ip
        lsmod
        ps
        sleep
        uname
        vmstat
    )

    local missing=0

    for command in "${commands[@]}"
    do
        if ! command -v "${command}" >/dev/null 2>&1
        then
            warning "Missing command: ${command}"
            missing=1
        fi
    done

    if [ "${missing}" -eq 0 ]
    then
        success "Required commands available."
    else
        warning "Some optional commands are missing."
    fi
}


# ------------------------------------------------------------
# Check Packet Filter Driver
# ------------------------------------------------------------

check_driver()
{
    info "Checking packet-filter driver..."

    if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
    then
        success "${MODULE_NAME} is loaded."
        return 0
    fi

    warning "${MODULE_NAME} is NOT loaded."

    echo
    echo "The stress test can continue for system/network testing,"
    echo "but packet-filter-specific validation will be limited."
    echo

    return 1
}


# ------------------------------------------------------------
# Capture Network Counters
# ------------------------------------------------------------

read_network_counters()
{
    local interface="$1"

    local rx_bytes=0
    local tx_bytes=0
    local rx_packets=0
    local tx_packets=0
    local rx_dropped=0
    local tx_dropped=0
    local rx_errors=0
    local tx_errors=0

    if [ -d "/sys/class/net/${interface}" ]
    then
        rx_bytes="$(
            cat "/sys/class/net/${interface}/statistics/rx_bytes" \
            2>/dev/null || echo 0
        )"

        tx_bytes="$(
            cat "/sys/class/net/${interface}/statistics/tx_bytes" \
            2>/dev/null || echo 0
        )"

        rx_packets="$(
            cat "/sys/class/net/${interface}/statistics/rx_packets" \
            2>/dev/null || echo 0
        )"

        tx_packets="$(
            cat "/sys/class/net/${interface}/statistics/tx_packets" \
            2>/dev/null || echo 0
        )"

        rx_dropped="$(
            cat "/sys/class/net/${interface}/statistics/rx_dropped" \
            2>/dev/null || echo 0
        )"

        tx_dropped="$(
            cat "/sys/class/net/${interface}/statistics/tx_dropped" \
            2>/dev/null || echo 0
        )"

        rx_errors="$(
            cat "/sys/class/net/${interface}/statistics/rx_errors" \
            2>/dev/null || echo 0
        )"

        tx_errors="$(
            cat "/sys/class/net/${interface}/statistics/tx_errors" \
            2>/dev/null || echo 0
        )"
    fi

    echo "${rx_bytes} ${tx_bytes} ${rx_packets} ${tx_packets} ${rx_dropped} ${tx_dropped} ${rx_errors} ${tx_errors}"
}


# ------------------------------------------------------------
# Capture CPU Counters
# ------------------------------------------------------------

read_cpu_counters()
{
    awk '/^cpu / {
        print $2,$3,$4,$5,$6,$7,$8,$9
    }' /proc/stat
}


# ------------------------------------------------------------
# Capture Memory
# ------------------------------------------------------------

read_memory()
{
    awk '
        /^MemTotal:/     { total=$2 }
        /^MemAvailable:/ { available=$2 }
        /^MemFree:/      { free=$2 }
        END {
            print total, available, free
        }
    ' /proc/meminfo
}


# ------------------------------------------------------------
# Collect System Information
# ------------------------------------------------------------

collect_system_info()
{
    info "Collecting system information..."

    {
        echo
        echo "============================================================"
        echo "SYSTEM INFORMATION"
        echo "============================================================"

        echo
        echo "Date:"
        date

        echo
        echo "Hostname:"
        hostname

        echo
        echo "Kernel:"
        uname -a

        echo
        echo "Architecture:"
        uname -m

        echo
        echo "CPU:"
        lscpu 2>/dev/null || true

        echo
        echo "Memory:"
        free -h

        echo
        echo "Load:"
        uptime

        echo
        echo "Interface:"
        echo "${INTERFACE}"

        echo
        echo "CPU Workers:"
        echo "${CPU_WORKERS}"

        echo
        echo "Memory Stress:"
        echo "${MEMORY_MB} MB"

        echo
        echo "Duration:"
        echo "${DURATION} seconds"

        echo
        echo "============================================================"

    } >> "${RESULT_FILE}"

    success "System information collected."
}


# ------------------------------------------------------------
# Save Initial Kernel Messages
# ------------------------------------------------------------

capture_dmesg_before()
{
    info "Capturing initial kernel messages..."

    dmesg > "${DMESG_BEFORE}" 2>/dev/null || true

    success "Initial dmesg captured."
}


# ------------------------------------------------------------
# CPU Stress Worker
# ------------------------------------------------------------

cpu_worker()
{
    local end_time

    end_time=$((SECONDS + DURATION))

    local value=1

    while [ "${SECONDS}" -lt "${end_time}" ]
    do
        value=$((value * 1664525 + 1013904223))

        value=$((value ^ (value >> 13)))

        value=$((value * 31))

        value=$((value & 0x7fffffff))

        if [ "${value}" -eq -1 ]
        then
            echo >/dev/null
        fi
    done
}


# ------------------------------------------------------------
# Start CPU Stress
# ------------------------------------------------------------

start_cpu_stress()
{
    info "Starting CPU stress..."

    CPU_PIDS=()

    local worker

    for ((worker=0; worker<CPU_WORKERS; worker++))
    do
        cpu_worker &
        CPU_PIDS+=("$!")
    done

    success "Started ${#CPU_PIDS[@]} CPU stress workers."
}


# ------------------------------------------------------------
# Stop CPU Stress
# ------------------------------------------------------------

stop_cpu_stress()
{
    local pid

    for pid in "${CPU_PIDS[@]}"
    do
        if kill -0 "${pid}" 2>/dev/null
        then
            kill "${pid}" 2>/dev/null || true
        fi
    done

    CPU_PIDS=()
}


# ------------------------------------------------------------
# Memory Stress
# ------------------------------------------------------------

start_memory_stress()
{
    if ! command -v stress-ng >/dev/null 2>&1
    then
        warning "stress-ng is not installed."
        warning "Skipping external memory stress."
        return
    fi

    info "Starting memory stress: ${MEMORY_MB} MB..."

    stress-ng \
        --vm 1 \
        --vm-bytes "${MEMORY_MB}M" \
        --vm-keep \
        --timeout "${DURATION}s" \
        > "${RESULT_DIR}/stress-ng-memory.log" 2>&1 &

    MEMORY_PID="$!"

    success "Memory stress started."
}


# ------------------------------------------------------------
# Stop Memory Stress
# ------------------------------------------------------------

stop_memory_stress()
{
    if [ -n "${MEMORY_PID}" ]
    then
        if kill -0 "${MEMORY_PID}" 2>/dev/null
        then
            kill "${MEMORY_PID}" 2>/dev/null || true
        fi

        MEMORY_PID=""
    fi
}


# ------------------------------------------------------------
# Start Network Traffic
# ------------------------------------------------------------

start_network_stress()
{
    if [ -z "${IPERF_SERVER}" ]
    then
        warning "IPERF_SERVER not specified."
        warning "Network traffic generation will be skipped."
        return
    fi

    if ! command -v iperf3 >/dev/null 2>&1
    then
        warning "iperf3 is not installed."
        warning "Network stress will be skipped."
        return
    fi

    info "Starting UDP network stress..."

    iperf3 \
        -c "${IPERF_SERVER}" \
        -p "${IPERF_PORT}" \
        -u \
        -b "${UDP_BANDWIDTH}" \
        -l "${PACKET_SIZE}" \
        -t "${DURATION}" \
        > "${TRAFFIC_LOG}" 2>&1 &

    TRAFFIC_PID="$!"

    success "Network stress started."
}


# ------------------------------------------------------------
# Stop Network Stress
# ------------------------------------------------------------

stop_network_stress()
{
    if [ -n "${TRAFFIC_PID}" ]
    then
        if kill -0 "${TRAFFIC_PID}" 2>/dev/null
        then
            kill "${TRAFFIC_PID}" 2>/dev/null || true
        fi

        TRAFFIC_PID=""
    fi
}


# ------------------------------------------------------------
# Monitor CPU
# ------------------------------------------------------------

monitor_cpu()
{
    {
        echo "timestamp,user,nice,system,idle,iowait,irq,softirq"

        while [ -f "${RESULT_DIR}/.running" ]
        do

            timestamp="$(date '+%H:%M:%S')"

            cpu="$(read_cpu_counters)"

            echo "${timestamp},${cpu}"

            sleep "${SAMPLE_INTERVAL}"

        done

    } > "${CPU_LOG}" 2>&1
}


# ------------------------------------------------------------
# Monitor Memory
# ------------------------------------------------------------

monitor_memory()
{
    {
        echo "timestamp,total_kb,available_kb,free_kb"

        while [ -f "${RESULT_DIR}/.running" ]
        do

            timestamp="$(date '+%H:%M:%S')"

            memory="$(read_memory)"

            echo "${timestamp},${memory}"

            sleep "${SAMPLE_INTERVAL}"

        done

    } > "${MEMORY_LOG}" 2>&1
}


# ------------------------------------------------------------
# Monitor Network
# ------------------------------------------------------------

monitor_network()
{
    {
        echo "timestamp,rx_bytes,tx_bytes,rx_packets,tx_packets,rx_dropped,tx_dropped,rx_errors,tx_errors"

        while [ -f "${RESULT_DIR}/.running" ]
        do

            timestamp="$(date '+%H:%M:%S')"

            counters="$(read_network_counters "${INTERFACE}")"

            echo "${timestamp},${counters}"

            sleep "${SAMPLE_INTERVAL}"

        done

    } > "${NETWORK_LOG}" 2>&1
}


# ------------------------------------------------------------
# Monitor Interrupts
# ------------------------------------------------------------

monitor_interrupts()
{
    {
        echo "============================================================"
        echo "INTERRUPT MONITOR"
        echo "============================================================"

        while [ -f "${RESULT_DIR}/.running" ]
        do

            echo
            echo "Timestamp: $(date)"

            grep -i \
                -E "eth|net|cpsw|dma" \
                /proc/interrupts \
                2>/dev/null || true

            echo
            echo "SoftIRQ:"
            grep -E "NET_RX|NET_TX" /proc/softirqs 2>/dev/null || true

            sleep "${SAMPLE_INTERVAL}"

        done

    } > "${IRQ_LOG}" 2>&1
}


# ------------------------------------------------------------
# Start Monitors
# ------------------------------------------------------------

start_monitors()
{
    info "Starting monitoring..."

    touch "${RESULT_DIR}/.running"

    monitor_cpu &
    MONITOR_CPU_PID="$!"

    monitor_memory &
    MONITOR_MEMORY_PID="$!"

    if [ -n "${INTERFACE}" ]
    then
        monitor_network &
        MONITOR_NETWORK_PID="$!"
    else
        MONITOR_NETWORK_PID=""
    fi

    monitor_interrupts &
    MONITOR_IRQ_PID="$!"

    success "Monitoring started."
}


# ------------------------------------------------------------
# Stop Monitors
# ------------------------------------------------------------

stop_monitors()
{
    rm -f "${RESULT_DIR}/.running"

    for pid in \
        "${MONITOR_CPU_PID:-}" \
        "${MONITOR_MEMORY_PID:-}" \
        "${MONITOR_NETWORK_PID:-}" \
        "${MONITOR_IRQ_PID:-}"
    do
        if [ -n "${pid}" ]
        then
            kill "${pid}" 2>/dev/null || true
        fi
    done

    wait 2>/dev/null || true
}


# ------------------------------------------------------------
# Start Stress
# ------------------------------------------------------------

start_stress()
{
    info "Starting stress workload..."

    start_cpu_stress

    start_memory_stress

    start_network_stress

    success "Stress workload started."
}


# ------------------------------------------------------------
# Wait For Test
# ------------------------------------------------------------

wait_for_test()
{
    info "Running stress test for ${DURATION} seconds..."

    local elapsed=0

    while [ "${elapsed}" -lt "${DURATION}" ]
    do

        sleep 1

        elapsed=$((elapsed + 1))

        if [ $((elapsed % 10)) -eq 0 ]
        then
            echo "  Stress test progress: ${elapsed}/${DURATION} seconds"
        fi

    done

    success "Stress duration completed."
}


# ------------------------------------------------------------
# Capture Final Kernel Messages
# ------------------------------------------------------------

capture_dmesg_after()
{
    info "Capturing final kernel messages..."

    dmesg > "${DMESG_AFTER}" 2>/dev/null || true

    success "Final dmesg captured."
}


# ------------------------------------------------------------
# Check Kernel Errors
# ------------------------------------------------------------

check_kernel_errors()
{
    info "Checking for kernel errors..."

    local error_count

    error_count="$(
        grep -Ei \
            "BUG:|Oops:|kernel panic|Call Trace:|general protection|"
            "segfault|watchdog|NETDEV WATCHDOG|soft lockup|hard lockup" \
            "${DMESG_AFTER}" \
            2>/dev/null |
        wc -l
    )"

    {
        echo
        echo "============================================================"
        echo "KERNEL ERROR CHECK"
        echo "============================================================"

        echo
        echo "Potential kernel errors:"
        echo "${error_count}"

        echo

        if [ "${error_count}" -gt 0 ]
        then
            echo "STATUS: FAIL"

            grep -Ei \
                "BUG:|Oops:|kernel panic|Call Trace:|general protection|segfault|watchdog|NETDEV WATCHDOG|soft lockup|hard lockup" \
                "${DMESG_AFTER}" \
                2>/dev/null || true

            TEST_FAILED=1

        else
            echo "STATUS: PASS"
        fi

    } >> "${RESULT_FILE}"

    if [ "${error_count}" -gt 0 ]
    then
        error "Potential kernel errors detected."
    else
        success "No major kernel errors detected."
    fi
}


# ------------------------------------------------------------
# Check Network Errors
# ------------------------------------------------------------

check_network_errors()
{
    if [ -z "${INTERFACE}" ]
    then
        return
    fi

    info "Checking network errors..."

    local counters

    counters="$(read_network_counters "${INTERFACE}")"

    local rx_bytes
    local tx_bytes
    local rx_packets
    local tx_packets
    local rx_dropped
    local tx_dropped
    local rx_errors
    local tx_errors

    read \
        rx_bytes \
        tx_bytes \
        rx_packets \
        tx_packets \
        rx_dropped \
        tx_dropped \
        rx_errors \
        tx_errors \
        <<< "${counters}"


    {
        echo
        echo "============================================================"
        echo "NETWORK ERROR CHECK"
        echo "============================================================"

        echo
        echo "Interface:"
        echo "${INTERFACE}"

        echo
        echo "RX packets:"
        echo "${rx_packets}"

        echo
        echo "TX packets:"
        echo "${tx_packets}"

        echo
        echo "RX dropped:"
        echo "${rx_dropped}"

        echo
        echo "TX dropped:"
        echo "${tx_dropped}"

        echo
        echo "RX errors:"
        echo "${rx_errors}"

        echo
        echo "TX errors:"
        echo "${tx_errors}"

    } >> "${RESULT_FILE}"

    if [ "${rx_errors}" -gt 0 ] || [ "${tx_errors}" -gt 0 ]
    then
        warning "Network errors detected."
    else
        success "No network interface errors reported."
    fi
}


# ------------------------------------------------------------
# Driver Validation
# ------------------------------------------------------------

validate_driver()
{
    info "Validating packet-filter driver..."

    {
        echo
        echo "============================================================"
        echo "PACKET FILTER VALIDATION"
        echo "============================================================"

        echo
        echo "Module status:"

        if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
        then
            echo "LOADED"
        else
            echo "NOT LOADED"
        fi

        echo
        echo "Module information:"
        modinfo "${MODULE_NAME}" 2>/dev/null || true

        echo
        echo "Packet-filter kernel messages:"

        grep -i \
            -E "packet_filter|packet-filter|packet filter" \
            "${DMESG_AFTER}" \
            2>/dev/null |
            tail -100 || true

        echo
        echo "Packet-filter interfaces:"

        find /proc \
            -maxdepth 2 \
            -iname "*packet*filter*" \
            2>/dev/null || true

        find /sys \
            -maxdepth 4 \
            -iname "*packet*filter*" \
            2>/dev/null || true

    } >> "${RESULT_FILE}"

    if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
    then
        success "Packet-filter module is still loaded."
    else
        warning "Packet-filter module is not loaded."
    fi
}


# ------------------------------------------------------------
# Collect Final System State
# ------------------------------------------------------------

collect_final_state()
{
    info "Collecting final system state..."

    {
        echo
        echo "============================================================"
        echo "FINAL SYSTEM STATE"
        echo "============================================================"

        echo
        echo "Date:"
        date

        echo
        echo "Load:"
        uptime

        echo
        echo "Memory:"
        free -h

        echo
        echo "CPU:"
        vmstat 1 2

        echo
        echo "Processes:"
        ps -eo pid,comm,%cpu,%mem --sort=-%cpu | head -20

        echo
        echo "Network:"
        ip -s link show "${INTERFACE}" 2>/dev/null || true

        echo
        echo "============================================================"

    } >> "${RESULT_FILE}"

    success "Final system state collected."
}


# ------------------------------------------------------------
# Calculate Test Duration
# ------------------------------------------------------------

calculate_duration()
{
    if [ -z "${START_TIME}" ] || [ -z "${END_TIME}" ]
    then
        return
    fi

    local elapsed

    elapsed=$((END_TIME - START_TIME))

    echo "${elapsed}"
}


# ------------------------------------------------------------
# Generate Summary
# ------------------------------------------------------------

generate_summary()
{
    info "Generating stress-test summary..."

    local elapsed

    elapsed="$(calculate_duration)"

    {
        echo
        echo "============================================================"
        echo "STRESS TEST SUMMARY"
        echo "============================================================"

        echo
        echo "Test:"
        echo "Packet Filter Stress Test"

        echo
        echo "Board:"
        echo "BeagleBone AI-64"

        echo
        echo "SoC:"
        echo "TI TDA4VM"

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
        echo "Configured duration:"
        echo "${DURATION} seconds"

        echo
        echo "Actual duration:"
        echo "${elapsed} seconds"

        echo
        echo "CPU workers:"
        echo "${CPU_WORKERS}"

        echo
        echo "Memory stress:"
        echo "${MEMORY_MB} MB"

        echo
        echo "UDP bandwidth:"
        echo "${UDP_BANDWIDTH}"

        echo
        echo "Packet size:"
        echo "${PACKET_SIZE} bytes"

        echo
        echo "Packet filter module:"

        if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
        then
            echo "LOADED"
        else
            echo "NOT LOADED"
        fi

        echo
        echo "Test status:"

        if [ "${TEST_FAILED}" -eq 0 ]
        then
            echo "PASS"
        else
            echo "FAIL"
        fi

        echo
        echo "Result directory:"
        echo "${RESULT_DIR}"

        echo
        echo "============================================================"

    } >> "${RESULT_FILE}"
}


# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------

cleanup()
{
    warning "Stopping stress test..."

    stop_cpu_stress

    stop_memory_stress

    stop_network_stress

    stop_monitors

    rm -f "${RESULT_DIR}/.running" 2>/dev/null || true
}


# ------------------------------------------------------------
# Signal Handler
# ------------------------------------------------------------

handle_signal()
{
    echo

    warning "Stress test interrupted."

    END_TIME="$(date +%s)"

    cleanup

    capture_dmesg_after

    check_kernel_errors

    check_network_errors

    validate_driver

    collect_final_state

    generate_summary

    echo
    warning "Stress test terminated by signal."
    echo
    echo "Results:"
    echo "  ${RESULT_FILE}"

    exit 130
}


# ------------------------------------------------------------
# Finalize Test
# ------------------------------------------------------------

finalize_test()
{
    END_TIME="$(date +%s)"

    stop_cpu_stress

    stop_memory_stress

    stop_network_stress

    stop_monitors

    capture_dmesg_after

    check_kernel_errors

    check_network_errors

    validate_driver

    collect_final_state

    generate_summary
}


# ------------------------------------------------------------
# Display Result
# ------------------------------------------------------------

display_result()
{
    echo
    echo "============================================================"
    echo "             STRESS TEST COMPLETED"
    echo "============================================================"

    if [ "${TEST_FAILED}" -eq 0 ]
    then
        success "Stress test status: PASS"
    else
        error "Stress test status: FAIL"
    fi

    echo
    echo "Result directory:"
    echo "  ${RESULT_DIR}"

    echo
    echo "Main report:"
    echo "  ${RESULT_FILE}"

    echo
    echo "CPU monitor:"
    echo "  ${CPU_LOG}"

    echo
    echo "Memory monitor:"
    echo "  ${MEMORY_LOG}"

    echo
    echo "Network monitor:"
    echo "  ${NETWORK_LOG}"

    echo
    echo "Interrupt monitor:"
    echo "  ${IRQ_LOG}"

    echo
    echo "Kernel logs:"
    echo "  ${DMESG_AFTER}"

    echo
    echo "============================================================"
}


# ------------------------------------------------------------
# Full Stress Test
# ------------------------------------------------------------

run_full_test()
{
    print_banner

    check_root

    check_commands

    detect_interface

    detect_cpu_workers

    create_result_directory

    check_driver || true

    collect_system_info

    capture_dmesg_before

    echo

    START_TIME="$(date +%s)"

    start_monitors

    start_stress

    wait_for_test

    finalize_test

    display_result

    echo

    if [ "${TEST_FAILED}" -eq 0 ]
    then
        success "Packet-filter stress test completed successfully."
    else
        error "Packet-filter stress test completed with failures."
    fi

    echo

    return "${TEST_FAILED}"
}


# ------------------------------------------------------------
# CPU Stress Only
# ------------------------------------------------------------

run_cpu_test()
{
    print_banner

    detect_cpu_workers

    create_result_directory

    collect_system_info

    START_TIME="$(date +%s)"

    start_monitors

    start_cpu_stress

    wait_for_test

    END_TIME="$(date +%s)"

    stop_cpu_stress

    stop_monitors

    collect_final_state

    generate_summary

    display_result
}


# ------------------------------------------------------------
# Network Stress Only
# ------------------------------------------------------------

run_network_test()
{
    print_banner

    detect_interface

    create_result_directory

    collect_system_info

    START_TIME="$(date +%s)"

    start_monitors

    start_network_stress

    wait_for_test

    END_TIME="$(date +%s)"

    stop_network_stress

    stop_monitors

    check_network_errors

    collect_final_state

    generate_summary

    display_result
}


# ------------------------------------------------------------
# Packet Filter Stress Only
# ------------------------------------------------------------

run_driver_test()
{
    print_banner

    create_result_directory

    check_driver || true

    capture_dmesg_before

    detect_interface

    START_TIME="$(date +%s)"

    start_monitors

    start_cpu_stress

    start_network_stress

    wait_for_test

    END_TIME="$(date +%s)"

    stop_cpu_stress

    stop_network_stress

    stop_monitors

    capture_dmesg_after

    check_kernel_errors

    check_network_errors

    validate_driver

    generate_summary

    display_result
}


# ------------------------------------------------------------
# Usage
# ------------------------------------------------------------

usage()
{
    echo
    echo "Usage:"
    echo
    echo "  sudo ./scripts/stress_test.sh"
    echo "  sudo ./scripts/stress_test.sh full"
    echo "  sudo ./scripts/stress_test.sh cpu"
    echo "  sudo ./scripts/stress_test.sh network"
    echo "  sudo ./scripts/stress_test.sh driver"
    echo
    echo "Environment variables:"
    echo
    echo "  DURATION=60"
    echo "  INTERFACE=eth0"
    echo "  CPU_WORKERS=3"
    echo "  MEMORY_MB=128"
    echo "  IPERF_SERVER=192.168.1.100"
    echo "  IPERF_PORT=5201"
    echo "  UDP_BANDWIDTH=100M"
    echo "  PACKET_SIZE=1024"
    echo "  SAMPLE_INTERVAL=1"
    echo
    echo "Examples:"
    echo
    echo "  sudo DURATION=300 \\"
    echo "      INTERFACE=eth0 \\"
    echo "      ./scripts/stress_test.sh"
    echo
    echo "  sudo DURATION=120 \\"
    echo "      INTERFACE=eth0 \\"
    echo "      IPERF_SERVER=192.168.1.100 \\"
    echo "      UDP_BANDWIDTH=500M \\"
    echo "      ./scripts/stress_test.sh network"
    echo
    echo "  sudo DURATION=600 \\"
    echo "      INTERFACE=eth0 \\"
    echo "      IPERF_SERVER=192.168.1.100 \\"
    echo "      ./scripts/stress_test.sh driver"
    echo
}


# ------------------------------------------------------------
# Signal Handling
# ------------------------------------------------------------

trap handle_signal INT TERM


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

main()
{
    case "${1:-full}" in

        full)
            run_full_test
            ;;

        cpu)
            run_cpu_test
            ;;

        network)
            run_network_test
            ;;

        driver)
            run_driver_test
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
