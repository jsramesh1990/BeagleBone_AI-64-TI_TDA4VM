/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter
 *
 * Userspace Functional Test Utility
 *
 * File:
 *     userspace/tools/filter_test.c
 *
 * Purpose:
 *     Functional validation of the packet-filter userspace
 *     library and kernel driver.
 *
 * Tests:
 *     1. Driver initialization
 *     2. Driver status
 *     3. Rule creation
 *     4. Rule retrieval
 *     5. Rule listing
 *     6. Rule enable/disable
 *     7. Packet filtering decisions
 *     8. Statistics
 *     9. Monitoring
 *    10. Rule deletion
 *    11. Rule cleanup
 *
 * Usage:
 *
 *     sudo ./filter_test
 *     sudo ./filter_test quick
 *     sudo ./filter_test full
 *
 * ============================================================
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/libfilter.h"


/*
 * ============================================================
 * Test Configuration
 * ============================================================
 */

#define TEST_RULE_ID              1
#define TEST_SOURCE_IP            "192.168.10.100"
#define TEST_DESTINATION_IP       "192.168.1.10"

#define TEST_SOURCE_PORT          12345
#define TEST_DESTINATION_PORT     8080


/*
 * ============================================================
 * Test Counters
 * ============================================================
 */

static unsigned int tests_run;

static unsigned int tests_passed;

static unsigned int tests_failed;


/*
 * ============================================================
 * Test Output
 * ============================================================
 */

static void print_banner(void)
{
    printf(
        "\n"
        "============================================================\n"
        " BeagleBone AI-64 - TI TDA4VM\n"
        " Packet Filter Functional Test\n"
        "============================================================\n"
        "\n");
}


static void print_separator(void)
{
    printf(
        "------------------------------------------------------------\n");
}


static void test_start(
        const char *name)
{
    tests_run++;

    printf(
        "[TEST %02u] %-45s ",
        tests_run,
        name);
}


static void test_pass(void)
{
    tests_passed++;

    printf(
        "[PASS]\n");
}


static void test_fail(void)
{
    tests_failed++;

    printf(
        "[FAIL]\n");
}


/*
 * ============================================================
 * Error Output
 * ============================================================
 */

static void print_filter_error(
        const char *operation,
        int error)
{
    fprintf(
        stderr,
        "\n"
        "       %s failed: %s (%d)\n",
        operation,
        filter_error_string(error),
        error);
}


/*
 * ============================================================
 * Test Rule Generator
 * ============================================================
 */

static void create_test_rule(
        filter_rule_t *rule)
{
    int ret;

    if (rule == NULL) {
        return;
    }

    filter_rule_init(
        rule);

    /*
     * Source IP.
     */

    ret =
        filter_rule_set_source_ip(
            rule,
            TEST_SOURCE_IP);

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "WARNING: unable to configure source IP\n");
    }

    /*
     * Destination IP.
     */

    ret =
        filter_rule_set_destination_ip(
            rule,
            TEST_DESTINATION_IP);

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "WARNING: unable to configure destination IP\n");
    }

    /*
     * TCP ports.
     */

    rule->src_port =
        TEST_SOURCE_PORT;

    rule->dst_port =
        TEST_DESTINATION_PORT;

    /*
     * Protocol.
     */

    rule->protocol =
        FILTER_PROTOCOL_TCP;

    /*
     * Action.
     */

    rule->action =
        FILTER_ACTION_ALLOW;

    /*
     * Priority.
     */

    rule->priority =
        10;

    /*
     * Enabled.
     */

    rule->enabled =
        1;
}


/*
 * ============================================================
 * Test 1 - Initialization
 * ============================================================
 */

static int test_initialization(void)
{
    /*
     * filter_init() is called by main.
     *
     * Reaching this point means initialization succeeded.
     */

    test_start(
        "Library and driver initialization");

    if (!filter_is_open()) {

        test_fail();

        printf(
            "       Driver device is not open.\n");

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 2 - Driver Version
 * ============================================================
 */

static int test_version(void)
{
    char version[128];

    uint32_t major;

    uint32_t minor;

    int ret;

    test_start(
        "Driver/API version query");

    ret =
        filter_get_api_version(
            &major,
            &minor);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "API version query",
            ret);

        test_fail();

        return 1;
    }

    ret =
        filter_get_driver_version(
            version,
            sizeof(version));

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "driver version query",
            ret);

        test_fail();

        return 1;
    }

    printf(
        "\n       API: %u.%u, Driver: %s\n       ",
        major,
        minor,
        version);

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 3 - Interface
 * ============================================================
 */

static int test_interface(void)
{
    char interface[
        FILTER_MAX_INTERFACE_NAME];

    int ret;

    test_start(
        "Network interface query");

    memset(
        interface,
        0,
        sizeof(interface));

    ret =
        filter_get_interface(
            interface,
            sizeof(interface));

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "interface query",
            ret);

        test_fail();

        return 1;
    }

    printf(
        "\n       Interface: %s\n       ",
        interface);

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 4 - Clear Rules
 * ============================================================
 */

static int test_clear_rules(void)
{
    uint32_t count;

    int ret;

    test_start(
        "Clear existing rules");

    ret =
        filter_clear_rules();

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "clear rules",
            ret);

        test_fail();

        return 1;
    }

    ret =
        filter_get_rule_count(
            &count);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "rule count query",
            ret);

        test_fail();

        return 1;
    }

    if (count != 0) {

        fprintf(
            stderr,
            "\n       Rule count is %u, expected 0\n",
            count);

        test_fail();

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 5 - Add Rule
 * ============================================================
 */

static int test_add_rule(
        filter_rule_t *rule)
{
    int ret;

    test_start(
        "Add packet-filter rule");

    create_test_rule(
        rule);

    ret =
        filter_add_rule(
            rule);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "add rule",
            ret);

        test_fail();

        return 1;
    }

    printf(
        "\n       Rule ID: %u\n       ",
        rule->rule_id);

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 6 - Get Rule
 * ============================================================
 */

static int test_get_rule(
        const filter_rule_t *expected)
{
    filter_rule_t actual;

    int ret;

    test_start(
        "Retrieve configured rule");

    memset(
        &actual,
        0,
        sizeof(actual));

    ret =
        filter_get_rule(
            expected->rule_id,
            &actual);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "get rule",
            ret);

        test_fail();

        return 1;
    }

    if (actual.rule_id != expected->rule_id) {

        fprintf(
            stderr,
            "\n       Rule ID mismatch\n");

        test_fail();

        return 1;
    }

    if (actual.src_port !=
        expected->src_port) {

        fprintf(
            stderr,
            "\n       Source port mismatch\n");

        test_fail();

        return 1;
    }

    if (actual.dst_port !=
        expected->dst_port) {

        fprintf(
            stderr,
            "\n       Destination port mismatch\n");

        test_fail();

        return 1;
    }

    if (actual.protocol !=
        expected->protocol) {

        fprintf(
            stderr,
            "\n       Protocol mismatch\n");

        test_fail();

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 7 - Rule Count
 * ============================================================
 */

static int test_rule_count(void)
{
    uint32_t count;

    int ret;

    test_start(
        "Rule count verification");

    ret =
        filter_get_rule_count(
            &count);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "rule count",
            ret);

        test_fail();

        return 1;
    }

    if (count != 1) {

        fprintf(
            stderr,
            "\n       Expected 1 rule, got %u\n",
            count);

        test_fail();

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 8 - Rule Enable
 * ============================================================
 */

static int test_enable_rule(
        uint32_t rule_id)
{
    filter_rule_t rule;

    int ret;

    test_start(
        "Enable packet-filter rule");

    ret =
        filter_enable_rule(
            rule_id);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "enable rule",
            ret);

        test_fail();

        return 1;
    }

    memset(
        &rule,
        0,
        sizeof(rule));

    ret =
        filter_get_rule(
            rule_id,
            &rule);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "verify enabled rule",
            ret);

        test_fail();

        return 1;
    }

    if (!rule.enabled) {

        fprintf(
            stderr,
            "\n       Rule remains disabled\n");

        test_fail();

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 9 - Rule Disable
 * ============================================================
 */

static int test_disable_rule(
        uint32_t rule_id)
{
    filter_rule_t rule;

    int ret;

    test_start(
        "Disable packet-filter rule");

    ret =
        filter_disable_rule(
            rule_id);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "disable rule",
            ret);

        test_fail();

        return 1;
    }

    memset(
        &rule,
        0,
        sizeof(rule));

    ret =
        filter_get_rule(
            rule_id,
            &rule);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "verify disabled rule",
            ret);

        test_fail();

        return 1;
    }

    if (rule.enabled) {

        fprintf(
            stderr,
            "\n       Rule remains enabled\n");

        test_fail();

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 10 - Re-enable Rule
 * ============================================================
 */

static int test_reenable_rule(
        uint32_t rule_id)
{
    int ret;

    test_start(
        "Re-enable packet-filter rule");

    ret =
        filter_enable_rule(
            rule_id);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "re-enable rule",
            ret);

        test_fail();

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 11 - Packet Processing
 * ============================================================
 */

static int test_packet_processing(void)
{
    filter_packet_info_t packet;

    filter_decision_t decision;

    int ret;

    test_start(
        "Packet filtering decision");

    memset(
        &packet,
        0,
        sizeof(packet));

    memset(
        &decision,
        0,
        sizeof(decision));

    /*
     * Build a packet that should match the test rule.
     */

    packet.src_ip =
        filter_ipv4_to_network(
            TEST_SOURCE_IP);

    packet.dst_ip =
        filter_ipv4_to_network(
            TEST_DESTINATION_IP);

    packet.src_port =
        TEST_SOURCE_PORT;

    packet.dst_port =
        TEST_DESTINATION_PORT;

    packet.protocol =
        FILTER_PROTOCOL_TCP;

    packet.packet_length =
        1500;

    ret =
        filter_process_packet(
            &packet,
            &decision);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "packet processing",
            ret);

        test_fail();

        return 1;
    }

    printf(
        "\n       Decision: %s\n       ",
        filter_action_to_string(
            decision.action));

    /*
     * The rule was configured as ALLOW.
     */

    if (decision.action !=
        FILTER_ACTION_ALLOW) {

        fprintf(
            stderr,
            "\n       Expected ALLOW decision\n");

        test_fail();

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 12 - Statistics
 * ============================================================
 */

static int test_statistics(void)
{
    filter_statistics_t stats;

    int ret;

    test_start(
        "Statistics query");

    memset(
        &stats,
        0,
        sizeof(stats));

    ret =
        filter_get_statistics(
            &stats);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "statistics query",
            ret);

        test_fail();

        return 1;
    }

    printf(
        "\n"
        "       Received : %llu\n"
        "       Processed: %llu\n"
        "       Allowed  : %llu\n"
        "       Dropped  : %llu\n"
        "       ",

        (unsigned long long)
            stats.packets_received,

        (unsigned long long)
            stats.packets_processed,

        (unsigned long long)
            stats.packets_allowed,

        (unsigned long long)
            stats.packets_dropped);

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 13 - Monitoring
 * ============================================================
 */

static int test_monitoring(void)
{
    int ret;

    test_start(
        "Packet monitoring control");

    ret =
        filter_enable_monitoring();

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "enable monitoring",
            ret);

        test_fail();

        return 1;
    }

    if (!filter_monitoring_enabled()) {

        fprintf(
            stderr,
            "\n       Monitoring did not enable\n");

        test_fail();

        return 1;
    }

    ret =
        filter_disable_monitoring();

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "disable monitoring",
            ret);

        test_fail();

        return 1;
    }

    if (filter_monitoring_enabled()) {

        fprintf(
            stderr,
            "\n       Monitoring did not disable\n");

        test_fail();

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 14 - Default Action
 * ============================================================
 */

static int test_default_action(void)
{
    filter_action_t original;

    filter_action_t current;

    int ret;

    test_start(
        "Default action configuration");

    ret =
        filter_get_default_action(
            &original);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "get default action",
            ret);

        test_fail();

        return 1;
    }

    ret =
        filter_set_default_action(
            FILTER_ACTION_DROP);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "set default action",
            ret);

        test_fail();

        return 1;
    }

    ret =
        filter_get_default_action(
            &current);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "verify default action",
            ret);

        test_fail();

        return 1;
    }

    if (current != FILTER_ACTION_DROP) {

        fprintf(
            stderr,
            "\n       Default action was not set to DROP\n");

        test_fail();

        return 1;
    }

    /*
     * Restore original configuration.
     */

    filter_set_default_action(
        original);

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test 15 - Rule Deletion
 * ============================================================
 */

static int test_delete_rule(
        uint32_t rule_id)
{
    uint32_t count;

    int ret;

    test_start(
        "Delete packet-filter rule");

    ret =
        filter_remove_rule(
            rule_id);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "delete rule",
            ret);

        test_fail();

        return 1;
    }

    ret =
        filter_get_rule_count(
            &count);

    if (ret != FILTER_SUCCESS) {

        print_filter_error(
            "verify rule deletion",
            ret);

        test_fail();

        return 1;
    }

    if (count != 0) {

        fprintf(
            stderr,
            "\n       Expected 0 rules, got %u\n",
            count);

        test_fail();

        return 1;
    }

    test_pass();

    return 0;
}


/*
 * ============================================================
 * Test Summary
 * ============================================================
 */

static void print_summary(void)
{
    print_separator();

    printf(
        "Test Summary\n");

    print_separator();

    printf(
        "Tests run     : %u\n",
        tests_run);

    printf(
        "Tests passed  : %u\n",
        tests_passed);

    printf(
        "Tests failed  : %u\n",
        tests_failed);

    print_separator();

    if (tests_failed == 0) {

        printf(
            "\n"
            "RESULT: PASS\n"
            "\n"
            "All packet-filter functional tests passed.\n"
            "\n");

    } else {

        printf(
            "\n"
            "RESULT: FAIL\n"
            "\n"
            "%u test(s) failed.\n"
            "\n",
            tests_failed);
    }
}


/*
 * ============================================================
 * Quick Test
 * ============================================================
 */

static int run_quick_tests(void)
{
    filter_rule_t rule;

    /*
     * Basic driver.
     */

    test_initialization();

    test_version();

    test_interface();

    /*
     * Rule lifecycle.
     */

    test_clear_rules();

    if (test_add_rule(
            &rule) != 0) {

        return 1;
    }

    test_get_rule(
        &rule);

    test_rule_count();

    /*
     * Packet processing.
     */

    test_packet_processing();

    /*
     * Statistics.
     */

    test_statistics();

    /*
     * Cleanup.
     */

    test_delete_rule(
        rule.rule_id);

    return 0;
}


/*
 * ============================================================
 * Full Test
 * ============================================================
 */

static int run_full_tests(void)
{
    filter_rule_t rule;

    /*
     * Driver tests.
     */

    test_initialization();

    test_version();

    test_interface();

    /*
     * Rule database tests.
     */

    test_clear_rules();

    if (test_add_rule(
            &rule) != 0) {

        return 1;
    }

    test_get_rule(
        &rule);

    test_rule_count();

    /*
     * Rule state tests.
     */

    test_disable_rule(
        rule.rule_id);

    test_reenable_rule(
        rule.rule_id);

    /*
     * Packet path.
     */

    test_packet_processing();

    /*
     * Statistics.
     */

    test_statistics();

    /*
     * Monitoring.
     */

    test_monitoring();

    /*
     * Default action.
     */

    test_default_action();

    /*
     * Final cleanup.
     */

    test_delete_rule(
        rule.rule_id);

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
    const char *mode =
        "full";

    int ret;

    print_banner();

    if (argc >= 2) {

        mode =
            argv[1];
    }

    if (strcmp(
            mode,
            "help") == 0 ||
        strcmp(
            mode,
            "--help") == 0 ||
        strcmp(
            mode,
            "-h") == 0) {

        print_usage(
            argv[0]);

        return 0;
    }

    if (strcmp(
            mode,
            "quick") != 0 &&
        strcmp(
            mode,
            "full") != 0) {

        fprintf(
            stderr,
            "ERROR: unknown test mode '%s'\n",
            mode);

        print_usage(
            argv[0]);

        return 1;
    }

    /*
     * Initialize userspace library.
     */

    ret =
        filter_init();

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "\n"
            "ERROR: unable to initialize packet filter.\n"
            "\n"
            "Check that the kernel driver is loaded:\n"
            "\n"
            "    sudo ./scripts/load_driver.sh\n"
            "\n"
            "Check device node:\n"
            "\n"
            "    ls -l /dev/packet_filter\n"
            "\n");

        return 1;
    }

    printf(
        "Test mode: %s\n\n",
        mode);

    /*
     * Run selected tests.
     */

    if (strcmp(
            mode,
            "quick") == 0) {

        ret =
            run_quick_tests();

    } else {

        ret =
            run_full_tests();
    }

    /*
     * Cleanup.
     */

    filter_cleanup();

    /*
     * Summary.
     */

    print_summary();

    if (tests_failed != 0) {
        return 1;
    }

    if (ret != 0) {
        return 1;
    }

    return 0;
}
