/*
 * Packet Filter Driver - Packet Parser
 *
 * BeagleBone AI-64 / TI TDA4VM
 *
 * Parses:
 *   Ethernet
 *      -> IPv4
 *          -> TCP / UDP / ICMP
 *
 * The parser only reads packet data. It does not make the
 * final ALLOW / DROP decision.
 */

#include "packet_parser.h"
#include "logging.h"

#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/skbuff.h>
#include <linux/string.h>

/* ============================================================
 * Module Information
 * ============================================================ */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BeagleBone AI-64 Packet Filter Project");
MODULE_DESCRIPTION(
    "Packet parser for BeagleBone AI-64 / TI TDA4VM packet filter"
);
MODULE_VERSION("1.0");


/* ============================================================
 * Internal Helpers
 * ============================================================ */

static bool pf_packet_has_data(const struct sk_buff *skb,
                               unsigned int offset,
                               unsigned int length)
{
    if (!skb)
        return false;

    if (offset > skb->len)
        return false;

    if (length > skb->len - offset)
        return false;

    return true;
}


static void *pf_packet_data(const struct sk_buff *skb,
                            unsigned int offset,
                            unsigned int length)
{
    if (!pf_packet_has_data(skb, offset, length))
        return NULL;

    return skb->data + offset;
}


/* ============================================================
 * Reset Parsed Packet
 * ============================================================ */

void pf_reset_parsed_packet(struct pf_parsed_packet *packet)
{
    if (!packet)
        return;

    memset(packet, 0, sizeof(*packet));

    packet->protocol = PF_PKT_PROTO_UNKNOWN;
}


/* ============================================================
 * Ethernet Parsing
 * ============================================================ */

int pf_parse_ethernet(struct sk_buff *skb,
                      struct pf_eth_info *eth)
{
    const struct ethhdr *ethhdr;

    if (!skb || !eth)
        return -EINVAL;

    memset(eth, 0, sizeof(*eth));

    if (!pf_packet_has_data(skb, 0, ETH_HLEN)) {
        pf_log_debug(
            "packet too short for Ethernet header\n"
        );

        return PF_PARSE_TRUNCATED_PACKET;
    }

    ethhdr = pf_packet_data(skb, 0, ETH_HLEN);

    if (!ethhdr)
        return PF_PARSE_INVALID_ETHERNET;

    memcpy(eth->src_mac,
           ethhdr->h_source,
           PF_ETH_ADDR_LEN);

    memcpy(eth->dst_mac,
           ethhdr->h_dest,
           PF_ETH_ADDR_LEN);

    eth->ethertype = ethhdr->h_proto;

    return 0;
}


/* ============================================================
 * IPv4 Check
 * ============================================================ */

bool pf_is_ipv4_packet(const struct pf_eth_info *eth)
{
    if (!eth)
        return false;

    return eth->ethertype == htons(ETH_P_IP);
}


/* ============================================================
 * IPv4 Parsing
 * ============================================================ */

int pf_parse_ipv4(struct sk_buff *skb,
                  unsigned int offset,
                  struct pf_ipv4_info *ipv4)
{
    const struct iphdr *iph;
    unsigned int header_length;

    if (!skb || !ipv4)
        return -EINVAL;

    memset(ipv4, 0, sizeof(*ipv4));

    if (!pf_packet_has_data(skb,
                            offset,
                            sizeof(struct iphdr))) {
        return PF_PARSE_TRUNCATED_PACKET;
    }

    iph = pf_packet_data(
        skb,
        offset,
        sizeof(struct iphdr)
    );

    if (!iph)
        return PF_PARSE_INVALID_IPV4;

    if (iph->version != 4) {
        pf_log_debug(
            "unsupported IP version: %u\n",
            iph->version
        );

        return PF_PARSE_INVALID_IPV4;
    }

    if (iph->ihl < 5) {
        pf_log_debug(
            "invalid IPv4 IHL: %u\n",
            iph->ihl
        );

        return PF_PARSE_INVALID_IPV4;
    }

    header_length = iph->ihl * 4;

    if (!pf_packet_has_data(skb,
                            offset,
                            header_length)) {
        return PF_PARSE_TRUNCATED_PACKET;
    }

    ipv4->version = iph->version;
    ipv4->ihl = iph->ihl;
    ipv4->tos = iph->tos;
    ipv4->total_length = iph->tot_len;
    ipv4->identification = iph->id;
    ipv4->fragment_offset = iph->frag_off;
    ipv4->ttl = iph->ttl;
    ipv4->protocol = iph->protocol;
    ipv4->checksum = iph->check;
    ipv4->src_ip = iph->saddr;
    ipv4->dst_ip = iph->daddr;

    return 0;
}


/* ============================================================
 * IPv4 Validation
 * ============================================================ */

bool pf_validate_ipv4(const struct pf_ipv4_info *ipv4)
{
    unsigned int header_length;
    unsigned int total_length;

    if (!ipv4)
        return false;

    if (ipv4->version != 4)
        return false;

    if (ipv4->ihl < 5)
        return false;

    header_length = ipv4->ihl * 4;

    total_length = ntohs(ipv4->total_length);

    if (total_length < header_length)
        return false;

    return true;
}


/* ============================================================
 * IPv4 Header Length
 * ============================================================ */

unsigned int pf_ipv4_header_length(
    const struct pf_ipv4_info *ipv4)
{
    if (!ipv4)
        return 0;

    if (ipv4->ihl < 5)
        return 0;

    return ipv4->ihl * 4;
}


/* ============================================================
 * TCP Parsing
 * ============================================================ */

int pf_parse_tcp(struct sk_buff *skb,
                 unsigned int offset,
                 struct pf_tcp_info *tcp)
{
    const struct tcphdr *tcph;
    unsigned int header_length;

    if (!skb || !tcp)
        return -EINVAL;

    memset(tcp, 0, sizeof(*tcp));

    if (!pf_packet_has_data(skb,
                            offset,
                            sizeof(struct tcphdr))) {
        return PF_PARSE_TRUNCATED_PACKET;
    }

    tcph = pf_packet_data(
        skb,
        offset,
        sizeof(struct tcphdr)
    );

    if (!tcph)
        return PF_PARSE_INVALID_TRANSPORT;

    if (tcph->doff < 5) {
        pf_log_debug(
            "invalid TCP data offset: %u\n",
            tcph->doff
        );

        return PF_PARSE_INVALID_TRANSPORT;
    }

    header_length = tcph->doff * 4;

    if (!pf_packet_has_data(skb,
                            offset,
                            header_length)) {
        return PF_PARSE_TRUNCATED_PACKET;
    }

    tcp->src_port = tcph->source;
    tcp->dst_port = tcph->dest;
    tcp->sequence = tcph->seq;
    tcp->acknowledgment = tcph->ack_seq;
    tcp->data_offset = tcph->doff;

    tcp->flags = 0;

    if (tcph->fin)
        tcp->flags |= TCP_FLAG_FIN;

    if (tcph->syn)
        tcp->flags |= TCP_FLAG_SYN;

    if (tcph->rst)
        tcp->flags |= TCP_FLAG_RST;

    if (tcph->psh)
        tcp->flags |= TCP_FLAG_PSH;

    if (tcph->ack)
        tcp->flags |= TCP_FLAG_ACK;

    if (tcph->urg)
        tcp->flags |= TCP_FLAG_URG;

    tcp->window = tcph->window;
    tcp->checksum = tcph->check;
    tcp->urgent_pointer = tcph->urg_ptr;

    return 0;
}


/* ============================================================
 * TCP Validation
 * ============================================================ */

bool pf_validate_tcp(const struct pf_tcp_info *tcp)
{
    if (!tcp)
        return false;

    if (tcp->data_offset < 5)
        return false;

    return true;
}


/* ============================================================
 * TCP Header Length
 * ============================================================ */

unsigned int pf_tcp_header_length(
    const struct pf_tcp_info *tcp)
{
    if (!tcp)
        return 0;

    if (tcp->data_offset < 5)
        return 0;

    return tcp->data_offset * 4;
}


/* ============================================================
 * UDP Parsing
 * ============================================================ */

int pf_parse_udp(struct sk_buff *skb,
                 unsigned int offset,
                 struct pf_udp_info *udp)
{
    const struct udphdr *udph;

    if (!skb || !udp)
        return -EINVAL;

    memset(udp, 0, sizeof(*udp));

    if (!pf_packet_has_data(skb,
                            offset,
                            sizeof(struct udphdr))) {
        return PF_PARSE_TRUNCATED_PACKET;
    }

    udph = pf_packet_data(
        skb,
        offset,
        sizeof(struct udphdr)
    );

    if (!udph)
        return PF_PARSE_INVALID_TRANSPORT;

    udp->src_port = udph->source;
    udp->dst_port = udph->dest;
    udp->length = udph->len;
    udp->checksum = udph->check;

    if (ntohs(udp->length) < sizeof(struct udphdr))
        return PF_PARSE_INVALID_TRANSPORT;

    return 0;
}


/* ============================================================
 * UDP Validation
 * ============================================================ */

bool pf_validate_udp(const struct pf_udp_info *udp)
{
    if (!udp)
        return false;

    if (ntohs(udp->length) < sizeof(struct udphdr))
        return false;

    return true;
}


/* ============================================================
 * ICMP Parsing
 * ============================================================ */

int pf_parse_icmp(struct sk_buff *skb,
                  unsigned int offset,
                  struct pf_icmp_info *icmp)
{
    const struct icmphdr *icmph;

    if (!skb || !icmp)
        return -EINVAL;

    memset(icmp, 0, sizeof(*icmp));

    if (!pf_packet_has_data(skb,
                            offset,
                            sizeof(struct icmphdr))) {
        return PF_PARSE_TRUNCATED_PACKET;
    }

    icmph = pf_packet_data(
        skb,
        offset,
        sizeof(struct icmphdr)
    );

    if (!icmph)
        return PF_PARSE_INVALID_TRANSPORT;

    icmp->type = icmph->type;
    icmp->code = icmph->code;
    icmp->checksum = icmph->checksum;

    /*
     * The last four bytes of the ICMP header are interpreted
     * according to the ICMP message type.
     */
    icmp->rest_of_header = icmph->un.gateway;

    return 0;
}


/* ============================================================
 * ICMP Validation
 * ============================================================ */

bool pf_validate_icmp(const struct pf_icmp_info *icmp)
{
    if (!icmp)
        return false;

    return true;
}


/* ============================================================
 * Protocol Conversion
 * ============================================================ */

enum pf_packet_protocol
pf_protocol_from_ip_header(__u8 protocol)
{
    switch (protocol) {

    case IPPROTO_TCP:
        return PF_PKT_PROTO_TCP;

    case IPPROTO_UDP:
        return PF_PKT_PROTO_UDP;

    case IPPROTO_ICMP:
        return PF_PKT_PROTO_ICMP;

    default:
        return PF_PKT_PROTO_UNKNOWN;
    }
}


/* ============================================================
 * Protocol Name
 * ============================================================ */

const char *pf_packet_protocol_name(
    enum pf_packet_protocol protocol)
{
    switch (protocol) {

    case PF_PKT_PROTO_TCP:
        return "TCP";

    case PF_PKT_PROTO_UDP:
        return "UDP";

    case PF_PKT_PROTO_ICMP:
        return "ICMP";

    case PF_PKT_PROTO_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}


/* ============================================================
 * Protocol Support
 * ============================================================ */

bool pf_packet_protocol_supported(__u8 protocol)
{
    switch (protocol) {

    case IPPROTO_TCP:
    case IPPROTO_UDP:
    case IPPROTO_ICMP:
        return true;

    default:
        return false;
    }
}


/* ============================================================
 * Complete Packet Parser
 * ============================================================ */

int pf_parse_packet(struct sk_buff *skb,
                    struct pf_parsed_packet *packet)
{
    int ret;
    unsigned int transport_offset;
    unsigned int payload_offset;

    if (!skb || !packet)
        return -EINVAL;

    pf_reset_parsed_packet(packet);

    packet->skb = skb;
    packet->dev = skb->dev;
    packet->packet_length = skb->len;

    /*
     * --------------------------------------------------------
     * Ethernet
     * --------------------------------------------------------
     */

    ret = pf_parse_ethernet(
        skb,
        &packet->eth
    );

    if (ret)
        return ret;

    packet->ethernet_valid = true;
    packet->ethernet_header_length = ETH_HLEN;

    /*
     * --------------------------------------------------------
     * IPv4
     * --------------------------------------------------------
     */

    if (!pf_is_ipv4_packet(&packet->eth)) {

        return PF_PARSE_UNSUPPORTED_PROTOCOL;
    }

    ret = pf_parse_ipv4(
        skb,
        packet->ethernet_header_length,
        &packet->ipv4
    );

    if (ret)
        return ret;

    if (!pf_validate_ipv4(&packet->ipv4))
        return PF_PARSE_INVALID_IPV4;

    packet->ipv4_valid = true;

    packet->ip_header_length =
        pf_ipv4_header_length(&packet->ipv4);

    /*
     * --------------------------------------------------------
     * Transport Layer
     * --------------------------------------------------------
     */

    transport_offset =
        packet->ethernet_header_length +
        packet->ip_header_length;

    packet->protocol =
        pf_protocol_from_ip_header(
            packet->ipv4.protocol
        );

    switch (packet->protocol) {

    case PF_PKT_PROTO_TCP:

        ret = pf_parse_tcp(
            skb,
            transport_offset,
            &packet->transport.tcp
        );

        if (ret)
            return ret;

        packet->transport_header_length =
            pf_tcp_header_length(
                &packet->transport.tcp
            );

        packet->transport_valid =
            pf_validate_tcp(
                &packet->transport.tcp
            );

        break;


    case PF_PKT_PROTO_UDP:

        ret = pf_parse_udp(
            skb,
            transport_offset,
            &packet->transport.udp
        );

        if (ret)
            return ret;

        packet->transport_header_length =
            sizeof(struct udphdr);

        packet->transport_valid =
            pf_validate_udp(
                &packet->transport.udp
            );

        break;


    case PF_PKT_PROTO_ICMP:

        ret = pf_parse_icmp(
            skb,
            transport_offset,
            &packet->transport.icmp
        );

        if (ret)
            return ret;

        packet->transport_header_length =
            sizeof(struct icmphdr);

        packet->transport_valid =
            pf_validate_icmp(
                &packet->transport.icmp
            );

        break;


    case PF_PKT_PROTO_UNKNOWN:

    default:

        packet->transport_valid = false;

        return PF_PARSE_UNSUPPORTED_PROTOCOL;
    }

    if (!packet->transport_valid)
        return PF_PARSE_INVALID_TRANSPORT;

    /*
     * --------------------------------------------------------
     * Payload
     * --------------------------------------------------------
     */

    payload_offset =
        transport_offset +
        packet->transport_header_length;

    packet->payload_offset = payload_offset;

    if (payload_offset < skb->len)
        packet->payload_length =
            skb->len - payload_offset;
    else
        packet->payload_length = 0;

    return 0;
}


/* ============================================================
 * Complete Packet Validation
 * ============================================================ */

bool pf_validate_packet(
    const struct pf_parsed_packet *packet)
{
    if (!packet)
        return false;

    if (!packet->ethernet_valid)
        return false;

    if (!packet->ipv4_valid)
        return false;

    if (!packet->transport_valid)
        return false;

    if (packet->protocol ==
        PF_PKT_PROTO_UNKNOWN)
        return false;

    return true;
}


/* ============================================================
 * IPv4 Address Matching
 * ============================================================ */

bool pf_ipv4_address_match(__be32 packet_ip,
                           __be32 rule_ip,
                           __be32 rule_mask)
{
    return (packet_ip & rule_mask) ==
           (rule_ip & rule_mask);
}


/* ============================================================
 * Port Matching
 * ============================================================ */

bool pf_port_match(__be16 packet_port,
                   __be16 start_port,
                   __be16 end_port)
{
    __u16 port;
    __u16 start;
    __u16 end;

    port = ntohs(packet_port);
    start = ntohs(start_port);
    end = ntohs(end_port);

    if (start > end)
        return false;

    return port >= start && port <= end;
}


/* ============================================================
 * Transport Offset
 * ============================================================ */

unsigned int pf_get_transport_offset(
    const struct pf_parsed_packet *packet)
{
    if (!packet)
        return 0;

    return packet->ethernet_header_length +
           packet->ip_header_length;
}


/* ============================================================
 * Payload Offset
 * ============================================================ */

unsigned int pf_get_payload_offset(
    const struct pf_parsed_packet *packet)
{
    if (!packet)
        return 0;

    return packet->payload_offset;
}


/* ============================================================
 * Payload Length
 * ============================================================ */

unsigned int pf_get_payload_length(
    const struct pf_parsed_packet *packet)
{
    if (!packet)
        return 0;

    return packet->payload_length;
}


/* ============================================================
 * Debug Dump - Ethernet
 * ============================================================ */

void pf_dump_ethernet(
    const struct pf_eth_info *eth)
{
    if (!eth)
        return;

    pr_debug(
        PF_LOG_PREFIX
        "ETH: "
        "SRC=%pM "
        "DST=%pM "
        "TYPE=0x%04x\n",
        eth->src_mac,
        eth->dst_mac,
        ntohs(eth->ethertype)
    );
}


/* ============================================================
 * Debug Dump - IPv4
 * ============================================================ */

void pf_dump_ipv4(
    const struct pf_ipv4_info *ipv4)
{
    if (!ipv4)
        return;

    pr_debug(
        PF_LOG_PREFIX
        "IPv4: "
        "SRC=%pI4 "
        "DST=%pI4 "
        "PROTO=%u "
        "TTL=%u "
        "LEN=%u\n",
        &ipv4->src_ip,
        &ipv4->dst_ip,
        ipv4->protocol,
        ipv4->ttl,
        ntohs(ipv4->total_length)
    );
}


/* ============================================================
 * Debug Dump - TCP
 * ============================================================ */

void pf_dump_tcp(
    const struct pf_tcp_info *tcp)
{
    if (!tcp)
        return;

    pr_debug(
        PF_LOG_PREFIX
        "TCP: "
        "SRC_PORT=%u "
        "DST_PORT=%u "
        "FLAGS=0x%x "
        "WINDOW=%u\n",
        ntohs(tcp->src_port),
        ntohs(tcp->dst_port),
        tcp->flags,
        ntohs(tcp->window)
    );
}


/* ============================================================
 * Debug Dump - UDP
 * ============================================================ */

void pf_dump_udp(
    const struct pf_udp_info *udp)
{
    if (!udp)
        return;

    pr_debug(
        PF_LOG_PREFIX
        "UDP: "
        "SRC_PORT=%u "
        "DST_PORT=%u "
        "LEN=%u\n",
        ntohs(udp->src_port),
        ntohs(udp->dst_port),
        ntohs(udp->length)
    );
}


/* ============================================================
 * Debug Dump - ICMP
 * ============================================================ */

void pf_dump_icmp(
    const struct pf_icmp_info *icmp)
{
    if (!icmp)
        return;

    pr_debug(
        PF_LOG_PREFIX
        "ICMP: "
        "TYPE=%u "
        "CODE=%u\n",
        icmp->type,
        icmp->code
    );
}


/* ============================================================
 * Debug Dump - Complete Packet
 * ============================================================ */

void pf_dump_parsed_packet(
    const struct pf_parsed_packet *packet)
{
    if (!packet)
        return;

    pr_debug(
        PF_LOG_PREFIX
        "========== PACKET ==========\n"
    );

    pr_debug(
        PF_LOG_PREFIX
        "length=%u protocol=%s\n",
        packet->packet_length,
        pf_packet_protocol_name(packet->protocol)
    );

    pf_dump_ethernet(&packet->eth);
    pf_dump_ipv4(&packet->ipv4);

    switch (packet->protocol) {

    case PF_PKT_PROTO_TCP:
        pf_dump_tcp(&packet->transport.tcp);
        break;

    case PF_PKT_PROTO_UDP:
        pf_dump_udp(&packet->transport.udp);
        break;

    case PF_PKT_PROTO_ICMP:
        pf_dump_icmp(&packet->transport.icmp);
        break;

    default:
        break;
    }

    pr_debug(
        PF_LOG_PREFIX
        "payload_offset=%u "
        "payload_length=%u\n",
        packet->payload_offset,
        packet->payload_length
    );

    pr_debug(
        PF_LOG_PREFIX
        "============================\n"
    );
}
