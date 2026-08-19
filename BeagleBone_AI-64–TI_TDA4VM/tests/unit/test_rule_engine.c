/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter Project
 *
 * Unit Test - Rule Engine
 *
 * File:
 *     tests/unit/test_rule_engine.c
 *
 * Purpose:
 *     Unit-test the packet-filter rule engine.
 *
 * Coverage:
 *     - Rule initialization
 *     - Whitelist rules
 *     - Blacklist rules
 *     - Monitoring rules
 *     - Source IP matching
 *     - Destination IP matching
 *     - Source port matching
 *     - Destination port matching
 *     - Protocol matching
 *     - Rule priority
 *     - Default action
 *     - Multiple rules
 *     - Rule statistics
 *     - Invalid input handling
 *
 * Build example:
 *
 *     gcc -Wall -Wextra -O2 \
 *         -I../../kernel/packet_filter \
 *         -o test_rule_engine \
 *         test_rule_engine.c \
 *         ../../kernel/packet_filter/rule_engine.c
 *
 * ============================================================
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rule_engine.h"

/*
 * ------------------------------------------------------------
 * Test Configuration
 * ------------------------------------------------------------
 */

#define TEST_SRC_IP        "192.168.1.100"
#define TEST_DST_IP        "192.168.1.1"

#define BLOCKED_SRC_IP     "10.10.10.10"
#define BLOCKED_DST_IP     "172.16.0.100"

#define TEST_SRC_PORT      12345
#define TEST_DST_PORT      8080

#define BLOCKED_SRC_PORT   4444
#define BLOCKED_DST_PORT   23

/*
 * ------------------------------------------------------------
 * Test Statistics
 * ------------------------------------------------------------
 */

struct test_statistics {
    unsigned int total;
    unsigned int passed;
    unsigned int failed;
};

static struct test_statistics test_stats;

/*
 * ------------------------------------------------------------
 * Test Helpers
 * ------------------------------------------------------------
 */

static void test_start(const char *name)
{
    printf("\n[TEST] %s\n", name);

    test_stats.total++;
}

static void test_pass(void)
{
    printf("[PASS]\n");

    test_stats.passed++;
}

static void test_fail(const char *reason)
{
    printf("[FAIL] %s\n", reason);

    test_stats.failed++;
}

#define TEST_ASSERT(condition, message)        \
    do {                                       \
        if (!(condition)) {                   \
            test_fail(message);               \
            return;                            \
        }                                      \
    } while (0)

/*
 * ------------------------------------------------------------
 * Test Packet Description
 * ------------------------------------------------------------
 *
 * This structure represents the information normally extracted
 * by packet_parser.c and supplied to rule_engine.c.
 *
 * If rule_engine.h already defines a packet context structure,
 * this helper can be mapped to that structure directly.
 * ------------------------------------------------------------
 */

struct test_packet {
    uint32_t src_ip;
    uint32_t dst_ip;

    uint16_t src_port;
    uint16_t dst_port;

    uint8_t protocol;
};

/*
 * ------------------------------------------------------------
 * IP Address Helper
 * ------------------------------------------------------------
 */

static uint32_t test_ip(const char *address)
{
    struct in_addr addr;

    if (inet_pton(
            AF_INET,
            address,
            &addr) != 1) {

        return 0;
    }

    return addr.s_addr;
}

/*
 * ------------------------------------------------------------
 * Create Test Packet
 * ------------------------------------------------------------
 */

static void create_packet(
        struct test_packet *packet,
        const char *src_ip,
        const char *dst_ip,
        uint16_t src_port,
        uint16_t dst_port,
        uint8_t protocol)
{
    memset(
        packet,
        0,
        sizeof(*packet));

    packet->src_ip =
        test_ip(src_ip);

    packet->dst_ip =
        test_ip(dst_ip);

    packet->src_port =
        src_port;

    packet->dst_port =
        dst_port;

    packet->protocol =
        protocol;
}

/*
 * ------------------------------------------------------------
 * Print Packet
 * ------------------------------------------------------------
 */

static void print_packet(
        const struct test_packet *packet)
{
    struct in_addr src;
    struct in_addr dst;

    char src_string[INET_ADDRSTRLEN];
    char dst_string[INET_ADDRSTRLEN];

    src.s_addr =
        packet->src_ip;

    dst.s_addr =
        packet->dst_ip;

    inet_ntop(
        AF_INET,
        &src,
        src_string,
        sizeof(src_string));

    inet_ntop(
        AF_INET,
        &dst,
        dst_string,
        sizeof(dst_string));

    printf(
        "Packet: %s:%u -> %s:%u protocol=%u\n",
        src_string,
        packet->src_port,
        dst_string,
        packet->dst_port,
        packet->protocol);
}

/*
 * ------------------------------------------------------------
 * Rule Action Conversion
 * ------------------------------------------------------------
 *
 * These helpers keep the tests readable.
 *
 * The rule_engine implementation may use different enum names.
 * Update these mappings if rule_engine.h uses different names.
 * ------------------------------------------------------------
 */

#ifndef RULE_ACTION_ALLOW

#define RULE_ACTION_ALLOW       0

#endif

#ifndef RULE_ACTION_DROP

#define RULE_ACTION_DROP        1

#endif

#ifndef RULE_ACTION_MONITOR

#define RULE_ACTION_MONITOR     2

#endif

/*
 * ------------------------------------------------------------
 * Test 1
 *
 * Rule Engine Initialization
 * ------------------------------------------------------------
 */

static void test_engine_initialization(void)
{
    int ret;

    /*
     * Expected project API:
     *
     *     rule_engine_init();
     *
     * If the implementation returns void, change this test
     * accordingly.
     */

    ret =
        rule_engine_init();

    TEST_ASSERT(
        ret == 0,
        "rule_engine_init() failed");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 2
 *
 * Empty Rule Table
 * ------------------------------------------------------------
 */

static void test_empty_rule_table(void)
{
    struct test_packet packet;

    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    print_packet(&packet);

    /*
     * With no matching rules, the default policy should be
     * returned.
     *
     * Depending on project policy this can be ALLOW or DROP.
     */

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_ALLOW ||
        action == RULE_ACTION_DROP,
        "Invalid default action");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 3
 *
 * Whitelist Source IP
 * ------------------------------------------------------------
 */

static void test_whitelist_source_ip(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    /*
     * Add an allow rule for the source IP.
     */

    ret =
        rule_engine_add_rule(
            packet.src_ip,
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_ALLOW);

    TEST_ASSERT(
        ret == 0,
        "Unable to add whitelist rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_ALLOW,
        "Whitelist source IP was not allowed");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 4
 *
 * Blacklist Source IP
 * ------------------------------------------------------------
 */

static void test_blacklist_source_ip(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        BLOCKED_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    ret =
        rule_engine_add_rule(
            packet.src_ip,
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add blacklist rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "Blacklisted source IP was not dropped");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 5
 *
 * Destination IP Matching
 * ------------------------------------------------------------
 */

static void test_destination_ip(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        BLOCKED_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    ret =
        rule_engine_add_rule(
            0,
            packet.dst_ip,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add destination IP rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "Destination IP rule did not match");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 6
 *
 * Source Port Matching
 * ------------------------------------------------------------
 */

static void test_source_port(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        BLOCKED_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    ret =
        rule_engine_add_rule(
            0,
            0,
            packet.src_port,
            0,
            IPPROTO_TCP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add source port rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "Source port rule did not match");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 7
 *
 * Destination Port Matching
 * ------------------------------------------------------------
 */

static void test_destination_port(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        BLOCKED_DST_PORT,
        IPPROTO_TCP);

    ret =
        rule_engine_add_rule(
            0,
            0,
            0,
            packet.dst_port,
            IPPROTO_TCP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add destination port rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "Destination port rule did not match");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 8
 *
 * TCP Protocol Matching
 * ------------------------------------------------------------
 */

static void test_tcp_protocol(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    ret =
        rule_engine_add_rule(
            0,
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add TCP rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "TCP rule did not match");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 9
 *
 * UDP Protocol Matching
 * ------------------------------------------------------------
 */

static void test_udp_protocol(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_UDP);

    ret =
        rule_engine_add_rule(
            0,
            0,
            0,
            0,
            IPPROTO_UDP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add UDP rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "UDP rule did not match");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 10
 *
 * ICMP Protocol Matching
 * ------------------------------------------------------------
 */

static void test_icmp_protocol(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        0,
        0,
        IPPROTO_ICMP);

    ret =
        rule_engine_add_rule(
            0,
            0,
            0,
            0,
            IPPROTO_ICMP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add ICMP rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "ICMP rule did not match");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 11
 *
 * Combined Rule Matching
 * ------------------------------------------------------------
 */

static void test_combined_rule(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        BLOCKED_SRC_IP,
        TEST_DST_IP,
        BLOCKED_SRC_PORT,
        BLOCKED_DST_PORT,
        IPPROTO_TCP);

    /*
     * Rule must match all fields:
     *
     * source IP
     * destination IP
     * source port
     * destination port
     * protocol
     */

    ret =
        rule_engine_add_rule(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add combined rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "Combined rule did not match");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 12
 *
 * Non-Matching Rule
 * ------------------------------------------------------------
 */

static void test_non_matching_rule(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    /*
     * Rule for a different source IP.
     */

    ret =
        rule_engine_add_rule(
            test_ip("10.20.30.40"),
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add non-matching rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action != RULE_ACTION_DROP,
        "Non-matching rule incorrectly dropped packet");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 13
 *
 * Rule Priority
 * ------------------------------------------------------------
 */

static void test_rule_priority(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        BLOCKED_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    /*
     * General ALLOW rule.
     */

    ret =
        rule_engine_add_rule(
            0,
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_ALLOW);

    TEST_ASSERT(
        ret == 0,
        "Unable to add allow rule");

    /*
     * Specific DROP rule.
     */

    ret =
        rule_engine_add_rule(
            packet.src_ip,
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add specific drop rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    /*
     * Specific rule should take precedence.
     */

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "Rule priority is incorrect");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 14
 *
 * Monitoring Rule
 * ------------------------------------------------------------
 */

static void test_monitor_rule(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    ret =
        rule_engine_add_rule(
            packet.src_ip,
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_MONITOR);

    TEST_ASSERT(
        ret == 0,
        "Unable to add monitoring rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    /*
     * Monitoring should not behave as an invalid action.
     */

    TEST_ASSERT(
        action == RULE_ACTION_MONITOR ||
        action == RULE_ACTION_ALLOW,
        "Monitoring rule returned invalid action");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 15
 *
 * Rule Removal
 * ------------------------------------------------------------
 */

static void test_rule_removal(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        BLOCKED_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    ret =
        rule_engine_add_rule(
            packet.src_ip,
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "Rule was not active");

    /*
     * Remove the rule.
     *
     * The exact API can be adapted to the implementation.
     */

    ret =
        rule_engine_remove_rule(
            packet.src_ip,
            0,
            0,
            0,
            IPPROTO_TCP);

    TEST_ASSERT(
        ret == 0,
        "Unable to remove rule");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 16
 *
 * Clear All Rules
 * ------------------------------------------------------------
 */

static void test_clear_rules(void)
{
    int ret;

    ret =
        rule_engine_clear_rules();

    TEST_ASSERT(
        ret == 0,
        "Unable to clear rule table");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 17
 *
 * Invalid IP Address
 * ------------------------------------------------------------
 */

static void test_invalid_ip(void)
{
    uint32_t address;

    address =
        test_ip("999.999.999.999");

    TEST_ASSERT(
        address == 0,
        "Invalid IPv4 address was accepted");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 18
 *
 * Wildcard Rule
 * ------------------------------------------------------------
 */

static void test_wildcard_rule(void)
{
    struct test_packet packet;

    int ret;
    int action;

    create_packet(
        &packet,
        TEST_SRC_IP,
        TEST_DST_IP,
        TEST_SRC_PORT,
        TEST_DST_PORT,
        IPPROTO_TCP);

    /*
     * All zero fields represent wildcard fields.
     */

    ret =
        rule_engine_add_rule(
            0,
            0,
            0,
            0,
            0,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add wildcard rule");

    action =
        rule_engine_process(
            packet.src_ip,
            packet.dst_ip,
            packet.src_port,
            packet.dst_port,
            packet.protocol);

    TEST_ASSERT(
        action == RULE_ACTION_DROP,
        "Wildcard rule did not match");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 19
 *
 * Multiple Rules
 * ------------------------------------------------------------
 */

static void test_multiple_rules(void)
{
    struct test_packet packet1;
    struct test_packet packet2;

    int ret;
    int action1;
    int action2;

    create_packet(
        &packet1,
        "192.168.1.10",
        TEST_DST_IP,
        1000,
        80,
        IPPROTO_TCP);

    create_packet(
        &packet2,
        "192.168.1.20",
        TEST_DST_IP,
        2000,
        443,
        IPPROTO_TCP);

    ret =
        rule_engine_add_rule(
            packet1.src_ip,
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_DROP);

    TEST_ASSERT(
        ret == 0,
        "Unable to add first rule");

    ret =
        rule_engine_add_rule(
            packet2.src_ip,
            0,
            0,
            0,
            IPPROTO_TCP,
            RULE_ACTION_ALLOW);

    TEST_ASSERT(
        ret == 0,
        "Unable to add second rule");

    action1 =
        rule_engine_process(
            packet1.src_ip,
            packet1.dst_ip,
            packet1.src_port,
            packet1.dst_port,
            packet1.protocol);

    action2 =
        rule_engine_process(
            packet2.src_ip,
            packet2.dst_ip,
            packet2.src_port,
            packet2.dst_port,
            packet2.protocol);

    TEST_ASSERT(
        action1 == RULE_ACTION_DROP,
        "First rule did not match");

    TEST_ASSERT(
        action2 == RULE_ACTION_ALLOW,
        "Second rule did not match");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Run Test Suite
 * ------------------------------------------------------------
 */

static void run_all_tests(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("             RULE ENGINE UNIT TEST SUITE\n");
    printf("             BeagleBone AI-64 / TI TDA4VM\n");
    printf("============================================================\n");

    test_start(
        "Rule engine initialization");
    test_engine_initialization();

    test_start(
        "Empty rule table");
    test_empty_rule_table();

    test_start(
        "Whitelist source IP");
    test_whitelist_source_ip();

    test_start(
        "Blacklist source IP");
    test_blacklist_source_ip();

    test_start(
        "Destination IP matching");
    test_destination_ip();

    test_start(
        "Source port matching");
    test_source_port();

    test_start(
        "Destination port matching");
    test_destination_port();

    test_start(
        "TCP protocol matching");
    test_tcp_protocol();

    test_start(
        "UDP protocol matching");
    test_udp_protocol();

    test_start(
        "ICMP protocol matching");
    test_icmp_protocol();

    test_start(
        "Combined rule matching");
    test_combined_rule();

    test_start(
        "Non-matching rule");
    test_non_matching_rule();

    test_start(
        "Rule priority");
    test_rule_priority();

    test_start(
        "Monitoring rule");
    test_monitor_rule();

    test_start(
        "Rule removal");
    test_rule_removal();

    test_start(
        "Clear all rules");
    test_clear_rules();

    test_start(
        "Invalid IP address");
    test_invalid_ip();

    test_start(
        "Wildcard rule");
    test_wildcard_rule();

    test_start(
        "Multiple rules");
    test_multiple_rules();
}

/*
 * ------------------------------------------------------------
 * Print Summary
 * ------------------------------------------------------------
 */

static void print_summary(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("                    TEST SUMMARY\n");
    printf("============================================================\n");

    printf(
        "Total  : %u\n",
        test_stats.total);

    printf(
        "Passed : %u\n",
        test_stats.passed);

    printf(
        "Failed : %u\n",
        test_stats.failed);

    if (test_stats.failed == 0) {

        printf(
            "\nRESULT : PASS\n");

    } else {

        printf(
            "\nRESULT : FAIL\n");
    }

    printf(
        "============================================================\n");
}

/*
 * ------------------------------------------------------------
 * Main
 * ------------------------------------------------------------
 */

int main(void)
{
    run_all_tests();

    print_summary();

    if (test_stats.failed != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
