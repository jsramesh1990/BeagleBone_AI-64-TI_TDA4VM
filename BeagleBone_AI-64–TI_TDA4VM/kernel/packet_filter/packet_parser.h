/*
 * Packet Filter Driver - Packet Parser Header
 *
 * BeagleBone AI-64 / TI TDA4VM
 *
 * Provides packet parsing definitions and APIs for extracting
 * Ethernet, IPv4, TCP, UDP and ICMP information from Linux
 * sk_buff packets.
 */

#ifndef PACKET_FILTER_PACKET_PARSER_H
#define PACKET_FILTER_PACKET_PARSER_H

#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

/* ============================================================
 * Parser Constants
 * ============================================================ */

#define PF_ETH_ADDR_LEN        6
#define PF_IPV4_ADDR_LEN       4

#define PF_MAX_PACKET_HEADERS  8

/* ============================================================
 * Packet Protocol Types
 * ============================================================ */

enum pf_packet_protocol {
    PF_PKT_PROTO_UNKNOWN = 0,
    PF_PKT_PROTO_TCP,
    PF_PKT_PROTO_UDP,
    PF_PKT_PROTO_ICMP,
};

/* ============================================================
 * Ethernet Information
 * ============================================================ */

struct pf_eth_info {
    __u8 src_mac[PF_ETH_ADDR_LEN];
    __u8 dst_mac[PF_ETH_ADDR_LEN];

    __be16 ethertype;
};


/* ============================================================
 * IPv4 Information
 * ============================================================ */

struct pf_ipv4_info {
    __u8 version;
    __u8 ihl;

    __u8 tos;

    __be16 total_length;
    __be16 identification;

    __be16 fragment_offset;

    __u8 ttl;
    __u8 protocol;

    __be16 checksum;

    __be32 src_ip;
    __be32 dst_ip;
};


/* ============================================================
 * TCP Information
 * ============================================================ */

struct pf_tcp_info {
    __be16 src_port;
    __be16 dst_port;

    __be32 sequence;
    __be32 acknowledgment;

    __u8 data_offset;
    __u8 flags;

    __be16 window;
    __be16 checksum;
    __be16 urgent_pointer;
};


/* ============================================================
 * UDP Information
 * ============================================================ */

struct pf_udp_info {
    __be16 src_port;
    __be16 dst_port;

    __be16 length;
    __be16 checksum;
};


/* ============================================================
 * ICMP Information
 * ============================================================ */

struct pf_icmp_info {
    __u8 type;
    __u8 code;

    __be16 checksum;

    __be32 rest_of_header;
};


/* ============================================================
 * Complete Parsed Packet
 * ============================================================ */

struct pf_parsed_packet {
    /*
     * Original socket buffer.
     */
    struct sk_buff *skb;

    /*
     * Network interface.
     */
    struct net_device *dev;

    /*
     * Packet length.
     */
    unsigned int packet_length;

    /*
     * Ethernet header information.
     */
    struct pf_eth_info eth;

    /*
     * IPv4 header information.
     */
    struct pf_ipv4_info ipv4;

    /*
     * Transport protocol information.
     */
    union {
        struct pf_tcp_info tcp;
        struct pf_udp_info udp;
        struct pf_icmp_info icmp;
    } transport;

    /*
     * Identified protocol.
     */
    enum pf_packet_protocol protocol;

    /*
     * Header lengths.
     */
    unsigned int ethernet_header_length;
    unsigned int ip_header_length;
    unsigned int transport_header_length;

    /*
     * Payload information.
     */
    unsigned int payload_offset;
    unsigned int payload_length;

    /*
     * Parsing status.
     */
    bool ethernet_valid;
    bool ipv4_valid;
    bool transport_valid;
};


/* ============================================================
 * Parser Result
 * ============================================================ */

enum pf_parser_result {
    PF_PARSE_SUCCESS = 0,
    PF_PARSE_INVALID_PACKET = -EINVAL,
    PF_PARSE_UNSUPPORTED_PROTOCOL = -EOPNOTSUPP,
    PF_PARSE_TRUNCATED_PACKET = -EMSGSIZE,
    PF_PARSE_INVALID_ETHERNET = -EPROTO,
    PF_PARSE_INVALID_IPV4 = -EPROTO,
    PF_PARSE_INVALID_TRANSPORT = -EPROTO,
};


/* ============================================================
 * Ethernet Parsing
 * ============================================================ */

/*
 * Parse Ethernet header.
 *
 * @skb:    Packet buffer.
 * @eth:    Ethernet information output.
 *
 * Returns:
 *   0 on success
 *   Negative error code on failure.
 */
int pf_parse_ethernet(struct sk_buff *skb,
                      struct pf_eth_info *eth);


/*
 * Check whether packet contains IPv4 Ethernet frame.
 */
bool pf_is_ipv4_packet(const struct pf_eth_info *eth);


/* ============================================================
 * IPv4 Parsing
 * ============================================================ */

/*
 * Parse IPv4 header.
 */
int pf_parse_ipv4(struct sk_buff *skb,
                  unsigned int offset,
                  struct pf_ipv4_info *ipv4);


/*
 * Validate IPv4 header.
 */
bool pf_validate_ipv4(const struct pf_ipv4_info *ipv4);


/*
 * Get IPv4 header length.
 */
unsigned int pf_ipv4_header_length(
    const struct pf_ipv4_info *ipv4);


/* ============================================================
 * TCP Parsing
 * ============================================================ */

/*
 * Parse TCP header.
 */
int pf_parse_tcp(struct sk_buff *skb,
                 unsigned int offset,
                 struct pf_tcp_info *tcp);


/*
 * Validate TCP header.
 */
bool pf_validate_tcp(const struct pf_tcp_info *tcp);


/*
 * Get TCP header length.
 */
unsigned int pf_tcp_header_length(
    const struct pf_tcp_info *tcp);


/* ============================================================
 * UDP Parsing
 * ============================================================ */

/*
 * Parse UDP header.
 */
int pf_parse_udp(struct sk_buff *skb,
                 unsigned int offset,
                 struct pf_udp_info *udp);


/*
 * Validate UDP header.
 */
bool pf_validate_udp(const struct pf_udp_info *udp);


/* ============================================================
 * ICMP Parsing
 * ============================================================ */

/*
 * Parse ICMP header.
 */
int pf_parse_icmp(struct sk_buff *skb,
                  unsigned int offset,
                  struct pf_icmp_info *icmp);


/*
 * Validate ICMP header.
 */
bool pf_validate_icmp(const struct pf_icmp_info *icmp);


/* ============================================================
 * Complete Packet Parsing
 * ============================================================ */

/*
 * Parse complete packet.
 *
 * Performs:
 *
 * Ethernet
 *    ↓
 * IPv4
 *    ↓
 * TCP / UDP / ICMP
 */
int pf_parse_packet(struct sk_buff *skb,
                    struct pf_parsed_packet *packet);


/*
 * Validate complete parsed packet.
 */
bool pf_validate_packet(
    const struct pf_parsed_packet *packet);


/*
 * Reset parsed packet structure.
 */
void pf_reset_parsed_packet(
    struct pf_parsed_packet *packet);


/* ============================================================
 * Protocol Helpers
 * ============================================================ */

/*
 * Convert IP protocol number to internal protocol type.
 */
enum pf_packet_protocol
pf_protocol_from_ip_header(__u8 protocol);


/*
 * Return protocol name.
 */
const char *pf_packet_protocol_name(
    enum pf_packet_protocol protocol);


/*
 * Check whether protocol is supported.
 */
bool pf_packet_protocol_supported(__u8 protocol);


/* ============================================================
 * Address Helpers
 * ============================================================ */

/*
 * Compare IPv4 addresses using address and mask.
 */
bool pf_ipv4_address_match(__be32 packet_ip,
                           __be32 rule_ip,
                           __be32 rule_mask);


/*
 * Compare port against port range.
 */
bool pf_port_match(__be16 packet_port,
                   __be16 start_port,
                   __be16 end_port);


/* ============================================================
 * Packet Offset Helpers
 * ============================================================ */

/*
 * Get transport-header offset.
 */
unsigned int pf_get_transport_offset(
    const struct pf_parsed_packet *packet);


/*
 * Get payload offset.
 */
unsigned int pf_get_payload_offset(
    const struct pf_parsed_packet *packet);


/*
 * Get payload length.
 */
unsigned int pf_get_payload_length(
    const struct pf_parsed_packet *packet);


/* ============================================================
 * Debug / Dump Helpers
 * ============================================================ */

/*
 * Print parsed packet information to kernel log.
 */
void pf_dump_parsed_packet(
    const struct pf_parsed_packet *packet);


/*
 * Print Ethernet information.
 */
void pf_dump_ethernet(
    const struct pf_eth_info *eth);


/*
 * Print IPv4 information.
 */
void pf_dump_ipv4(
    const struct pf_ipv4_info *ipv4);


/*
 * Print TCP information.
 */
void pf_dump_tcp(
    const struct pf_tcp_info *tcp);


/*
 * Print UDP information.
 */
void pf_dump_udp(
    const struct pf_udp_info *udp);


/*
 * Print ICMP information.
 */
void pf_dump_icmp(
    const struct pf_icmp_info *icmp);


/* ============================================================
 * Inline Helpers
 * ============================================================ */

/*
 * Check whether packet is TCP.
 */
static inline bool pf_packet_is_tcp(
    const struct pf_parsed_packet *packet)
{
    return packet &&
           packet->protocol == PF_PKT_PROTO_TCP;
}


/*
 * Check whether packet is UDP.
 */
static inline bool pf_packet_is_udp(
    const struct pf_parsed_packet *packet)
{
    return packet &&
           packet->protocol == PF_PKT_PROTO_UDP;
}


/*
 * Check whether packet is ICMP.
 */
static inline bool pf_packet_is_icmp(
    const struct pf_parsed_packet *packet)
{
    return packet &&
           packet->protocol == PF_PKT_PROTO_ICMP;
}


/*
 * Check whether packet has a valid IPv4 header.
 */
static inline bool pf_packet_has_ipv4(
    const struct pf_parsed_packet *packet)
{
    return packet && packet->ipv4_valid;
}


/*
 * Get source IPv4 address.
 */
static inline __be32 pf_packet_src_ip(
    const struct pf_parsed_packet *packet)
{
    if (!packet)
        return 0;

    return packet->ipv4.src_ip;
}


/*
 * Get destination IPv4 address.
 */
static inline __be32 pf_packet_dst_ip(
    const struct pf_parsed_packet *packet)
{
    if (!packet)
        return 0;

    return packet->ipv4.dst_ip;
}


/*
 * Get source port.
 */
static inline __be16 pf_packet_src_port(
    const struct pf_parsed_packet *packet)
{
    if (!packet)
        return 0;

    if (packet->protocol == PF_PKT_PROTO_TCP)
        return packet->transport.tcp.src_port;

    if (packet->protocol == PF_PKT_PROTO_UDP)
        return packet->transport.udp.src_port;

    return 0;
}


/*
 * Get destination port.
 */
static inline __be16 pf_packet_dst_port(
    const struct pf_parsed_packet *packet)
{
    if (!packet)
        return 0;

    if (packet->protocol == PF_PKT_PROTO_TCP)
        return packet->transport.tcp.dst_port;

    if (packet->protocol == PF_PKT_PROTO_UDP)
        return packet->transport.udp.dst_port;

    return 0;
}


#endif /* PACKET_FILTER_PACKET_PARSER_H */
