#!/bin/bash
#
# ============================================================
# BeagleBone AI-64 - TI TDA4VM
# Packet Filter Project
#
# Performance Test Script
#
# Measures:
#   - CPU usage
#   - Memory usage
#   - Network throughput
#   - Packet rate
#   - Packet drops
#   - Driver statistics
#   - Interrupt activity
#   - SoftIRQ activity
#   - Load average
#   - Packet-filter processing performance
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

BUILD_DIR="${PROJECT_ROOT}/build"

PERF_DIR="${BUILD_DIR}/performance"

TIMESTAMP="$(date '+%Y%m%d_%H%M%S')"

RESULT_DIR="${PERF_DIR}/${TIMESTAMP}"

RESULT_FILE="${RESULT_DIR}/performance.txt"

DURATION="${DURATION:-30}"

INTERFACE="${INTERFACE:-}"

IPERF_SERVER="${IPERF_SERVER:-}"

IPERF_PORT="${IPERF_PORT:-5201}"

PACKET_SIZE="${PACKET_SIZE:-1024}"


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
    echo "       Packet Filter Performance Test"
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
        warning "Some performance counters require root privileges."
        warning "Recommended:"
        echo
        echo "  sudo ./scripts/perf_test.sh"
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

    success "Performance result directory:"
    echo "  ${RESULT_DIR}"
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

    INTERFACE="$(ip -o link show | \
        awk -F': ' '$2 != "lo" {print $2}' | \
        head -1)"
}


# ------------------------------------------------------------
# System Information
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
        echo "Load Average:"
        uptime

        echo
        echo "Network Interface:"
        echo "${INTERFACE}"

        echo
        echo "IP Address:"
        ip addr show "${INTERFACE}" 2>/dev/null || true

        echo
        echo "============================================================"

    } >> "${RESULT_FILE}"

    success "System information collected."
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
    else
        warning "${MODULE_NAME} is not loaded."
        warning "Performance test will continue with system/network measurements."
    fi
}


# ------------------------------------------------------------
# CPU Usage
# ------------------------------------------------------------

measure_cpu()
{
    info "Measuring CPU usage for ${DURATION} seconds..."

    {
        echo
        echo "============================================================"
        echo "CPU PERFORMANCE"
        echo "============================================================"

        echo
        echo "vmstat:"
        vmstat 1 "${DURATION}"

        echo
        echo "mpstat:"
        if command -v mpstat >/dev/null 2>&1
        then
            mpstat -P ALL 1 "${DURATION}"
        else
            echo "mpstat not installed."
        fi

    } >> "${RESULT_FILE}"

    success "CPU performance measurement completed."
}


# ------------------------------------------------------------
# Memory Usage
# ------------------------------------------------------------

measure_memory()
{
    info "Measuring memory usage..."

    {
        echo
        echo "============================================================"
        echo "MEMORY PERFORMANCE"
        echo "============================================================"

        echo
        echo "Memory before test:"
        free -h

        echo
        echo "/proc/meminfo:"
        cat /proc/meminfo

        echo
        echo "Slab information:"
        if [ -f /proc/slabinfo ]
        then
            head -30 /proc/slabinfo
        fi

    } >> "${RESULT_FILE}"

    success "Memory information collected."
}


# ------------------------------------------------------------
# Network Statistics
# ------------------------------------------------------------

network_statistics()
{
    info "Collecting network statistics..."

    {
        echo
        echo "============================================================"
        echo "NETWORK STATISTICS"
        echo "============================================================"

        echo
        echo "Interface:"
        ip -s link show "${INTERFACE}" 2>/dev/null || true

        echo
        echo "ethtool statistics:"
        if command -v ethtool >/dev/null 2>&1
        then
            ethtool -S "${INTERFACE}" 2>/dev/null || true
        fi

        echo
        echo "/proc/net/dev:"
        cat /proc/net/dev

        echo
        echo "Socket statistics:"
        ss -s

    } >> "${RESULT_FILE}"

    success "Network statistics collected."
}


# ------------------------------------------------------------
# Capture Network Counters
# ------------------------------------------------------------

read_interface_counters()
{
    local interface="$1"

    local rx_bytes
    local tx_bytes
    local rx_packets
    local tx_packets
    local rx_dropped
    local tx_dropped

    rx_bytes="$(cat "/sys/class/net/${interface}/statistics/rx_bytes" 2>/dev/null || echo 0)"
    tx_bytes="$(cat "/sys/class/net/${interface}/statistics/tx_bytes" 2>/dev/null || echo 0)"
    rx_packets="$(cat "/sys/class/net/${interface}/statistics/rx_packets" 2>/dev/null || echo 0)"
    tx_packets="$(cat "/sys/class/net/${interface}/statistics/tx_packets" 2>/dev/null || echo 0)"
    rx_dropped="$(cat "/sys/class/net/${interface}/statistics/rx_dropped" 2>/dev/null || echo 0)"
    tx_dropped="$(cat "/sys/class/net/${interface}/statistics/tx_dropped" 2>/dev/null || echo 0)"

    echo "${rx_bytes} ${tx_bytes} ${rx_packets} ${tx_packets} ${rx_dropped} ${tx_dropped}"
}


# ------------------------------------------------------------
# Packet Rate Test
# ------------------------------------------------------------

measure_packet_rate()
{
    info "Measuring packet rate for ${DURATION} seconds..."

    if [ ! -d "/sys/class/net/${INTERFACE}" ]
    then
        warning "Network interface ${INTERFACE} not found."
        return
    fi

    read \
        RX_BYTES_START \
        TX_BYTES_START \
        RX_PACKETS_START \
        TX_PACKETS_START \
        RX_DROPPED_START \
        TX_DROPPED_START \
        <<< "$(read_interface_counters "${INTERFACE}")"


    sleep "${DURATION}"


    read \
        RX_BYTES_END \
        TX_BYTES_END \
        RX_PACKETS_END \
        TX_PACKETS_END \
        RX_DROPPED_END \
        TX_DROPPED_END \
        <<< "$(read_interface_counters "${INTERFACE}")"


    RX_BYTES_DIFF=$((RX_BYTES_END - RX_BYTES_START))
    TX_BYTES_DIFF=$((TX_BYTES_END - TX_BYTES_START))

    RX_PACKETS_DIFF=$((RX_PACKETS_END - RX_PACKETS_START))
    TX_PACKETS_DIFF=$((TX_PACKETS_END - TX_PACKETS_START))

    RX_DROPPED_DIFF=$((RX_DROPPED_END - RX_DROPPED_START))
    TX_DROPPED_DIFF=$((TX_DROPPED_END - TX_DROPPED_START))


    RX_PPS=$((RX_PACKETS_DIFF / DURATION))
    TX_PPS=$((TX_PACKETS_DIFF / DURATION))

    RX_BPS=$((RX_BYTES_DIFF / DURATION))
    TX_BPS=$((TX_BYTES_DIFF / DURATION))


    {
        echo
        echo "============================================================"
        echo "PACKET RATE"
        echo "============================================================"

        echo
        echo "Test duration:"
        echo "  ${DURATION} seconds"

        echo
        echo "RX packets:"
        echo "  ${RX_PACKETS_DIFF}"

        echo
        echo "TX packets:"
        echo "  ${TX_PACKETS_DIFF}"

        echo
        echo "RX packets/sec:"
        echo "  ${RX_PPS}"

        echo
        echo "TX packets/sec:"
        echo "  ${TX_PPS}"

        echo
        echo "RX bytes:"
        echo "  ${RX_BYTES_DIFF}"

        echo
        echo "TX bytes:"
        echo "  ${TX_BYTES_DIFF}"

        echo
        echo "RX bytes/sec:"
        echo "  ${RX_BPS}"

        echo
        echo "TX bytes/sec:"
        echo "  ${TX_BPS}"

        echo
        echo "RX dropped:"
        echo "  ${RX_DROPPED_DIFF}"

        echo
        echo "TX dropped:"
        echo "  ${TX_DROPPED_DIFF}"

    } >> "${RESULT_FILE}"

    success "Packet-rate measurement completed."
}


# ------------------------------------------------------------
# Iperf3 Throughput Test
# ------------------------------------------------------------

iperf_test()
{
    if [ -z "${IPERF_SERVER}" ]
    then
        warning "IPERF_SERVER not specified."
        warning "Skipping iperf3 throughput test."
        return
    fi


    if ! command -v iperf3 >/dev/null 2>&1
    then
        warning "iperf3 is not installed."
        warning "Skipping throughput test."
        return
    fi


    info "Running iperf3 throughput test..."

    {
        echo
        echo "============================================================"
        echo "IPERF3 THROUGHPUT"
        echo "============================================================"

        echo
        echo "Server:"
        echo "  ${IPERF_SERVER}"

        echo
        echo "Port:"
        echo "  ${IPERF_PORT}"

        echo
        echo "Duration:"
        echo "  ${DURATION} seconds"

        echo

        iperf3 \
            -c "${IPERF_SERVER}" \
            -p "${IPERF_PORT}" \
            -t "${DURATION}"

    } >> "${RESULT_FILE}" 2>&1 || true

    success "iperf3 test completed."
}


# ------------------------------------------------------------
# UDP Packet Test
# ------------------------------------------------------------

udp_test()
{
    if [ -z "${IPERF_SERVER}" ]
    then
        warning "IPERF_SERVER not specified."
        return
    fi


    if ! command -v iperf3 >/dev/null 2>&1
    then
        warning "iperf3 not installed."
        return
    fi


    info "Running UDP packet-filter test..."

    {
        echo
        echo "============================================================"
        echo "UDP PERFORMANCE"
        echo "============================================================"

        echo
        echo "Packet size:"
        echo "  ${PACKET_SIZE} bytes"

        echo

        iperf3 \
            -c "${IPERF_SERVER}" \
            -p "${IPERF_PORT}" \
            -u \
            -l "${PACKET_SIZE}" \
            -t "${DURATION}"

    } >> "${RESULT_FILE}" 2>&1 || true

    success "UDP performance test completed."
}


# ------------------------------------------------------------
# Interrupt Statistics
# ------------------------------------------------------------

measure_interrupts()
{
    info "Collecting interrupt statistics..."

    {
        echo
        echo "============================================================"
        echo "INTERRUPT PERFORMANCE"
        echo "============================================================"

        echo
        echo "/proc/interrupts:"
        cat /proc/interrupts

        echo
        echo "Network-related interrupts:"
        grep -i \
            -E "eth|net|cpsw|dma" \
            /proc/interrupts \
            2>/dev/null || true

    } >> "${RESULT_FILE}"

    success "Interrupt statistics collected."
}


# ------------------------------------------------------------
# SoftIRQ Statistics
# ------------------------------------------------------------

measure_softirq()
{
    info "Collecting SoftIRQ statistics..."

    {
        echo
        echo "============================================================"
        echo "SOFTIRQ PERFORMANCE"
        echo "============================================================"

        echo
        echo "/proc/softirqs:"
        cat /proc/softirqs

    } >> "${RESULT_FILE}"

    success "SoftIRQ statistics collected."
}


# ------------------------------------------------------------
# Driver Statistics
# ------------------------------------------------------------

driver_statistics()
{
    info "Collecting packet-filter driver statistics..."

    {
        echo
        echo "============================================================"
        echo "PACKET FILTER DRIVER"
        echo "============================================================"

        echo
        echo "Module:"
        lsmod | grep "^${MODULE_NAME}" || \
            echo "packet_filter is not loaded."

        echo
        echo "Module information:"
        modinfo "${MODULE_NAME}" 2>/dev/null || true

        echo
        echo "Kernel messages:"
        dmesg | grep -i \
            -E "packet_filter|packet-filter|packet filter" \
            | tail -100 || true

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

    } >> "${RESULT_FILE}"

    success "Driver statistics collected."
}


# ------------------------------------------------------------
# CPU Frequency
# ------------------------------------------------------------

measure_cpu_frequency()
{
    info "Collecting CPU frequency information..."

    {
        echo
        echo "============================================================"
        echo "CPU FREQUENCY"
        echo "============================================================"

        for cpu in /sys/devices/system/cpu/cpu[0-9]*
        do

            if [ -f "${cpu}/cpufreq/scaling_cur_freq" ]
            then

                echo
                echo "$(basename "${cpu}"):"
                cat "${cpu}/cpufreq/scaling_cur_freq"

            fi

        done

    } >> "${RESULT_FILE}"

    success "CPU frequency information collected."
}


# ------------------------------------------------------------
# Network Link Information
# ------------------------------------------------------------

network_link_info()
{
    info "Collecting network link information..."

    {
        echo
        echo "============================================================"
        echo "NETWORK LINK"
        echo "============================================================"

        echo
        echo "Interface:"
        ip link show "${INTERFACE}" 2>/dev/null || true

        echo
        echo "Address:"
        ip addr show "${INTERFACE}" 2>/dev/null || true

        if command -v ethtool >/dev/null 2>&1
        then

            echo
            echo "Link:"
            ethtool "${INTERFACE}" 2>/dev/null || true

            echo
            echo "Driver:"
            ethtool -i "${INTERFACE}" 2>/dev/null || true

        fi

    } >> "${RESULT_FILE}"

    success "Network link information collected."
}


# ------------------------------------------------------------
# Process Statistics
# ------------------------------------------------------------

process_statistics()
{
    info "Collecting process statistics..."

    {
        echo
        echo "============================================================"
        echo "PROCESS INFORMATION"
        echo "============================================================"

        echo
        echo "Top CPU processes:"
        ps -eo pid,ppid,comm,%cpu,%mem --sort=-%cpu | head -20

        echo
        echo "Top memory processes:"
        ps -eo pid,ppid,comm,%cpu,%mem --sort=-%mem | head -20

    } >> "${RESULT_FILE}"

    success "Process statistics collected."
}


# ------------------------------------------------------------
# Generate Performance Summary
# ------------------------------------------------------------

generate_summary()
{
    info "Generating performance summary..."

    {
        echo
        echo "============================================================"
        echo "PERFORMANCE TEST SUMMARY"
        echo "============================================================"

        echo
        echo "Test Date:"
        date

        echo
        echo "Duration:"
        echo "${DURATION} seconds"

        echo
        echo "Interface:"
        echo "${INTERFACE}"

        echo
        echo "Kernel:"
        uname -r

        echo
        echo "Architecture:"
        uname -m

        echo
        echo "Packet Filter:"
        if lsmod | awk '{print $1}' | grep -qx "${MODULE_NAME}"
        then
            echo "LOADED"
        else
            echo "NOT LOADED"
        fi

        echo
        echo "Result:"
        echo "${RESULT_FILE}"

        echo
        echo "============================================================"

    } >> "${RESULT_FILE}"

    success "Performance summary generated."
}


# ------------------------------------------------------------
# Display Result
# ------------------------------------------------------------

display_result()
{
    echo
    echo "============================================================"
    echo "              PERFORMANCE TEST RESULT"
    echo "============================================================"

    echo
    echo "Result file:"
    echo "  ${RESULT_FILE}"

    echo
    echo "Test duration:"
    echo "  ${DURATION} seconds"

    echo
    echo "Interface:"
    echo "  ${INTERFACE}"

    echo
    echo "============================================================"
}


# ------------------------------------------------------------
# Full Performance Test
# ------------------------------------------------------------

run_full_test()
{
    print_banner

    check_root

    detect_interface

    create_result_directory

    check_driver

    collect_system_info

    network_link_info

    measure_memory

    measure_cpu_frequency

    network_statistics

    measure_interrupts

    measure_softirq

    process_statistics

    driver_statistics

    measure_packet_rate

    iperf_test

    udp_test

    generate_summary

    display_result

    echo

    success "Performance test completed successfully."
    echo
}


# ------------------------------------------------------------
# Quick Test
# ------------------------------------------------------------

run_quick_test()
{
    print_banner

    DURATION="${QUICK_DURATION:-10}"

    detect_interface

    create_result_directory

    check_driver

    network_statistics

    measure_packet_rate

    driver_statistics

    generate_summary

    display_result

    echo

    success "Quick performance test completed."
}


# ------------------------------------------------------------
# CPU Test Only
# ------------------------------------------------------------

run_cpu_test()
{
    print_banner

    create_result_directory

    collect_system_info

    measure_cpu

    measure_memory

    measure_cpu_frequency

    display_result
}


# ------------------------------------------------------------
# Network Test Only
# ------------------------------------------------------------

run_network_test()
{
    print_banner

    detect_interface

    create_result_directory

    network_link_info

    network_statistics

    measure_packet_rate

    iperf_test

    udp_test

    display_result
}


# ------------------------------------------------------------
# Driver Test Only
# ------------------------------------------------------------

run_driver_test()
{
    print_banner

    create_result_directory

    check_driver

    driver_statistics

    measure_interrupts

    measure_softirq

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
    echo "  sudo ./scripts/perf_test.sh"
    echo "  sudo ./scripts/perf_test.sh full"
    echo "  sudo ./scripts/perf_test.sh quick"
    echo "  sudo ./scripts/perf_test.sh cpu"
    echo "  sudo ./scripts/perf_test.sh network"
    echo "  sudo ./scripts/perf_test.sh driver"
    echo "  ./scripts/perf_test.sh help"
    echo
    echo "Environment variables:"
    echo
    echo "  DURATION=30"
    echo "  INTERFACE=eth0"
    echo "  IPERF_SERVER=192.168.1.100"
    echo "  IPERF_PORT=5201"
    echo "  PACKET_SIZE=1024"
    echo
    echo "Examples:"
    echo
    echo "  sudo INTERFACE=eth0 DURATION=60 \\"
    echo "      ./scripts/perf_test.sh"
    echo
    echo "  sudo INTERFACE=eth0 \\"
    echo "      IPERF_SERVER=192.168.1.100 \\"
    echo "      DURATION=60 \\"
    echo "      ./scripts/perf_test.sh network"
    echo
}


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

main()
{
    case "${1:-full}" in

        full)
            run_full_test
            ;;

        quick)
            run_quick_test
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
