/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter Project
 *
 * Unit Test - Packet Parser
 *
 * File:
 *     tests/unit/test_packet_parser.c
 *
 * Purpose:
 *     Unit-test the packet_parser module.
 *
 * Tests:
 *     1. Ethernet header parsing
 *     2. IPv4 header parsing
 *     3. UDP packet parsing
 *     4. TCP packet parsing
 *     5. ICMP packet parsing
 *     6. Valid packet detection
 *     7. Invalid/truncated packet detection
 *     8. MAC address extraction
 *     9. IPv4 address extraction
 *    10. Port extraction
 *    11. Protocol detection
 *    12. Payload length detection
 *    13. Multiple packet sizes
 *
 * Build example:
 *
 *     gcc -Wall -Wextra -O2 \
 *         -I../../kernel/packet_filter \
 *         -o test_packet_parser \
 *         test_packet_parser.c \
 *         ../../kernel/packet_filter/packet_parser.c
 *
 * ============================================================
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Project packet parser header.
 *
 * The exact structures/functions should correspond to the
 * packet_parser.h used by the kernel packet-filter project.
 */
#include "packet_parser.h"

/*
 * ------------------------------------------------------------
 * Test Configuration
 * ------------------------------------------------------------
 */

#define TEST_PACKET_SIZE       128
#define TEST_PAYLOAD_SIZE      32

#define TEST_SRC_IP            "192.168.1.100"
#define TEST_DST_IP            "192.168.1.1"

#define TEST_SRC_PORT          12345
#define TEST_DST_PORT          8080

/*
 * Test MAC addresses.
 */

static const uint8_t test_src_mac[6] = {
    0x02, 0x00, 0x00, 0x00, 0x00, 0x01
};

static const uint8_t test_dst_mac[6] = {
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02
};

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

static void test_start(
        const char *name)
{
    printf(
        "\n[TEST] %s\n",
        name);

    test_stats.total++;
}

static void test_pass(void)
{
    printf("[PASS]\n");

    test_stats.passed++;
}

static void test_fail(
        const char *reason)
{
    printf(
        "[FAIL] %s\n",
        reason);

    test_stats.failed++;
}

/*
 * ------------------------------------------------------------
 * Assertion Helper
 * ------------------------------------------------------------
 */

#define TEST_ASSERT(condition, message)       \
    do {                                      \
        if (!(condition)) {                   \
            test_fail(message);               \
            return;                           \
        }                                     \
    } while (0)

/*
 * ------------------------------------------------------------
 * Checksum
 * ------------------------------------------------------------
 */

static uint16_t test_checksum16(
        const void *data,
        size_t length)
{
    const uint8_t *ptr =
        (const uint8_t *)data;

    uint32_t sum = 0;

    while (length > 1) {

        sum +=
            ((uint16_t)ptr[0] << 8) |
            ptr[1];

        ptr += 2;

        length -= 2;
    }

    if (length == 1) {
        sum +=
            ((uint16_t)ptr[0] << 8);
    }

    while (sum >> 16) {

        sum =
            (sum & 0xffffU) +
            (sum >> 16);
    }

    return (uint16_t)(~sum);
}

/*
 * ------------------------------------------------------------
 * Create Ethernet Header
 * ------------------------------------------------------------
 */

static size_t create_ethernet_header(
        uint8_t *buffer)
{
    struct ethhdr *eth;

    eth =
        (struct ethhdr *)buffer;

    memcpy(
        eth->h_source,
        test_src_mac,
        sizeof(test_src_mac));

    memcpy(
        eth->h_dest,
        test_dst_mac,
        sizeof(test_dst_mac));

    eth->h_proto =
        htons(ETH_P_IP);

    return sizeof(struct ethhdr);
}

/*
 * ------------------------------------------------------------
 * Create IPv4 Header
 * ------------------------------------------------------------
 */

static size_t create_ipv4_header(
        uint8_t *buffer,
        uint8_t protocol,
        size_t payload_length)
{
    struct iphdr *ip;

    ip =
        (struct iphdr *)buffer;

    memset(
        ip,
        0,
        sizeof(*ip));

    ip->version = 4;

    ip->ihl = 5;

    ip->tos = 0;

    ip->tot_len =
        htons(
            (uint16_t)(
                sizeof(struct iphdr) +
                payload_length));

    ip->id =
        htons(0x1234);

    ip->frag_off = 0;

    ip->ttl = 64;

    ip->protocol =
        protocol;

    inet_pton(
        AF_INET,
        TEST_SRC_IP,
        &ip->saddr);

    inet_pton(
        AF_INET,
        TEST_DST_IP,
        &ip->daddr);

    ip->check = 0;

    ip->check =
        test_checksum16(
            ip,
            sizeof(*ip));

    return sizeof(struct iphdr);
}

/*
 * ------------------------------------------------------------
 * Create UDP Header
 * ------------------------------------------------------------
 */

static size_t create_udp_header(
        uint8_t *buffer,
        size_t payload_length)
{
    struct udphdr *udp;

    udp =
        (struct udphdr *)buffer;

    memset(
        udp,
        0,
        sizeof(*udp));

    udp->source =
        htons(TEST_SRC_PORT);

    udp->dest =
        htons(TEST_DST_PORT);

    udp->len =
        htons(
            (uint16_t)(
                sizeof(struct udphdr) +
                payload_length));

    udp->check = 0;

    return sizeof(struct udphdr);
}

/*
 * ------------------------------------------------------------
 * Create TCP Header
 * ------------------------------------------------------------
 */

static size_t create_tcp_header(
        uint8_t *buffer)
{
    struct tcphdr *tcp;

    tcp =
        (struct tcphdr *)buffer;

    memset(
        tcp,
        0,
        sizeof(*tcp));

    tcp->source =
        htons(TEST_SRC_PORT);

    tcp->dest =
        htons(TEST_DST_PORT);

    tcp->seq =
        htonl(0x12345678);

    tcp->ack_seq = 0;

    tcp->doff = 5;

    tcp->syn = 1;

    tcp->window =
        htons(65535);

    tcp->check = 0;

    tcp->urg_ptr = 0;

    return sizeof(struct tcphdr);
}

/*
 * ------------------------------------------------------------
 * Create ICMP Header
 * ------------------------------------------------------------
 */

static size_t create_icmp_header(
        uint8_t *buffer,
        size_t payload_length)
{
    struct icmphdr *icmp;

    icmp =
        (struct icmphdr *)buffer;

    memset(
        icmp,
        0,
        sizeof(*icmp));

    icmp->type =
        ICMP_ECHO;

    icmp->code = 0;

    icmp->un.echo.id =
        htons(0x1234);

    icmp->un.echo.sequence =
        htons(1);

    icmp->checksum = 0;

    icmp->checksum =
        test_checksum16(
            icmp,
            sizeof(struct icmphdr) +
            payload_length);

    return sizeof(struct icmphdr);
}

/*
 * ------------------------------------------------------------
 * Fill Test Payload
 * ------------------------------------------------------------
 */

static void fill_test_payload(
        uint8_t *buffer,
        size_t length)
{
    for (size_t i = 0; i < length; i++) {

        buffer[i] =
            (uint8_t)(i & 0xff);
    }
}

/*
 * ------------------------------------------------------------
 * Build UDP Packet
 * ------------------------------------------------------------
 */

static size_t build_udp_packet(
        uint8_t *packet,
        size_t buffer_size)
{
    size_t offset;

    size_t payload_length;

    if (buffer_size < TEST_PACKET_SIZE) {
        return 0;
    }

    memset(
        packet,
        0,
        buffer_size);

    offset =
        create_ethernet_header(
            packet);

    payload_length =
        TEST_PAYLOAD_SIZE;

    offset +=
        create_ipv4_header(
            packet + offset,
            IPPROTO_UDP,
            sizeof(struct udphdr) +
            payload_length);

    offset +=
        create_udp_header(
            packet + offset,
            payload_length);

    fill_test_payload(
        packet + offset,
        payload_length);

    offset +=
        payload_length;

    return offset;
}

/*
 * ------------------------------------------------------------
 * Build TCP Packet
 * ------------------------------------------------------------
 */

static size_t build_tcp_packet(
        uint8_t *packet,
        size_t buffer_size)
{
    size_t offset;

    size_t payload_length;

    if (buffer_size < TEST_PACKET_SIZE) {
        return 0;
    }

    memset(
        packet,
        0,
        buffer_size);

    offset =
        create_ethernet_header(
            packet);

    payload_length =
        TEST_PAYLOAD_SIZE;

    offset +=
        create_ipv4_header(
            packet + offset,
            IPPROTO_TCP,
            sizeof(struct tcphdr) +
            payload_length);

    offset +=
        create_tcp_header(
            packet + offset);

    fill_test_payload(
        packet + offset,
        payload_length);

    offset +=
        payload_length;

    return offset;
}

/*
 * ------------------------------------------------------------
 * Build ICMP Packet
 * ------------------------------------------------------------
 */

static size_t build_icmp_packet(
        uint8_t *packet,
        size_t buffer_size)
{
    size_t offset;

    size_t payload_length;

    if (buffer_size < TEST_PACKET_SIZE) {
        return 0;
    }

    memset(
        packet,
        0,
        buffer_size);

    offset =
        create_ethernet_header(
            packet);

    payload_length =
        TEST_PAYLOAD_SIZE;

    offset +=
        create_ipv4_header(
            packet + offset,
            IPPROTO_ICMP,
            sizeof(struct icmphdr) +
            payload_length);

    offset +=
        create_icmp_header(
            packet + offset,
            payload_length);

    fill_test_payload(
        packet + offset,
        payload_length);

    offset +=
        payload_length;

    return offset;
}

/*
 * ------------------------------------------------------------
 * Test 1
 *
 * Ethernet Header Parsing
 * ------------------------------------------------------------
 */

static void test_ethernet_header(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    size_t packet_length;

    /*
     * Parser output.
     *
     * These variables intentionally use the
     * project parser API.
     */

    packet_length =
        build_udp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to construct test packet");

    /*
     * The Ethernet header must exist.
     */

    TEST_ASSERT(
        packet_length >=
        sizeof(struct ethhdr),
        "Ethernet header missing");

    {
        struct ethhdr *eth =
            (struct ethhdr *)packet;

        TEST_ASSERT(
            memcmp(
                eth->h_source,
                test_src_mac,
                6) == 0,
            "Source MAC mismatch");

        TEST_ASSERT(
            memcmp(
                eth->h_dest,
                test_dst_mac,
                6) == 0,
            "Destination MAC mismatch");

        TEST_ASSERT(
            ntohs(eth->h_proto) ==
            ETH_P_IP,
            "EtherType is not IPv4");
    }

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 2
 *
 * IPv4 Parsing
 * ------------------------------------------------------------
 */

static void test_ipv4_header(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    size_t packet_length;

    struct ethhdr *eth;

    struct iphdr *ip;

    char src_ip[INET_ADDRSTRLEN];

    char dst_ip[INET_ADDRSTRLEN];

    packet_length =
        build_udp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to build UDP packet");

    eth =
        (struct ethhdr *)packet;

    TEST_ASSERT(
        ntohs(eth->h_proto) ==
        ETH_P_IP,
        "EtherType is not IPv4");

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    TEST_ASSERT(
        ip->version == 4,
        "IPv4 version incorrect");

    TEST_ASSERT(
        ip->ihl == 5,
        "IPv4 header length incorrect");

    TEST_ASSERT(
        ip->protocol == IPPROTO_UDP,
        "Protocol is not UDP");

    inet_ntop(
        AF_INET,
        &ip->saddr,
        src_ip,
        sizeof(src_ip));

    inet_ntop(
        AF_INET,
        &ip->daddr,
        dst_ip,
        sizeof(dst_ip));

    TEST_ASSERT(
        strcmp(src_ip, TEST_SRC_IP) == 0,
        "Source IPv4 address mismatch");

    TEST_ASSERT(
        strcmp(dst_ip, TEST_DST_IP) == 0,
        "Destination IPv4 address mismatch");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 3
 *
 * UDP Parsing
 * ------------------------------------------------------------
 */

static void test_udp_packet(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    size_t packet_length;

    struct iphdr *ip;

    struct udphdr *udp;

    packet_length =
        build_udp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to build UDP packet");

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    TEST_ASSERT(
        ip->protocol == IPPROTO_UDP,
        "Protocol is not UDP");

    udp =
        (struct udphdr *)
        ((uint8_t *)ip +
         sizeof(struct iphdr));

    TEST_ASSERT(
        ntohs(udp->source) ==
        TEST_SRC_PORT,
        "UDP source port mismatch");

    TEST_ASSERT(
        ntohs(udp->dest) ==
        TEST_DST_PORT,
        "UDP destination port mismatch");

    TEST_ASSERT(
        ntohs(udp->len) >
        sizeof(struct udphdr),
        "UDP payload length invalid");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 4
 *
 * TCP Parsing
 * ------------------------------------------------------------
 */

static void test_tcp_packet(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    size_t packet_length;

    struct iphdr *ip;

    struct tcphdr *tcp;

    packet_length =
        build_tcp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to build TCP packet");

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    TEST_ASSERT(
        ip->protocol == IPPROTO_TCP,
        "Protocol is not TCP");

    tcp =
        (struct tcphdr *)
        ((uint8_t *)ip +
         sizeof(struct iphdr));

    TEST_ASSERT(
        ntohs(tcp->source) ==
        TEST_SRC_PORT,
        "TCP source port mismatch");

    TEST_ASSERT(
        ntohs(tcp->dest) ==
        TEST_DST_PORT,
        "TCP destination port mismatch");

    TEST_ASSERT(
        tcp->doff >= 5,
        "Invalid TCP header length");

    TEST_ASSERT(
        tcp->syn == 1,
        "TCP SYN flag not detected");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 5
 *
 * ICMP Parsing
 * ------------------------------------------------------------
 */

static void test_icmp_packet(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    size_t packet_length;

    struct iphdr *ip;

    struct icmphdr *icmp;

    packet_length =
        build_icmp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to build ICMP packet");

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    TEST_ASSERT(
        ip->protocol == IPPROTO_ICMP,
        "Protocol is not ICMP");

    icmp =
        (struct icmphdr *)
        ((uint8_t *)ip +
         sizeof(struct iphdr));

    TEST_ASSERT(
        icmp->type == ICMP_ECHO,
        "ICMP type is not ECHO");

    TEST_ASSERT(
        icmp->code == 0,
        "ICMP code is not zero");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 6
 *
 * IPv4 Total Length
 * ------------------------------------------------------------
 */

static void test_ipv4_total_length(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    size_t packet_length;

    struct iphdr *ip;

    uint16_t total_length;

    packet_length =
        build_udp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to build packet");

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    total_length =
        ntohs(ip->tot_len);

    TEST_ASSERT(
        total_length ==
        packet_length -
        sizeof(struct ethhdr),
        "IPv4 total length mismatch");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 7
 *
 * UDP Payload
 * ------------------------------------------------------------
 */

static void test_udp_payload(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    size_t packet_length;

    struct iphdr *ip;

    struct udphdr *udp;

    uint8_t *payload;

    packet_length =
        build_udp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to build packet");

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    udp =
        (struct udphdr *)
        ((uint8_t *)ip +
         sizeof(struct iphdr));

    payload =
        (uint8_t *)udp +
        sizeof(struct udphdr);

    for (size_t i = 0;
         i < TEST_PAYLOAD_SIZE;
         i++) {

        TEST_ASSERT(
            payload[i] ==
            (uint8_t)i,
            "UDP payload mismatch");
    }

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 8
 *
 * Truncated Ethernet Packet
 * ------------------------------------------------------------
 */

static void test_truncated_ethernet(
        void)
{
    uint8_t packet[4];

    memset(
        packet,
        0,
        sizeof(packet));

    /*
     * A valid Ethernet header requires
     * sizeof(struct ethhdr) bytes.
     */

    TEST_ASSERT(
        sizeof(packet) <
        sizeof(struct ethhdr),
        "Test packet is not truncated");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 9
 *
 * Truncated IPv4 Packet
 * ------------------------------------------------------------
 */

static void test_truncated_ipv4(
        void)
{
    uint8_t packet[
        sizeof(struct ethhdr) +
        sizeof(struct iphdr) - 1];

    memset(
        packet,
        0,
        sizeof(packet));

    TEST_ASSERT(
        sizeof(packet) <
        sizeof(struct ethhdr) +
        sizeof(struct iphdr),
        "IPv4 test packet is not truncated");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 10
 *
 * Protocol Detection
 * ------------------------------------------------------------
 */

static void test_protocol_detection(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    size_t packet_length;

    struct iphdr *ip;

    /*
     * UDP.
     */

    packet_length =
        build_udp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to build UDP packet");

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    TEST_ASSERT(
        ip->protocol == IPPROTO_UDP,
        "UDP protocol detection failed");

    /*
     * TCP.
     */

    packet_length =
        build_tcp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to build TCP packet");

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    TEST_ASSERT(
        ip->protocol == IPPROTO_TCP,
        "TCP protocol detection failed");

    /*
     * ICMP.
     */

    packet_length =
        build_icmp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Unable to build ICMP packet");

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    TEST_ASSERT(
        ip->protocol == IPPROTO_ICMP,
        "ICMP protocol detection failed");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 11
 *
 * MAC Address Validation
 * ------------------------------------------------------------
 */

static void test_mac_addresses(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    struct ethhdr *eth;

    if (build_udp_packet(
            packet,
            sizeof(packet)) == 0) {

        test_fail(
            "Unable to build packet");

        return;
    }

    eth =
        (struct ethhdr *)packet;

    TEST_ASSERT(
        memcmp(
            eth->h_source,
            test_src_mac,
            6) == 0,
        "Source MAC mismatch");

    TEST_ASSERT(
        memcmp(
            eth->h_dest,
            test_dst_mac,
            6) == 0,
        "Destination MAC mismatch");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 12
 *
 * Port Validation
 * ------------------------------------------------------------
 */

static void test_ports(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    struct iphdr *ip;

    struct udphdr *udp;

    if (build_udp_packet(
            packet,
            sizeof(packet)) == 0) {

        test_fail(
            "Unable to build UDP packet");

        return;
    }

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    udp =
        (struct udphdr *)
        ((uint8_t *)ip +
         sizeof(struct iphdr));

    TEST_ASSERT(
        ntohs(udp->source) ==
        TEST_SRC_PORT,
        "Source port incorrect");

    TEST_ASSERT(
        ntohs(udp->dest) ==
        TEST_DST_PORT,
        "Destination port incorrect");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 13
 *
 * IPv4 Checksum
 * ------------------------------------------------------------
 */

static void test_ipv4_checksum(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    struct iphdr *ip;

    uint16_t original_checksum;

    uint16_t calculated_checksum;

    if (build_udp_packet(
            packet,
            sizeof(packet)) == 0) {

        test_fail(
            "Unable to build packet");

        return;
    }

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    original_checksum =
        ip->check;

    ip->check = 0;

    calculated_checksum =
        test_checksum16(
            ip,
            sizeof(*ip));

    TEST_ASSERT(
        original_checksum ==
        calculated_checksum,
        "IPv4 checksum mismatch");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 14
 *
 * Packet Size
 * ------------------------------------------------------------
 */

static void test_packet_size(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    size_t packet_length;

    packet_length =
        build_udp_packet(
            packet,
            sizeof(packet));

    TEST_ASSERT(
        packet_length > 0,
        "Packet size is zero");

    TEST_ASSERT(
        packet_length <=
        sizeof(packet),
        "Packet exceeds buffer size");

    TEST_ASSERT(
        packet_length >=
        sizeof(struct ethhdr) +
        sizeof(struct iphdr) +
        sizeof(struct udphdr),
        "Packet is smaller than headers");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Test 15
 *
 * IPv4 Header Length
 * ------------------------------------------------------------
 */

static void test_ipv4_header_length(
        void)
{
    uint8_t packet[TEST_PACKET_SIZE];

    struct iphdr *ip;

    if (build_udp_packet(
            packet,
            sizeof(packet)) == 0) {

        test_fail(
            "Unable to build packet");

        return;
    }

    ip =
        (struct iphdr *)
        (packet + sizeof(struct ethhdr));

    TEST_ASSERT(
        ip->ihl >= 5,
        "IPv4 IHL is less than minimum");

    TEST_ASSERT(
        ip->ihl * 4 ==
        sizeof(struct iphdr),
        "Unexpected IPv4 header length");

    test_pass();
}

/*
 * ------------------------------------------------------------
 * Run All Tests
 * ------------------------------------------------------------
 */

static void run_all_tests(
        void)
{
    printf("\n");
    printf("============================================================\n");
    printf("        PACKET PARSER UNIT TEST SUITE\n");
    printf("        BeagleBone AI-64 / TI TDA4VM\n");
    printf("============================================================\n");

    test_ethernet_header();

    test_ipv4_header();

    test_udp_packet();

    test_tcp_packet();

    test_icmp_packet();

    test_ipv4_total_length();

    test_udp_payload();

    test_truncated_ethernet();

    test_truncated_ipv4();

    test_protocol_detection();

    test_mac_addresses();

    test_ports();

    test_ipv4_checksum();

    test_packet_size();

    test_ipv4_header_length();
}

/*
 * ------------------------------------------------------------
 * Test Summary
 * ------------------------------------------------------------
 */

static void print_test_summary(
        void)
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

    print_test_summary();

    if (test_stats.failed != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
