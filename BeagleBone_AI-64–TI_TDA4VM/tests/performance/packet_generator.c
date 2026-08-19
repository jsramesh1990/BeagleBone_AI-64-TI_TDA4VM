/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter Project
 *
 * Packet Generator
 *
 * File:
 *     tests/performance/packet_generator.c
 *
 * Purpose:
 *     Generate Ethernet/IP/TCP/UDP packets for performance,
 *     filtering, stress and integration testing.
 *
 * Supported:
 *     - Raw Ethernet frames
 *     - IPv4
 *     - TCP
 *     - UDP
 *     - ICMP
 *     - Configurable source/destination MAC
 *     - Configurable source/destination IPv4
 *     - Configurable source/destination ports
 *     - Configurable packet size
 *     - Configurable packet count
 *     - Configurable packet rate
 *     - Continuous mode
 *     - Dry-run mode
 *     - Packet dump
 *     - Statistics
 *
 * WARNING:
 *     Raw packet transmission requires root/CAP_NET_RAW
 *     privileges and can affect the connected network.
 *
 * Example:
 *
 *     sudo ./packet_generator \
 *         --interface eth0 \
 *         --count 1000
 *
 *     sudo ./packet_generator \
 *         --interface eth0 \
 *         --protocol udp \
 *         --size 512 \
 *         --count 10000
 *
 *     sudo ./packet_generator \
 *         --interface eth0 \
 *         --rate 10000 \
 *         --continuous
 *
 * ============================================================
 */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/*
 * ------------------------------------------------------------
 * Configuration
 * ------------------------------------------------------------
 */

#define DEFAULT_INTERFACE      "eth0"
#define DEFAULT_PACKET_SIZE    128
#define DEFAULT_PACKET_COUNT   1000
#define DEFAULT_RATE           0

#define MIN_PACKET_SIZE        64
#define MAX_PACKET_SIZE        9000

#define MAX_PACKET_BUFFER      9216

#define DEFAULT_SRC_PORT       12345
#define DEFAULT_DST_PORT       8080

#define ETH_ALEN               6

/*
 * ------------------------------------------------------------
 * Protocol
 * ------------------------------------------------------------
 */

enum packet_protocol {
    PROTOCOL_UDP = 0,
    PROTOCOL_TCP,
    PROTOCOL_ICMP
};

/*
 * ------------------------------------------------------------
 * Generator Configuration
 * ------------------------------------------------------------
 */

struct generator_config {
    char interface[IFNAMSIZ];

    uint8_t src_mac[ETH_ALEN];
    uint8_t dst_mac[ETH_ALEN];

    struct in_addr src_ip;
    struct in_addr dst_ip;

    uint16_t src_port;
    uint16_t dst_port;

    enum packet_protocol protocol;

    size_t packet_size;

    uint64_t packet_count;

    uint64_t rate;

    int continuous;

    int verbose;

    int dump_packet;

    int dry_run;

    int randomize;

    uint8_t ttl;
};

/*
 * ------------------------------------------------------------
 * Generator Statistics
 * ------------------------------------------------------------
 */

struct generator_stats {
    uint64_t packets_generated;

    uint64_t packets_sent;

    uint64_t packets_failed;

    uint64_t bytes_sent;

    uint64_t start_ns;

    uint64_t end_ns;
};

/*
 * ------------------------------------------------------------
 * Global Stop Flag
 * ------------------------------------------------------------
 */

static volatile sig_atomic_t stop_generator = 0;

/*
 * ------------------------------------------------------------
 * Signal Handler
 * ------------------------------------------------------------
 */

static void signal_handler(int signal_number)
{
    (void)signal_number;

    stop_generator = 1;
}

/*
 * ------------------------------------------------------------
 * Time
 * ------------------------------------------------------------
 */

static uint64_t get_time_ns(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return ((uint64_t)ts.tv_sec * 1000000000ULL) +
           (uint64_t)ts.tv_nsec;
}

/*
 * ------------------------------------------------------------
 * Checksum
 * ------------------------------------------------------------
 */

static uint16_t checksum16(
        const void *data,
        size_t length)
{
    const uint8_t *bytes = data;

    uint32_t sum = 0;

    while (length > 1) {
        sum += ((uint16_t)bytes[0] << 8) |
               bytes[1];

        bytes += 2;
        length -= 2;
    }

    if (length == 1) {
        sum += ((uint16_t)bytes[0] << 8);
    }

    while (sum >> 16) {
        sum = (sum & 0xffffU) +
              (sum >> 16);
    }

    return (uint16_t)(~sum);
}

/*
 * ------------------------------------------------------------
 * IPv4 Checksum
 * ------------------------------------------------------------
 */

static uint16_t ip_checksum(
        struct iphdr *ip)
{
    ip->check = 0;

    return checksum16(
        ip,
        sizeof(struct iphdr));
}

/*
 * ------------------------------------------------------------
 * TCP/UDP Pseudo Header
 * ------------------------------------------------------------
 */

struct pseudo_header {
    uint32_t src_addr;

    uint32_t dst_addr;

    uint8_t reserved;

    uint8_t protocol;

    uint16_t length;
};

/*
 * ------------------------------------------------------------
 * Transport Checksum
 * ------------------------------------------------------------
 */

static uint16_t transport_checksum(
        struct iphdr *ip,
        const void *transport,
        size_t transport_length)
{
    struct pseudo_header pseudo;

    uint8_t *buffer;

    size_t total_length;

    uint16_t result;

    memset(&pseudo, 0, sizeof(pseudo));

    pseudo.src_addr = ip->saddr;
    pseudo.dst_addr = ip->daddr;

    pseudo.protocol = ip->protocol;

    pseudo.length =
        htons((uint16_t)transport_length);

    total_length =
        sizeof(pseudo) + transport_length;

    /*
     * Add one byte if transport payload is odd.
     */

    buffer = calloc(
        1,
        total_length + 1);

    if (buffer == NULL) {
        return 0;
    }

    memcpy(
        buffer,
        &pseudo,
        sizeof(pseudo));

    memcpy(
        buffer + sizeof(pseudo),
        transport,
        transport_length);

    result = checksum16(
        buffer,
        total_length);

    free(buffer);

    return result;
}

/*
 * ------------------------------------------------------------
 * MAC Parser
 * ------------------------------------------------------------
 */

static int parse_mac(
        const char *text,
        uint8_t mac[ETH_ALEN])
{
    unsigned int values[ETH_ALEN];

    int result;

    result = sscanf(
        text,
        "%x:%x:%x:%x:%x:%x",
        &values[0],
        &values[1],
        &values[2],
        &values[3],
        &values[4],
        &values[5]);

    if (result != ETH_ALEN) {
        return -1;
    }

    for (int i = 0; i < ETH_ALEN; i++) {

        if (values[i] > 0xffU) {
            return -1;
        }

        mac[i] = (uint8_t)values[i];
    }

    return 0;
}

/*
 * ------------------------------------------------------------
 * MAC Formatter
 * ------------------------------------------------------------
 */

static void mac_to_string(
        const uint8_t mac[ETH_ALEN],
        char *buffer,
        size_t length)
{
    snprintf(
        buffer,
        length,
        "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]);
}

/*
 * ------------------------------------------------------------
 * Random MAC
 * ------------------------------------------------------------
 */

static void random_mac(
        uint8_t mac[ETH_ALEN])
{
    for (int i = 0; i < ETH_ALEN; i++) {
        mac[i] = (uint8_t)(rand() & 0xff);
    }

    /*
     * Locally administered unicast MAC.
     */

    mac[0] &= 0xfe;
    mac[0] |= 0x02;
}

/*
 * ------------------------------------------------------------
 * Random IPv4
 * ------------------------------------------------------------
 */

static uint32_t random_ipv4(void)
{
    uint32_t value;

    value = ((uint32_t)(rand() & 0xff) << 24) |
            ((uint32_t)(rand() & 0xff) << 16) |
            ((uint32_t)(rand() & 0xff) << 8) |
            ((uint32_t)(rand() & 0xff));

    return htonl(value);
}

/*
 * ------------------------------------------------------------
 * Protocol Name
 * ------------------------------------------------------------
 */

static const char *protocol_name(
        enum packet_protocol protocol)
{
    switch (protocol) {

    case PROTOCOL_UDP:
        return "UDP";

    case PROTOCOL_TCP:
        return "TCP";

    case PROTOCOL_ICMP:
        return "ICMP";

    default:
        return "UNKNOWN";
    }
}

/*
 * ------------------------------------------------------------
 * Parse Protocol
 * ------------------------------------------------------------
 */

static int parse_protocol(
        const char *value,
        enum packet_protocol *protocol)
{
    if (strcasecmp(value, "udp") == 0) {
        *protocol = PROTOCOL_UDP;
        return 0;
    }

    if (strcasecmp(value, "tcp") == 0) {
        *protocol = PROTOCOL_TCP;
        return 0;
    }

    if (strcasecmp(value, "icmp") == 0) {
        *protocol = PROTOCOL_ICMP;
        return 0;
    }

    return -1;
}

/*
 * ------------------------------------------------------------
 * Fill Payload
 * ------------------------------------------------------------
 */

static void fill_payload(
        uint8_t *payload,
        size_t length,
        uint64_t packet_number)
{
    for (size_t i = 0; i < length; i++) {

        payload[i] =
            (uint8_t)((i + packet_number) & 0xff);
    }
}

/*
 * ------------------------------------------------------------
 * Build Ethernet Header
 * ------------------------------------------------------------
 */

static size_t build_ethernet_header(
        uint8_t *buffer,
        const struct generator_config *config)
{
    struct ethhdr *eth;

    eth = (struct ethhdr *)buffer;

    memcpy(
        eth->h_source,
        config->src_mac,
        ETH_ALEN);

    memcpy(
        eth->h_dest,
        config->dst_mac,
        ETH_ALEN);

    eth->h_proto = htons(ETH_P_IP);

    return sizeof(struct ethhdr);
}

/*
 * ------------------------------------------------------------
 * Build IPv4 Header
 * ------------------------------------------------------------
 */

static size_t build_ip_header(
        uint8_t *buffer,
        size_t payload_length,
        const struct generator_config *config)
{
    struct iphdr *ip;

    ip = (struct iphdr *)buffer;

    memset(ip, 0, sizeof(*ip));

    ip->version = 4;

    ip->ihl = 5;

    ip->tos = 0;

    ip->tot_len =
        htons((uint16_t)(
            sizeof(struct iphdr) +
            payload_length));

    ip->id =
        htons((uint16_t)(rand() & 0xffff));

    ip->frag_off = 0;

    ip->ttl = config->ttl;

    switch (config->protocol) {

    case PROTOCOL_UDP:
        ip->protocol = IPPROTO_UDP;
        break;

    case PROTOCOL_TCP:
        ip->protocol = IPPROTO_TCP;
        break;

    case PROTOCOL_ICMP:
        ip->protocol = IPPROTO_ICMP;
        break;
    }

    ip->saddr = config->src_ip.s_addr;

    ip->daddr = config->dst_ip.s_addr;

    ip->check = ip_checksum(ip);

    return sizeof(struct iphdr);
}

/*
 * ------------------------------------------------------------
 * Build UDP Header
 * ------------------------------------------------------------
 */

static size_t build_udp_packet(
        uint8_t *buffer,
        size_t payload_length,
        const struct generator_config *config,
        struct iphdr *ip)
{
    struct udphdr *udp;

    udp = (struct udphdr *)buffer;

    memset(
        udp,
        0,
        sizeof(*udp));

    udp->source =
        htons(config->src_port);

    udp->dest =
        htons(config->dst_port);

    udp->len =
        htons((uint16_t)(
            sizeof(struct udphdr) +
            payload_length));

    udp->check = 0;

    udp->check =
        transport_checksum(
            ip,
            udp,
            sizeof(struct udphdr) +
            payload_length);

    return sizeof(struct udphdr);
}

/*
 * ------------------------------------------------------------
 * Build TCP Header
 * ------------------------------------------------------------
 */

static size_t build_tcp_packet(
        uint8_t *buffer,
        size_t payload_length,
        const struct generator_config *config,
        struct iphdr *ip)
{
    struct tcphdr *tcp;

    tcp = (struct tcphdr *)buffer;

    memset(
        tcp,
        0,
        sizeof(*tcp));

    tcp->source =
        htons(config->src_port);

    tcp->dest =
        htons(config->dst_port);

    tcp->seq =
        htonl((uint32_t)rand());

    tcp->ack_seq = 0;

    tcp->doff = 5;

    tcp->syn = 1;

    tcp->window =
        htons(65535);

    tcp->check = 0;

    tcp->check =
        transport_checksum(
            ip,
            tcp,
            sizeof(struct tcphdr) +
            payload_length);

    return sizeof(struct tcphdr);
}

/*
 * ------------------------------------------------------------
 * Build ICMP Header
 * ------------------------------------------------------------
 */

static size_t build_icmp_packet(
        uint8_t *buffer,
        size_t payload_length)
{
    struct icmphdr *icmp;

    icmp = (struct icmphdr *)buffer;

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
        htons((uint16_t)(rand() & 0xffff));

    icmp->checksum = 0;

    icmp->checksum =
        checksum16(
            icmp,
            sizeof(struct icmphdr) +
            payload_length);

    return sizeof(struct icmphdr);
}

/*
 * ------------------------------------------------------------
 * Build Complete Packet
 * ------------------------------------------------------------
 */

static int build_packet(
        uint8_t *buffer,
        size_t buffer_size,
        const struct generator_config *config,
        uint64_t packet_number)
{
    size_t offset;

    size_t ip_payload_size;

    size_t transport_size;

    struct iphdr *ip;

    if (buffer == NULL ||
        config == NULL) {
        return -1;
    }

    if (buffer_size < config->packet_size) {
        return -1;
    }

    memset(
        buffer,
        0,
        config->packet_size);

    /*
     * Ethernet.
     */

    offset =
        build_ethernet_header(
            buffer,
            config);

    /*
     * IP.
     */

    if (config->packet_size <
        offset +
        sizeof(struct iphdr)) {
        return -1;
    }

    ip =
        (struct iphdr *)
        (buffer + offset);

    ip_payload_size =
        config->packet_size -
        offset -
        sizeof(struct iphdr);

    build_ip_header(
        buffer + offset,
        ip_payload_size,
        config);

    offset += sizeof(struct iphdr);

    /*
     * Transport layer.
     */

    switch (config->protocol) {

    case PROTOCOL_UDP:

        if (config->packet_size <
            offset +
            sizeof(struct udphdr)) {
            return -1;
        }

        transport_size =
            config->packet_size -
            offset -
            sizeof(struct udphdr);

        build_udp_packet(
            buffer + offset,
            transport_size,
            config,
            ip);

        offset += sizeof(struct udphdr);

        break;

    case PROTOCOL_TCP:

        if (config->packet_size <
            offset +
            sizeof(struct tcphdr)) {
            return -1;
        }

        transport_size =
            config->packet_size -
            offset -
            sizeof(struct tcphdr);

        build_tcp_packet(
            buffer + offset,
            transport_size,
            config,
            ip);

        offset += sizeof(struct tcphdr);

        break;

    case PROTOCOL_ICMP:

        if (config->packet_size <
            offset +
            sizeof(struct icmphdr)) {
            return -1;
        }

        transport_size =
            config->packet_size -
            offset -
            sizeof(struct icmphdr);

        build_icmp_packet(
            buffer + offset,
            transport_size);

        offset += sizeof(struct icmphdr);

        break;
    }

    /*
     * Payload.
     */

    if (offset < config->packet_size) {

        fill_payload(
            buffer + offset,
            config->packet_size - offset,
            packet_number);
    }

    /*
     * Recalculate IPv4 checksum after packet construction.
     */

    ip->tot_len =
        htons((uint16_t)(
            config->packet_size -
            sizeof(struct ethhdr)));

    ip->check = 0;

    ip->check =
        ip_checksum(ip);

    return 0;
}

/*
 * ------------------------------------------------------------
 * Hex Dump
 * ------------------------------------------------------------
 */

static void dump_packet(
        const uint8_t *packet,
        size_t length)
{
    printf("\nPacket dump (%zu bytes):\n",
           length);

    for (size_t i = 0; i < length; i++) {

        if ((i % 16) == 0) {
            printf("%04zx: ", i);
        }

        printf("%02x ", packet[i]);

        if ((i % 16) == 15 ||
            i == length - 1) {

            printf("\n");
        }
    }

    printf("\n");
}

/*
 * ------------------------------------------------------------
 * Interface MAC
 * ------------------------------------------------------------
 */

static int get_interface_mac(
        int fd,
        const char *interface,
        uint8_t mac[ETH_ALEN])
{
    struct ifreq ifr;

    memset(
        &ifr,
        0,
        sizeof(ifr));

    strncpy(
        ifr.ifr_name,
        interface,
        IFNAMSIZ - 1);

    if (ioctl(
            fd,
            SIOCGIFHWADDR,
            &ifr) < 0) {

        return -1;
    }

    memcpy(
        mac,
        ifr.ifr_hwaddr.sa_data,
        ETH_ALEN);

    return 0;
}

/*
 * ------------------------------------------------------------
 * Interface Index
 * ------------------------------------------------------------
 */

static int get_interface_index(
        int fd,
        const char *interface)
{
    struct ifreq ifr;

    memset(
        &ifr,
        0,
        sizeof(ifr));

    strncpy(
        ifr.ifr_name,
        interface,
        IFNAMSIZ - 1);

    if (ioctl(
            fd,
            SIOCGIFINDEX,
            &ifr) < 0) {

        return -1;
    }

    return ifr.ifr_ifindex;
}

/*
 * ------------------------------------------------------------
 * Interface Information
 * ------------------------------------------------------------
 */

static int print_interface_information(
        int fd,
        const char *interface)
{
    uint8_t mac[ETH_ALEN];

    char mac_string[32];

    int index;

    index =
        get_interface_index(
            fd,
            interface);

    if (index < 0) {

        perror("SIOCGIFINDEX");

        return -1;
    }

    if (get_interface_mac(
            fd,
            interface,
            mac) < 0) {

        perror("SIOCGIFHWADDR");

        return -1;
    }

    mac_to_string(
        mac,
        mac_string,
        sizeof(mac_string));

    printf("Interface Information\n");
    printf("---------------------\n");

    printf("Interface : %s\n",
           interface);

    printf("Index     : %d\n",
           index);

    printf("MAC       : %s\n",
           mac_string);

    printf("\n");

    return 0;
}

/*
 * ------------------------------------------------------------
 * Bind Packet Socket
 * ------------------------------------------------------------
 */

static int bind_packet_socket(
        int fd,
        const char *interface)
{
    struct sockaddr_ll address;

    int interface_index;

    interface_index =
        get_interface_index(
            fd,
            interface);

    if (interface_index < 0) {
        return -1;
    }

    memset(
        &address,
        0,
        sizeof(address));

    address.sll_family =
        AF_PACKET;

    address.sll_protocol =
        htons(ETH_P_ALL);

    address.sll_ifindex =
        interface_index;

    address.sll_halen =
        ETH_ALEN;

    if (bind(
            fd,
            (struct sockaddr *)&address,
            sizeof(address)) < 0) {

        return -1;
    }

    return 0;
}

/*
 * ------------------------------------------------------------
 * Send Packet
 * ------------------------------------------------------------
 */

static ssize_t send_packet(
        int fd,
        const uint8_t *packet,
        size_t packet_size)
{
    struct sockaddr_ll address;

    const struct ethhdr *eth;

    memset(
        &address,
        0,
        sizeof(address));

    eth =
        (const struct ethhdr *)packet;

    address.sll_family =
        AF_PACKET;

    address.sll_halen =
        ETH_ALEN;

    memcpy(
        address.sll_addr,
        eth->h_dest,
        ETH_ALEN);

    /*
     * The socket is already bound to the
     * interface, therefore sll_ifindex
     * can be left unchanged here.
     */

    return sendto(
        fd,
        packet,
        packet_size,
        0,
        (struct sockaddr *)&address,
        sizeof(address));
}

/*
 * ------------------------------------------------------------
 * Rate Limiter
 * ------------------------------------------------------------
 */

static void rate_limit(
        uint64_t packet_number,
        uint64_t start_time_ns,
        uint64_t rate)
{
    uint64_t target_ns;

    uint64_t current_ns;

    if (rate == 0) {
        return;
    }

    /*
     * Target time:
     *
     * packet_number / rate
     */

    target_ns =
        (packet_number * 1000000000ULL) /
        rate;

    for (;;) {

        current_ns =
            get_time_ns() -
            start_time_ns;

        if (current_ns >= target_ns) {
            break;
        }

        /*
         * Avoid busy spinning for larger intervals.
         */

        if ((target_ns - current_ns) >
            1000000ULL) {

            struct timespec ts;

            uint64_t remaining =
                target_ns - current_ns;

            ts.tv_sec =
                remaining / 1000000000ULL;

            ts.tv_nsec =
                remaining % 1000000000ULL;

            nanosleep(
                &ts,
                NULL);

        } else {

            sched_yield();
        }
    }
}

/*
 * ------------------------------------------------------------
 * Print Statistics
 * ------------------------------------------------------------
 */

static void print_statistics(
        const struct generator_stats *stats)
{
    uint64_t elapsed_ns;

    double seconds;

    double pps;

    double mbps;

    elapsed_ns =
        stats->end_ns -
        stats->start_ns;

    seconds =
        (double)elapsed_ns /
        1000000000.0;

    if (seconds <= 0.0) {
        seconds = 0.000000001;
    }

    pps =
        (double)stats->packets_sent /
        seconds;

    mbps =
        ((double)stats->bytes_sent * 8.0) /
        seconds /
        1000000.0;

    printf("\n");
    printf("============================================================\n");
    printf("              PACKET GENERATOR STATISTICS\n");
    printf("============================================================\n");

    printf("Packets generated : %lu\n",
           (unsigned long)
           stats->packets_generated);

    printf("Packets sent      : %lu\n",
           (unsigned long)
           stats->packets_sent);

    printf("Packets failed    : %lu\n",
           (unsigned long)
           stats->packets_failed);

    printf("Bytes sent        : %lu\n",
           (unsigned long)
           stats->bytes_sent);

    printf("Elapsed time      : %.3f sec\n",
           seconds);

    printf("Packets/sec       : %.2f\n",
           pps);

    printf("Throughput        : %.2f Mbps\n",
           mbps);

    printf("============================================================\n");
    printf("\n");
}

/*
 * ------------------------------------------------------------
 * Generate and Send
 * ------------------------------------------------------------
 */

static int run_generator(
        int fd,
        const struct generator_config *config,
        struct generator_stats *stats)
{
    uint8_t *packet;

    uint64_t packet_number;

    uint64_t start_time_ns;

    packet =
        malloc(config->packet_size);

    if (packet == NULL) {

        fprintf(
            stderr,
            "[ERROR] Unable to allocate packet buffer\n");

        return -1;
    }

    memset(
        stats,
        0,
        sizeof(*stats));

    stats->start_ns =
        get_time_ns();

    start_time_ns =
        stats->start_ns;

    packet_number = 0;

    printf("[INFO] Packet generation started.\n");

    if (config->continuous) {

        printf("[INFO] Continuous mode enabled.\n");

        printf("[INFO] Press Ctrl+C to stop.\n");
    }

    while (!stop_generator) {

        /*
         * Stop after requested count.
         */

        if (!config->continuous &&
            packet_number >=
            config->packet_count) {

            break;
        }

        /*
         * Randomization.
         */

        if (config->randomize) {

            struct generator_config temp;

            memcpy(
                &temp,
                config,
                sizeof(temp));

            random_mac(
                temp.src_mac);

            temp.src_ip.s_addr =
                random_ipv4();

            if (build_packet(
                    packet,
                    config->packet_size,
                    &temp,
                    packet_number) != 0) {

                fprintf(
                    stderr,
                    "[ERROR] Packet build failed\n");

                stats->packets_failed++;

                packet_number++;

                continue;
            }

        } else {

            if (build_packet(
                    packet,
                    config->packet_size,
                    config,
                    packet_number) != 0) {

                fprintf(
                    stderr,
                    "[ERROR] Packet build failed\n");

                free(packet);

                return -1;
            }
        }

        /*
         * Packet dump only for the first packet.
         */

        if (config->dump_packet &&
            packet_number == 0) {

            dump_packet(
                packet,
                config->packet_size);
        }

        stats->packets_generated++;

        /*
         * Dry run.
         */

        if (!config->dry_run) {

            ssize_t sent;

            sent =
                send_packet(
                    fd,
                    packet,
                    config->packet_size);

            if (sent < 0) {

                stats->packets_failed++;

                if (config->verbose) {

                    fprintf(
                        stderr,
                        "[ERROR] sendto(): %s\n",
                        strerror(errno));
                }

            } else if ((size_t)sent !=
                       config->packet_size) {

                stats->packets_failed++;

                if (config->verbose) {

                    fprintf(
                        stderr,
                        "[ERROR] Partial packet sent: "
                        "%zd/%zu\n",
                        sent,
                        config->packet_size);
                }

            } else {

                stats->packets_sent++;

                stats->bytes_sent +=
                    (uint64_t)sent;
            }

        } else {

            stats->packets_sent++;

            stats->bytes_sent +=
                config->packet_size;
        }

        packet_number++;

        /*
         * Rate limiting.
         */

        rate_limit(
            packet_number,
            start_time_ns,
            config->rate);

        /*
         * Progress.
         */

        if (config->verbose &&
            ((packet_number % 1000ULL) == 0)) {

            printf(
                "[INFO] Packets: %lu\n",
                (unsigned long)
                packet_number);
        }
    }

    stats->end_ns =
        get_time_ns();

    free(packet);

    printf("[INFO] Packet generation stopped.\n");

    return 0;
}

/*
 * ------------------------------------------------------------
 * Configuration Defaults
 * ------------------------------------------------------------
 */

static void initialize_config(
        struct generator_config *config)
{
    memset(
        config,
        0,
        sizeof(*config));

    strncpy(
        config->interface,
        DEFAULT_INTERFACE,
        IFNAMSIZ - 1);

    /*
     * Default source MAC.
     *
     * This should normally be replaced by the
     * actual interface MAC.
     */

    config->src_mac[0] = 0x02;
    config->src_mac[1] = 0x00;
    config->src_mac[2] = 0x00;
    config->src_mac[3] = 0x00;
    config->src_mac[4] = 0x00;
    config->src_mac[5] = 0x01;

    /*
     * Broadcast destination.
     *
     * User should normally provide the actual
     * destination MAC.
     */

    memset(
        config->dst_mac,
        0xff,
        ETH_ALEN);

    inet_pton(
        AF_INET,
        "192.168.1.100",
        &config->src_ip);

    inet_pton(
        AF_INET,
        "192.168.1.1",
        &config->dst_ip);

    config->src_port =
        DEFAULT_SRC_PORT;

    config->dst_port =
        DEFAULT_DST_PORT;

    config->protocol =
        PROTOCOL_UDP;

    config->packet_size =
        DEFAULT_PACKET_SIZE;

    config->packet_count =
        DEFAULT_PACKET_COUNT;

    config->rate =
        DEFAULT_RATE;

    config->continuous = 0;

    config->verbose = 0;

    config->dump_packet = 0;

    config->dry_run = 0;

    config->randomize = 0;

    config->ttl = 64;
}

/*
 * ------------------------------------------------------------
 * Parse Integer
 * ------------------------------------------------------------
 */

static int parse_uint64(
        const char *text,
        uint64_t *value)
{
    char *end;

    unsigned long long result;

    if (text == NULL ||
        value == NULL) {

        return -1;
    }

    errno = 0;

    result =
        strtoull(
            text,
            &end,
            10);

    if (errno != 0 ||
        end == text ||
        *end != '\0') {

        return -1;
    }

    *value =
        (uint64_t)result;

    return 0;
}

/*
 * ------------------------------------------------------------
 * Parse Packet Size
 * ------------------------------------------------------------
 */

static int parse_packet_size(
        const char *text,
        size_t *size)
{
    uint64_t value;

    if (parse_uint64(
            text,
            &value) != 0) {

        return -1;
    }

    if (value < MIN_PACKET_SIZE ||
        value > MAX_PACKET_SIZE) {

        fprintf(
            stderr,
            "Packet size must be between "
            "%d and %d bytes\n",
            MIN_PACKET_SIZE,
            MAX_PACKET_SIZE);

        return -1;
    }

    *size =
        (size_t)value;

    return 0;
}

/*
 * ------------------------------------------------------------
 * Parse Arguments
 * ------------------------------------------------------------
 */

static int parse_arguments(
        int argc,
        char **argv,
        struct generator_config *config)
{
    static const struct option long_options[] = {

        {"interface", required_argument, 0, 'i'},

        {"src-mac", required_argument, 0, 's'},

        {"dst-mac", required_argument, 0, 'd'},

        {"src-ip", required_argument, 0, 'S'},

        {"dst-ip", required_argument, 0, 'D'},

        {"src-port", required_argument, 0, 'p'},

        {"dst-port", required_argument, 0, 'P'},

        {"protocol", required_argument, 0, 't'},

        {"size", required_argument, 0, 'z'},

        {"count", required_argument, 0, 'c'},

        {"rate", required_argument, 0, 'r'},

        {"continuous", no_argument, 0, 'C'},

        {"verbose", no_argument, 0, 'v'},

        {"dump", no_argument, 0, 'x'},

        {"dry-run", no_argument, 0, 'n'},

        {"random", no_argument, 0, 'R'},

        {"ttl", required_argument, 0, 'T'},

        {"help", no_argument, 0, 'h'},

        {0, 0, 0, 0}
    };

    int option;

    int option_index = 0;

    while ((option = getopt_long(
                argc,
                argv,
                "i:s:d:S:D:p:P:t:z:c:r:Cvx nRT:h",
                long_options,
                &option_index)) != -1) {

        switch (option) {

        case 'i':

            strncpy(
                config->interface,
                optarg,
                IFNAMSIZ - 1);

            config->interface[
                IFNAMSIZ - 1] = '\0';

            break;

        case 's':

            if (parse_mac(
                    optarg,
                    config->src_mac) != 0) {

                fprintf(
                    stderr,
                    "Invalid source MAC: %s\n",
                    optarg);

                return -1;
            }

            break;

        case 'd':

            if (parse_mac(
                    optarg,
                    config->dst_mac) != 0) {

                fprintf(
                    stderr,
                    "Invalid destination MAC: %s\n",
                    optarg);

                return -1;
            }

            break;

        case 'S':

            if (inet_pton(
                    AF_INET,
                    optarg,
                    &config->src_ip) != 1) {

                fprintf(
                    stderr,
                    "Invalid source IPv4: %s\n",
                    optarg);

                return -1;
            }

            break;

        case 'D':

            if (inet_pton(
                    AF_INET,
                    optarg,
                    &config->dst_ip) != 1) {

                fprintf(
                    stderr,
                    "Invalid destination IPv4: %s\n",
                    optarg);

                return -1;
            }

            break;

        case 'p': {

            uint64_t value;

            if (parse_uint64(
                    optarg,
                    &value) != 0 ||
                value > 65535) {

                fprintf(
                    stderr,
                    "Invalid source port\n");

                return -1;
            }

            config->src_port =
                (uint16_t)value;

            break;
        }

        case 'P': {

            uint64_t value;

            if (parse_uint64(
                    optarg,
                    &value) != 0 ||
                value > 65535) {

                fprintf(
                    stderr,
                    "Invalid destination port\n");

                return -1;
            }

            config->dst_port =
                (uint16_t)value;

            break;
        }

        case 't':

            if (parse_protocol(
                    optarg,
                    &config->protocol) != 0) {

                fprintf(
                    stderr,
                    "Supported protocols: "
                    "udp, tcp, icmp\n");

                return -1;
            }

            break;

        case 'z':

            if (parse_packet_size(
                    optarg,
                    &config->packet_size) != 0) {

                return -1;
            }

            break;

        case 'c':

            if (parse_uint64(
                    optarg,
                    &config->packet_count) != 0) {

                fprintf(
                    stderr,
                    "Invalid packet count\n");

                return -1;
            }

            break;

        case 'r':

            if (parse_uint64(
                    optarg,
                    &config->rate) != 0) {

                fprintf(
                    stderr,
                    "Invalid packet rate\n");

                return -1;
            }

            break;

        case 'C':

            config->continuous = 1;

            break;

        case 'v':

            config->verbose = 1;

            break;

        case 'x':

            config->dump_packet = 1;

            break;

        case 'n':

            config->dry_run = 1;

            break;

        case 'R':

            config->randomize = 1;

            break;

        case 'T': {

            uint64_t value;

            if (parse_uint64(
                    optarg,
                    &value) != 0 ||
                value > 255) {

                fprintf(
                    stderr,
                    "Invalid TTL\n");

                return -1;
            }

            config->ttl =
                (uint8_t)value;

            break;
        }

        case 'h':

            return 1;

        default:

            return -1;
        }
    }

    if (!config->continuous &&
        config->packet_count == 0) {

        fprintf(
            stderr,
            "Packet count must be greater than zero\n");

        return -1;
    }

    return 0;
}

/*
 * ------------------------------------------------------------
 * Print Configuration
 * ------------------------------------------------------------
 */

static void print_configuration(
        const struct generator_config *config)
{
    char src_mac[32];

    char dst_mac[32];

    char src_ip[INET_ADDRSTRLEN];

    char dst_ip[INET_ADDRSTRLEN];

    mac_to_string(
        config->src_mac,
        src_mac,
        sizeof(src_mac));

    mac_to_string(
        config->dst_mac,
        dst_mac,
        sizeof(dst_mac));

    inet_ntop(
        AF_INET,
        &config->src_ip,
        src_ip,
        sizeof(src_ip));

    inet_ntop(
        AF_INET,
        &config->dst_ip,
        dst_ip,
        sizeof(dst_ip));

    printf("\n");
    printf("Packet Generator Configuration\n");
    printf("-------------------------------\n");

    printf("Interface    : %s\n",
           config->interface);

    printf("Source MAC   : %s\n",
           src_mac);

    printf("Destination MAC: %s\n",
           dst_mac);

    printf("Source IP    : %s\n",
           src_ip);

    printf("Destination IP: %s\n",
           dst_ip);

    printf("Source port  : %u\n",
           config->src_port);

    printf("Destination port: %u\n",
           config->dst_port);

    printf("Protocol     : %s\n",
           protocol_name(config->protocol));

    printf("Packet size  : %zu bytes\n",
           config->packet_size);

    printf("Packet count : %lu\n",
           (unsigned long)
           config->packet_count);

    printf("Packet rate  : %lu pps\n",
           (unsigned long)
           config->rate);

    printf("TTL          : %u\n",
           config->ttl);

    printf("Continuous   : %s\n",
           config->continuous ?
           "yes" : "no");

    printf("Randomize    : %s\n",
           config->randomize ?
           "yes" : "no");

    printf("Dry run      : %s\n",
           config->dry_run ?
           "yes" : "no");

    printf("\n");
}

/*
 * ------------------------------------------------------------
 * Usage
 * ------------------------------------------------------------
 */

static void print_usage(
        const char *program)
{
    printf("\n");

    printf("Usage:\n");

    printf("  sudo %s [options]\n",
           program);

    printf("\n");

    printf("Options:\n");

    printf("  -i, --interface <name>\n");
    printf("      Network interface. Default: eth0\n");

    printf("\n");

    printf("  -s, --src-mac <mac>\n");
    printf("      Source MAC address.\n");

    printf("\n");

    printf("  -d, --dst-mac <mac>\n");
    printf("      Destination MAC address.\n");

    printf("\n");

    printf("  -S, --src-ip <ip>\n");
    printf("      Source IPv4 address.\n");

    printf("\n");

    printf("  -D, --dst-ip <ip>\n");
    printf("      Destination IPv4 address.\n");

    printf("\n");

    printf("  -p, --src-port <port>\n");
    printf("      Source TCP/UDP port.\n");

    printf("\n");

    printf("  -P, --dst-port <port>\n");
    printf("      Destination TCP/UDP port.\n");

    printf("\n");

    printf("  -t, --protocol <udp|tcp|icmp>\n");
    printf("      Packet protocol. Default: udp\n");

    printf("\n");

    printf("  -z, --size <bytes>\n");
    printf("      Packet size. Default: 128\n");

    printf("\n");

    printf("  -c, --count <count>\n");
    printf("      Number of packets. Default: 1000\n");

    printf("\n");

    printf("  -r, --rate <pps>\n");
    printf("      Packet rate. 0 = unlimited.\n");

    printf("\n");

    printf("  -C, --continuous\n");
    printf("      Generate packets until Ctrl+C.\n");

    printf("\n");

    printf("  -v, --verbose\n");
    printf("      Verbose output.\n");

    printf("\n");

    printf("  -x, --dump\n");
    printf("      Hex dump first generated packet.\n");

    printf("\n");

    printf("  -n, --dry-run\n");
    printf("      Build packets but do not transmit.\n");

    printf("\n");

    printf("  -R, --random\n");
    printf("      Randomize source MAC/IP per packet.\n");

    printf("\n");

    printf("  -T, --ttl <value>\n");
    printf("      IPv4 TTL. Default: 64\n");

    printf("\n");

    printf("  -h, --help\n");
    printf("      Display this help.\n");

    printf("\n");

    printf("Examples:\n");

    printf("\n");

    printf("  sudo %s \\\n",
           program);

    printf("      --interface eth0 \\\n");

    printf("      --protocol udp \\\n");

    printf("      --size 512 \\\n");

    printf("      --count 10000\n");

    printf("\n");

    printf("  sudo %s \\\n",
           program);

    printf("      --interface eth0 \\\n");

    printf("      --protocol tcp \\\n");

    printf("      --rate 10000 \\\n");

    printf("      --continuous\n");

    printf("\n");

    printf("  sudo %s \\\n",
           program);

    printf("      --interface eth0 \\\n");

    printf("      --dry-run \\\n");

    printf("      --dump \\\n");

    printf("      --count 1\n");

    printf("\n");
}

/*
 * ------------------------------------------------------------
 * Main
 * ------------------------------------------------------------
 */

int main(
        int argc,
        char **argv)
{
    struct generator_config config;

    struct generator_stats stats;

    int fd = -1;

    int ret;

    srand(
        (unsigned int)
        time(NULL));

    initialize_config(
        &config);

    ret =
        parse_arguments(
            argc,
            argv,
            &config);

    if (ret == 1) {

        print_usage(argv[0]);

        return EXIT_SUCCESS;
    }

    if (ret != 0) {

        print_usage(argv[0]);

        return EXIT_FAILURE;
    }

    /*
     * --------------------------------------------------------
     * Signal handlers
     * --------------------------------------------------------
     */

    signal(
        SIGINT,
        signal_handler);

    signal(
        SIGTERM,
        signal_handler);

    /*
     * --------------------------------------------------------
     * Configuration
     * --------------------------------------------------------
     */

    print_configuration(
        &config);

    /*
     * --------------------------------------------------------
     * Dry-run does not need a raw socket.
     * --------------------------------------------------------
     */

    if (config.dry_run) {

        printf(
            "[INFO] Dry-run mode: "
            "no packets will be transmitted.\n");

        if (run_generator(
                -1,
                &config,
                &stats) != 0) {

            return EXIT_FAILURE;
        }

        print_statistics(
            &stats);

        return EXIT_SUCCESS;
    }

    /*
     * --------------------------------------------------------
     * Create raw packet socket.
     * --------------------------------------------------------
     */

    printf(
        "[INFO] Creating AF_PACKET raw socket...\n");

    fd =
        socket(
            AF_PACKET,
            SOCK_RAW,
            htons(ETH_P_ALL));

    if (fd < 0) {

        fprintf(
            stderr,
            "[ERROR] Unable to create raw socket: %s\n",
            strerror(errno));

        fprintf(
            stderr,
            "[INFO] Run as root or grant CAP_NET_RAW.\n");

        return EXIT_FAILURE;
    }

    printf(
        "[PASS] Raw packet socket created.\n");

    /*
     * --------------------------------------------------------
     * Interface information.
     * --------------------------------------------------------
     */

    if (print_interface_information(
            fd,
            config.interface) != 0) {

        fprintf(
            stderr,
            "[ERROR] Unable to access interface %s\n",
            config.interface);

        close(fd);

        return EXIT_FAILURE;
    }

    /*
     * --------------------------------------------------------
     * Bind socket.
     * --------------------------------------------------------
     */

    if (bind_packet_socket(
            fd,
            config.interface) != 0) {

        fprintf(
            stderr,
            "[ERROR] Unable to bind socket to %s: %s\n",
            config.interface,
            strerror(errno));

        close(fd);

        return EXIT_FAILURE;
    }

    printf(
        "[PASS] Socket bound to %s.\n",
        config.interface);

    /*
     * --------------------------------------------------------
     * Generate and transmit.
     * --------------------------------------------------------
     */

    if (run_generator(
            fd,
            &config,
            &stats) != 0) {

        close(fd);

        return EXIT_FAILURE;
    }

    /*
     * --------------------------------------------------------
     * Statistics.
     * --------------------------------------------------------
     */

    print_statistics(
        &stats);

    /*
     * --------------------------------------------------------
     * Close socket.
     * --------------------------------------------------------
     */

    close(fd);

    printf(
        "[PASS] Packet generator completed.\n");

    return EXIT_SUCCESS;
}
