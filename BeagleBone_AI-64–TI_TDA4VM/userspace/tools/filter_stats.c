/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter
 *
 * Userspace Statistics Utility
 *
 * File:
 *     userspace/tools/filter_stats.c
 *
 * Purpose:
 *     Display and manage packet-filter statistics from
 *     userspace.
 *
 * Functions:
 *     - Display packet counters
 *     - Display byte counters
 *     - Display allow/drop/monitor counters
 *     - Display rule statistics
 *     - Reset statistics
 *     - Periodically monitor statistics
 *
 * Usage:
 *
 *     filter_stats
 *     filter_stats show
 *     filter_stats reset
 *     filter_stats monitor
 *     filter_stats monitor <seconds>
 *
 * ============================================================
 */

#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/libfilter.h"


/*
 * ============================================================
 * Configuration
 * ============================================================
 */

#define DEFAULT_MONITOR_INTERVAL  1


/*
 * ============================================================
 * Global State
 * ============================================================
 */

static volatile sig_atomic_t stop_monitoring = 0;


/*
 * ============================================================
 * Signal Handler
 * ============================================================
 */

static void signal_handler(
        int signal_number)
{
    (void)signal_number;

    stop_monitoring = 1;
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
        " Packet Filter Statistics\n"
        "============================================================\n");
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
        "\n"
        "Usage:\n"
        "\n"
        "  %s [command]\n"
        "\n"
        "Commands:\n"
        "\n"
        "  show\n"
        "      Display current packet statistics.\n"
        "\n"
        "  reset\n"
        "      Reset all packet statistics.\n"
        "\n"
        "  monitor\n"
        "      Continuously display statistics.\n"
        "\n"
        "  monitor <seconds>\n"
        "      Display statistics periodically using\n"
        "      the specified interval.\n"
        "\n"
        "  help\n"
        "      Display this help message.\n"
        "\n"
        "Examples:\n"
        "\n"
        "  %s\n"
        "\n"
        "  %s show\n"
        "\n"
        "  %s reset\n"
        "\n"
        "  %s monitor\n"
        "\n"
        "  %s monitor 5\n"
        "\n",
        program,
        program,
        program,
        program,
        program);
}


/*
 * ============================================================
 * Print Main Statistics
 * ============================================================
 */

static void print_statistics(
        const filter_statistics_t *stats)
{
    if (stats == NULL) {
        return;
    }

    printf(
        "\n"
        "Packet Filter Statistics\n");

    printf(
        "------------------------------------------------------------\n");

    printf(
        "Packets received       : %" PRIu64 "\n",
        stats->packets_received);

    printf(
        "Packets processed      : %" PRIu64 "\n",
        stats->packets_processed);

    printf(
        "Packets allowed        : %" PRIu64 "\n",
        stats->packets_allowed);

    printf(
        "Packets dropped        : %" PRIu64 "\n",
        stats->packets_dropped);

    printf(
        "Packets monitored      : %" PRIu64 "\n",
        stats->packets_monitored);

    printf(
        "Packets rejected       : %" PRIu64 "\n",
        stats->packets_rejected);

    printf(
        "Bytes received         : %" PRIu64 "\n",
        stats->bytes_received);

    printf(
        "Bytes processed        : %" PRIu64 "\n",
        stats->bytes_processed);

    printf(
        "Bytes allowed          : %" PRIu64 "\n",
        stats->bytes_allowed);

    printf(
        "Bytes dropped          : %" PRIu64 "\n",
        stats->bytes_dropped);

    printf(
        "Rule matches           : %" PRIu64 "\n",
        stats->rule_matches);

    printf(
        "Rule misses            : %" PRIu64 "\n",
        stats->rule_misses);

    printf(
        "Parser errors          : %" PRIu64 "\n",
        stats->parser_errors);

    printf(
        "Driver errors          : %" PRIu64 "\n",
        stats->driver_errors);

    printf(
        "------------------------------------------------------------\n");
}


/*
 * ============================================================
 * Calculate Packet Drop Rate
 * ============================================================
 */

static void print_rates(
        const filter_statistics_t *stats)
{
    double drop_rate;

    double allow_rate;

    if (stats == NULL) {
        return;
    }

    if (stats->packets_processed == 0) {

        printf(
            "Drop rate              : 0.00 %%\n");

        printf(
            "Allow rate             : 0.00 %%\n");

        return;
    }

    drop_rate =
        ((double)stats->packets_dropped *
         100.0) /
        (double)stats->packets_processed;

    allow_rate =
        ((double)stats->packets_allowed *
         100.0) /
        (double)stats->packets_processed;

    printf(
        "Drop rate              : %.2f %%\n",
        drop_rate);

    printf(
        "Allow rate             : %.2f %%\n",
        allow_rate);
}


/*
 * ============================================================
 * Print Rule Statistics
 * ============================================================
 */

static void print_rule_statistics(void)
{
    filter_rule_t rules[FILTER_MAX_RULES];

    uint32_t count;

    uint32_t i;

    int ret;

    count =
        FILTER_MAX_RULES;

    ret =
        filter_get_rules(
            rules,
            &count);

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "WARNING: unable to retrieve rule statistics: %s\n",
            filter_error_string(ret));

        return;
    }

    printf(
        "\n"
        "Rule Statistics\n");

    printf(
        "------------------------------------------------------------\n");

    printf(
        "%-6s %-10s %-12s %-12s\n",
        "ID",
        "STATUS",
        "MATCHES",
        "ACTION");

    printf(
        "------------------------------------------------------------\n");

    for (i = 0; i < count; i++) {

        printf(
            "%-6u %-10s %-12" PRIu64 " %-12s\n",

            rules[i].rule_id,

            rules[i].enabled
                ? "enabled"
                : "disabled",

            rules[i].match_count,

            filter_action_to_string(
                (filter_action_t)
                    rules[i].action));
    }

    printf(
        "------------------------------------------------------------\n");
}


/*
 * ============================================================
 * Show Statistics
 * ============================================================
 */

static int command_show(void)
{
    filter_statistics_t stats;

    int ret;

    memset(
        &stats,
        0,
        sizeof(stats));

    ret =
        filter_get_statistics(
            &stats);

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "ERROR: failed to read statistics: %s\n",
            filter_error_string(ret));

        return 1;
    }

    print_statistics(
        &stats);

    print_rates(
        &stats);

    print_rule_statistics();

    return 0;
}


/*
 * ============================================================
 * Reset Statistics
 * ============================================================
 */

static int command_reset(void)
{
    int ret;

    ret =
        filter_reset_statistics();

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "ERROR: failed to reset statistics: %s\n",
            filter_error_string(ret));

        return 1;
    }

    printf(
        "Packet-filter statistics reset successfully.\n");

    return 0;
}


/*
 * ============================================================
 * Statistics Delta
 * ============================================================
 */

static void print_delta(
        const filter_statistics_t *old_stats,
        const filter_statistics_t *new_stats,
        double elapsed)
{
    uint64_t packets;

    uint64_t bytes;

    double pps;

    double mbps;

    if (old_stats == NULL ||
        new_stats == NULL ||
        elapsed <= 0.0) {

        return;
    }

    packets =
        new_stats->packets_processed -
        old_stats->packets_processed;

    bytes =
        new_stats->bytes_processed -
        old_stats->bytes_processed;

    pps =
        (double)packets /
        elapsed;

    mbps =
        ((double)bytes * 8.0) /
        elapsed /
        1000000.0;

    printf(
        "\n"
        "Interval Performance\n");

    printf(
        "------------------------------------------------------------\n");

    printf(
        "Packets processed     : %" PRIu64 "\n",
        packets);

    printf(
        "Bytes processed       : %" PRIu64 "\n",
        bytes);

    printf(
        "Packet rate            : %.2f packets/sec\n",
        pps);

    printf(
        "Data rate              : %.2f Mbps\n",
        mbps);

    printf(
        "------------------------------------------------------------\n");
}


/*
 * ============================================================
 * Monitor Statistics
 * ============================================================
 */

static int command_monitor(
        unsigned int interval)
{
    filter_statistics_t previous;

    filter_statistics_t current;

    struct timespec start_time;

    struct timespec end_time;

    double elapsed;

    int ret;

    memset(
        &previous,
        0,
        sizeof(previous));

    memset(
        &current,
        0,
        sizeof(current));

    /*
     * Install signal handlers so Ctrl+C cleanly
     * terminates monitoring.
     */

    signal(
        SIGINT,
        signal_handler);

    signal(
        SIGTERM,
        signal_handler);

    ret =
        filter_get_statistics(
            &previous);

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "ERROR: failed to read statistics: %s\n",
            filter_error_string(ret));

        return 1;
    }

    printf(
        "\n"
        "Monitoring packet statistics.\n"
        "Interval: %u second(s)\n"
        "Press Ctrl+C to stop.\n",
        interval);

    while (!stop_monitoring) {

        sleep(interval);

        if (stop_monitoring) {
            break;
        }

        if (clock_gettime(
                CLOCK_MONOTONIC,
                &start_time) != 0) {

            perror(
                "clock_gettime");

            break;
        }

        ret =
            filter_get_statistics(
                &current);

        if (ret != FILTER_SUCCESS) {

            fprintf(
                stderr,
                "ERROR: failed to read statistics: %s\n",
                filter_error_string(ret));

            continue;
        }

        if (clock_gettime(
                CLOCK_MONOTONIC,
                &end_time) != 0) {

            perror(
                "clock_gettime");

            break;
        }

        elapsed =
            (double)(
                end_time.tv_sec -
                start_time.tv_sec);

        elapsed +=
            (double)(
                end_time.tv_nsec -
                start_time.tv_nsec)
            / 1000000000.0;

        /*
         * Avoid zero/negative elapsed values.
         */

        if (elapsed <= 0.0) {
            elapsed =
                (double)interval;
        }

        printf(
            "\033[2J\033[H");

        print_banner();

        print_statistics(
            &current);

        print_rates(
            &current);

        print_delta(
            &previous,
            &current,
            elapsed);

        previous =
            current;
    }

    printf(
        "\n"
        "Statistics monitoring stopped.\n");

    return 0;
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
    const char *command =
        "show";

    unsigned int interval =
        DEFAULT_MONITOR_INTERVAL;

    char *end;

    unsigned long value;

    int ret;

    print_banner();

    /*
     * Command.
     */

    if (argc >= 2) {

        command =
            argv[1];
    }

    /*
     * Help.
     */

    if (strcmp(
            command,
            "help") == 0 ||
        strcmp(
            command,
            "--help") == 0 ||
        strcmp(
            command,
            "-h") == 0) {

        print_usage(
            argv[0]);

        return 0;
    }

    /*
     * Monitor interval.
     */

    if (strcmp(
            command,
            "monitor") == 0 &&
        argc >= 3) {

        errno = 0;

        value =
            strtoul(
                argv[2],
                &end,
                10);

        if (errno != 0 ||
            end == argv[2] ||
            *end != '\0' ||
            value == 0 ||
            value > 3600) {

            fprintf(
                stderr,
                "ERROR: invalid monitor interval\n");

            return 1;
        }

        interval =
            (unsigned int)value;
    }

    /*
     * Initialize libfilter.
     */

    ret =
        filter_init();

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "\n"
            "ERROR: unable to initialize packet filter.\n"
            "\n"
            "Make sure the kernel driver is loaded:\n"
            "\n"
            "    sudo ./scripts/load_driver.sh\n"
            "\n"
            "Check the device:\n"
            "\n"
            "    ls -l /dev/packet_filter\n"
            "\n");

        return 1;
    }

    /*
     * Command dispatch.
     */

    if (strcmp(
            command,
            "show") == 0) {

        ret =
            command_show();

    } else if (strcmp(
            command,
            "reset") == 0) {

        ret =
            command_reset();

    } else if (strcmp(
            command,
            "monitor") == 0) {

        ret =
            command_monitor(
                interval);

    } else {

        fprintf(
            stderr,
            "ERROR: unknown command '%s'\n",
            command);

        print_usage(
            argv[0]);

        ret = 1;
    }

    /*
     * Cleanup.
     */

    filter_cleanup();

    return ret;
}
