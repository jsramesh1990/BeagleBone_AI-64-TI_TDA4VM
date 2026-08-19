/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter
 *
 * Userspace Benchmark Utility
 *
 * File:
 *     userspace/tools/benchmark.c
 *
 * Purpose:
 *     Measure the userspace packet-filter API performance.
 *
 * Measures:
 *     - Rule insertion time
 *     - Rule lookup time
 *     - Rule deletion time
 *     - Packet processing API throughput
 *     - Statistics read latency
 *
 * NOTE:
 *     This benchmark measures the userspace/ioctl path.
 *     It is NOT a replacement for real NIC line-rate testing.
 *
 * ============================================================
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/libfilter.h"


/*
 * ============================================================
 * Benchmark Configuration
 * ============================================================
 */

#define DEFAULT_ITERATIONS        1000
#define DEFAULT_RULE_COUNT        100
#define DEFAULT_PACKET_COUNT      100000

#define MAX_RULE_COUNT             FILTER_MAX_RULES


/*
 * ============================================================
 * Timing Helpers
 * ============================================================
 */

static uint64_t timespec_to_ns(
        const struct timespec *ts)
{
    return
        ((uint64_t)ts->tv_sec * 1000000000ULL) +
        (uint64_t)ts->tv_nsec;
}


static uint64_t get_time_ns(void)
{
    struct timespec ts;

    if (clock_gettime(
            CLOCK_MONOTONIC,
            &ts) != 0) {

        return 0;
    }

    return timespec_to_ns(&ts);
}


static double ns_to_us(
        uint64_t ns)
{
    return
        (double)ns / 1000.0;
}


static double ns_to_ms(
        uint64_t ns)
{
    return
        (double)ns / 1000000.0;
}


/*
 * ============================================================
 * Banner
 * ============================================================
 */

static void print_banner(void)
{
    printf(
        "\n"
        "============================================================\n"
        " BeagleBone AI-64 - TI TDA4VM\n"
        " Packet Filter Performance Benchmark\n"
        "============================================================\n"
        "\n");
}


/*
 * ============================================================
 * Usage
 * ============================================================
 */

static void print_usage(
        const char *program)
{
    printf(
        "Usage:\n"
        "\n"
        "  %s [options]\n"
        "\n"
        "Options:\n"
        "\n"
        "  -n <count>     Number of packet iterations\n"
        "  -r <count>     Number of rules\n"
        "  -p <count>     Number of packet-processing tests\n"
        "  -h             Show help\n"
        "\n"
        "Defaults:\n"
        "\n"
        "  iterations     : %d\n"
        "  rules           : %d\n"
        "  packets         : %d\n"
        "\n"
        "Example:\n"
        "\n"
        "  sudo %s -n 10000 -r 100 -p 100000\n"
        "\n",
        program,
        DEFAULT_ITERATIONS,
        DEFAULT_RULE_COUNT,
        DEFAULT_PACKET_COUNT,
        program);
}


/*
 * ============================================================
 * Rule Generator
 * ============================================================
 */

static void create_test_rule(
        filter_rule_t *rule,
        uint32_t index)
{
    uint32_t source_host;

    if (rule == NULL) {
        return;
    }

    filter_rule_init(rule);

    /*
     * Generate addresses:
     *
     *     192.168.10.1
     *     192.168.10.2
     *     ...
     */

    source_host =
        100 + (index % 100);

    rule->src_ip =
        htonl(
            (192U << 24) |
            (168U << 16) |
            (10U << 8) |
            source_host);

    rule->src_ip_mask =
        htonl(0xffffffffU);

    rule->dst_ip =
        htonl(
            (192U << 24) |
            (168U << 16) |
            (1U << 8) |
            10U);

    rule->dst_ip_mask =
        htonl(0xffffffffU);

    rule->src_port =
        (uint16_t)(10000 + (index % 1000));

    rule->dst_port =
        8080;

    rule->protocol =
        FILTER_PROTOCOL_TCP;

    /*
     * Alternate actions so that the rule engine sees
     * different decisions during testing.
     */

    switch (index % 3) {

    case 0:
        rule->action =
            FILTER_ACTION_ALLOW;
        break;

    case 1:
        rule->action =
            FILTER_ACTION_DROP;
        break;

    default:
        rule->action =
            FILTER_ACTION_MONITOR;
        break;
    }

    rule->priority =
        (uint16_t)(index + 1);

    rule->enabled =
        1;
}


/*
 * ============================================================
 * Packet Generator
 * ============================================================
 */

static void create_test_packet(
        filter_packet_info_t *packet,
        uint32_t index)
{
    uint32_t source_host;

    if (packet == NULL) {
        return;
    }

    memset(
        packet,
        0,
        sizeof(*packet));

    source_host =
        100 + (index % 100);

    packet->src_ip =
        htonl(
            (192U << 24) |
            (168U << 16) |
            (10U << 8) |
            source_host);

    packet->dst_ip =
        htonl(
            (192U << 24) |
            (168U << 16) |
            (1U << 8) |
            10U);

    packet->src_port =
        (uint16_t)(10000 + (index % 1000));

    packet->dst_port =
        8080;

    packet->protocol =
        FILTER_PROTOCOL_TCP;

    packet->packet_length =
        1500;
}


/*
 * ============================================================
 * Rule Insertion Benchmark
 * ============================================================
 */

static int benchmark_rule_add(
        uint32_t rule_count)
{
    filter_rule_t rule;

    uint32_t i;

    uint32_t successful = 0;

    uint64_t start;

    uint64_t end;

    uint64_t elapsed;

    double average_us;

    double rate;

    printf(
        "\n"
        "[1] Rule Addition Benchmark\n");

    printf(
        "    Rules: %u\n",
        rule_count);

    start =
        get_time_ns();

    for (i = 0; i < rule_count; i++) {

        create_test_rule(
            &rule,
            i);

        if (filter_add_rule(
                &rule) == FILTER_SUCCESS) {

            successful++;

        } else {

            /*
             * Rule insertion can fail if the driver or
             * rule table has limitations.
             */
        }
    }

    end =
        get_time_ns();

    elapsed =
        end - start;

    if (successful == 0) {

        printf(
            "    No rules were inserted.\n");

        return 1;
    }

    average_us =
        ns_to_us(elapsed) /
        (double)successful;

    rate =
        ((double)successful * 1000000000.0) /
        (double)elapsed;

    printf(
        "    Successful : %u\n",
        successful);

    printf(
        "    Total time : %.3f ms\n",
        ns_to_ms(elapsed));

    printf(
        "    Average    : %.3f us/rule\n",
        average_us);

    printf(
        "    Throughput : %.2f rules/sec\n",
        rate);

    return 0;
}


/*
 * ============================================================
 * Rule Lookup Benchmark
 * ============================================================
 */

static int benchmark_rule_lookup(
        uint32_t iterations,
        uint32_t rule_count)
{
    filter_rule_t rule;

    uint32_t i;

    uint32_t successful = 0;

    uint32_t rule_id;

    uint64_t start;

    uint64_t end;

    uint64_t elapsed;

    double average_us;

    double rate;

    printf(
        "\n"
        "[2] Rule Lookup Benchmark\n");

    printf(
        "    Iterations: %u\n",
        iterations);

    if (rule_count == 0) {
        return 1;
    }

    start =
        get_time_ns();

    for (i = 0; i < iterations; i++) {

        rule_id =
            (i % rule_count) + 1;

        memset(
            &rule,
            0,
            sizeof(rule));

        if (filter_get_rule(
                rule_id,
                &rule) == FILTER_SUCCESS) {

            successful++;
        }
    }

    end =
        get_time_ns();

    elapsed =
        end - start;

    if (successful == 0) {

        printf(
            "    No successful lookups.\n");

        return 1;
    }

    average_us =
        ns_to_us(elapsed) /
        (double)successful;

    rate =
        ((double)successful * 1000000000.0) /
        (double)elapsed;

    printf(
        "    Successful : %u\n",
        successful);

    printf(
        "    Total time : %.3f ms\n",
        ns_to_ms(elapsed));

    printf(
        "    Average    : %.3f us/lookup\n",
        average_us);

    printf(
        "    Throughput : %.2f lookups/sec\n",
        rate);

    return 0;
}


/*
 * ============================================================
 * Packet Processing Benchmark
 * ============================================================
 */

static int benchmark_packet_processing(
        uint32_t packet_count)
{
    filter_packet_info_t packet;

    filter_decision_t decision;

    uint32_t i;

    uint32_t successful = 0;

    uint64_t start;

    uint64_t end;

    uint64_t elapsed;

    double average_ns;

    double rate;

    printf(
        "\n"
        "[3] Packet Processing Benchmark\n");

    printf(
        "    Packets: %u\n",
        packet_count);

    start =
        get_time_ns();

    for (i = 0; i < packet_count; i++) {

        create_test_packet(
            &packet,
            i);

        memset(
            &decision,
            0,
            sizeof(decision));

        if (filter_process_packet(
                &packet,
                &decision) ==
            FILTER_SUCCESS) {

            successful++;
        }
    }

    end =
        get_time_ns();

    elapsed =
        end - start;

    if (successful == 0) {

        printf(
            "    No successful packet operations.\n");

        return 1;
    }

    average_ns =
        (double)elapsed /
        (double)successful;

    rate =
        ((double)successful * 1000000000.0) /
        (double)elapsed;

    printf(
        "    Successful : %u\n",
        successful);

    printf(
        "    Total time : %.3f ms\n",
        ns_to_ms(elapsed));

    printf(
        "    Average    : %.2f ns/packet\n",
        average_ns);

    printf(
        "    Throughput : %.2f packets/sec\n",
        rate);

    printf(
        "    Throughput : %.2f Mpps\n",
        rate / 1000000.0);

    return 0;
}


/*
 * ============================================================
 * Statistics Benchmark
 * ============================================================
 */

static int benchmark_statistics(
        uint32_t iterations)
{
    filter_statistics_t statistics;

    uint32_t i;

    uint32_t successful = 0;

    uint64_t start;

    uint64_t end;

    uint64_t elapsed;

    double average_us;

    printf(
        "\n"
        "[4] Statistics Read Benchmark\n");

    printf(
        "    Iterations: %u\n",
        iterations);

    start =
        get_time_ns();

    for (i = 0; i < iterations; i++) {

        if (filter_get_statistics(
                &statistics) ==
            FILTER_SUCCESS) {

            successful++;
        }
    }

    end =
        get_time_ns();

    elapsed =
        end - start;

    if (successful == 0) {

        printf(
            "    Statistics read failed.\n");

        return 1;
    }

    average_us =
        ns_to_us(elapsed) /
        (double)successful;

    printf(
        "    Successful : %u\n",
        successful);

    printf(
        "    Total time : %.3f ms\n",
        ns_to_ms(elapsed));

    printf(
        "    Average    : %.3f us/read\n",
        average_us);

    return 0;
}


/*
 * ============================================================
 * Rule Enumeration Benchmark
 * ============================================================
 */

static int benchmark_rule_enumeration(
        uint32_t iterations)
{
    filter_rule_t rules[
        FILTER_MAX_RULES];

    uint32_t count;

    uint32_t i;

    uint32_t successful = 0;

    uint64_t start;

    uint64_t end;

    uint64_t elapsed;

    double average_us;

    printf(
        "\n"
        "[5] Rule Enumeration Benchmark\n");

    printf(
        "    Iterations: %u\n",
        iterations);

    start =
        get_time_ns();

    for (i = 0; i < iterations; i++) {

        count =
            FILTER_MAX_RULES;

        if (filter_get_rules(
                rules,
                &count) ==
            FILTER_SUCCESS) {

            successful++;
        }
    }

    end =
        get_time_ns();

    elapsed =
        end - start;

    if (successful == 0) {

        printf(
            "    Rule enumeration failed.\n");

        return 1;
    }

    average_us =
        ns_to_us(elapsed) /
        (double)successful;

    printf(
        "    Successful : %u\n",
        successful);

    printf(
        "    Total time : %.3f ms\n",
        ns_to_ms(elapsed));

    printf(
        "    Average    : %.3f us/read\n",
        average_us);

    return 0;
}


/*
 * ============================================================
 * Rule Deletion Benchmark
 * ============================================================
 */

static int benchmark_rule_delete(
        uint32_t rule_count)
{
    uint32_t i;

    uint32_t successful = 0;

    uint64_t start;

    uint64_t end;

    uint64_t elapsed;

    double average_us;

    double rate;

    printf(
        "\n"
        "[6] Rule Deletion Benchmark\n");

    printf(
        "    Rules: %u\n",
        rule_count);

    if (rule_count == 0) {
        return 1;
    }

    start =
        get_time_ns();

    for (i = 0; i < rule_count; i++) {

        if (filter_remove_rule(
                i + 1) ==
            FILTER_SUCCESS) {

            successful++;
        }
    }

    end =
        get_time_ns();

    elapsed =
        end - start;

    if (successful == 0) {

        printf(
            "    No rules deleted.\n");

        return 1;
    }

    average_us =
        ns_to_us(elapsed) /
        (double)successful;

    rate =
        ((double)successful * 1000000000.0) /
        (double)elapsed;

    printf(
        "    Successful : %u\n",
        successful);

    printf(
        "    Total time : %.3f ms\n",
        ns_to_ms(elapsed));

    printf(
        "    Average    : %.3f us/rule\n",
        average_us);

    printf(
        "    Throughput : %.2f deletes/sec\n",
        rate);

    return 0;
}


/*
 * ============================================================
 * Benchmark Summary
 * ============================================================
 */

static void print_summary(void)
{
    printf(
        "\n"
        "============================================================\n"
        " Benchmark Completed\n"
        "============================================================\n"
        "\n"
        "Important:\n"
        "\n"
        "This benchmark measures the userspace API and ioctl path.\n"
        "It does not represent physical Ethernet line-rate performance.\n"
        "\n"
        "For actual packet-filter performance testing, use:\n"
        "\n"
        "    tests/performance/packet_generator\n"
        "\n"
        "together with a real network interface and packet capture.\n"
        "\n");
}


/*
 * ============================================================
 * Main
 * ============================================================
 */

int main(
        int argc,
        char **argv)
{
    uint32_t iterations =
        DEFAULT_ITERATIONS;

    uint32_t rule_count =
        DEFAULT_RULE_COUNT;

    uint32_t packet_count =
        DEFAULT_PACKET_COUNT;

    int option;

    int ret;

    print_banner();

    /*
     * Command-line options.
     */

    while ((option =
            getopt(
                argc,
                argv,
                "n:r:p:h")) != -1) {

        switch (option) {

        case 'n': {

            unsigned long value;

            char *end;

            errno = 0;

            value =
                strtoul(
                    optarg,
                    &end,
                    10);

            if (errno != 0 ||
                end == optarg ||
                *end != '\0' ||
                value == 0 ||
                value > UINT32_MAX) {

                fprintf(
                    stderr,
                    "ERROR: invalid iteration count\n");

                return 1;
            }

            iterations =
                (uint32_t)value;

            break;
        }

        case 'r': {

            unsigned long value;

            char *end;

            errno = 0;

            value =
                strtoul(
                    optarg,
                    &end,
                    10);

            if (errno != 0 ||
                end == optarg ||
                *end != '\0' ||
                value == 0 ||
                value > MAX_RULE_COUNT) {

                fprintf(
                    stderr,
                    "ERROR: invalid rule count "
                    "(1-%d)\n",
                    MAX_RULE_COUNT);

                return 1;
            }

            rule_count =
                (uint32_t)value;

            break;
        }

        case 'p': {

            unsigned long value;

            char *end;

            errno = 0;

            value =
                strtoul(
                    optarg,
                    &end,
                    10);

            if (errno != 0 ||
                end == optarg ||
                *end != '\0' ||
                value == 0 ||
                value > UINT32_MAX) {

                fprintf(
                    stderr,
                    "ERROR: invalid packet count\n");

                return 1;
            }

            packet_count =
                (uint32_t)value;

            break;
        }

        case 'h':

            print_usage(
                argv[0]);

            return 0;

        default:

            print_usage(
                argv[0]);

            return 1;
        }
    }

    /*
     * Initialize library and open the driver.
     */

    ret =
        filter_init();

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "\n"
            "ERROR: unable to initialize packet filter.\n"
            "\n"
            "Make sure the driver is loaded:\n"
            "\n"
            "    sudo ./scripts/load_driver.sh\n"
            "\n"
            "Then verify:\n"
            "\n"
            "    ls -l /dev/packet_filter\n"
            "\n");

        return 1;
    }

    /*
     * Start benchmark.
     */

    printf(
        "Benchmark configuration:\n"
        "    Rule count    : %u\n"
        "    Iterations    : %u\n"
        "    Packet count  : %u\n",
        rule_count,
        iterations,
        packet_count);

    /*
     * Remove old test rules first.
     */

    printf(
        "\nPreparing driver...\n");

    ret =
        filter_clear_rules();

    if (ret != FILTER_SUCCESS) {

        printf(
            "WARNING: unable to clear existing rules: %s\n",
            filter_error_string(ret));
    }

    /*
     * Rule insertion.
     */

    benchmark_rule_add(
        rule_count);

    /*
     * Rule lookup.
     */

    benchmark_rule_lookup(
        iterations,
        rule_count);

    /*
     * Packet processing.
     */

    benchmark_packet_processing(
        packet_count);

    /*
     * Statistics.
     */

    benchmark_statistics(
        iterations);

    /*
     * Rule enumeration.
     */

    benchmark_rule_enumeration(
        iterations);

    /*
     * Rule deletion.
     */

    benchmark_rule_delete(
        rule_count);

    /*
     * Summary.
     */

    print_summary();

    filter_cleanup();

    return 0;
}
