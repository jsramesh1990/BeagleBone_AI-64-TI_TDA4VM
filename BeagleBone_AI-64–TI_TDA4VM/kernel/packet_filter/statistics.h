/*
 * Packet Filter Driver - Statistics Header
 *
 * BeagleBone AI-64 / TI TDA4VM
 *
 * Provides:
 *   - Global packet statistics
 *   - RX/TX packet counters
 *   - Allow/drop/monitor counters
 *   - Byte counters
 *   - Error counters
 *   - Rule statistics support
 *   - Statistics reset/read APIs
 */

#ifndef PACKET_FILTER_STATISTICS_H
#define PACKET_FILTER_STATISTICS_H

#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>

#include "packet_filter.h"
#include "rule_engine.h"


/* ============================================================
 * Statistics Version
 * ============================================================ */

#define PF_STATS_VERSION_MAJOR        1
#define PF_STATS_VERSION_MINOR        0


/* ============================================================
 * Packet Direction
 * ============================================================ */

enum pf_stats_direction {
    PF_STATS_RX = 0,
    PF_STATS_TX,
};


/* ============================================================
 * Packet Decision
 * ============================================================ */

enum pf_stats_action {
    PF_STATS_ACTION_ALLOW = 0,
    PF_STATS_ACTION_DROP,
    PF_STATS_ACTION_MONITOR,
};


/* ============================================================
 * Error Types
 * ============================================================ */

enum pf_stats_error {
    PF_STATS_ERROR_PARSE = 0,
    PF_STATS_ERROR_MALFORMED,
    PF_STATS_ERROR_NO_MEMORY,
    PF_STATS_ERROR_INVALID_RULE,
    PF_STATS_ERROR_IOCTL,
    PF_STATS_ERROR_INTERNAL,
};


/* ============================================================
 * Global Packet Statistics
 * ============================================================ */

struct pf_global_stats {

    /* --------------------------------------------------------
     * RX statistics
     * -------------------------------------------------------- */

    atomic64_t rx_packets;
    atomic64_t rx_bytes;

    atomic64_t rx_allowed;
    atomic64_t rx_dropped;
    atomic64_t rx_monitored;

    /* --------------------------------------------------------
     * TX statistics
     * -------------------------------------------------------- */

    atomic64_t tx_packets;
    atomic64_t tx_bytes;

    atomic64_t tx_allowed;
    atomic64_t tx_dropped;
    atomic64_t tx_monitored;

    /* --------------------------------------------------------
     * Protocol statistics
     * -------------------------------------------------------- */

    atomic64_t tcp_packets;
    atomic64_t udp_packets;
    atomic64_t icmp_packets;
    atomic64_t other_packets;

    /* --------------------------------------------------------
     * Packet parsing statistics
     * -------------------------------------------------------- */

    atomic64_t parsed_packets;
    atomic64_t parse_errors;
    atomic64_t malformed_packets;

    /* --------------------------------------------------------
     * Rule statistics
     * -------------------------------------------------------- */

    atomic64_t whitelist_matches;
    atomic64_t blacklist_matches;
    atomic64_t monitor_matches;
    atomic64_t rule_misses;

    /* --------------------------------------------------------
     * Error statistics
     * -------------------------------------------------------- */

    atomic64_t memory_errors;
    atomic64_t invalid_rule_errors;
    atomic64_t ioctl_errors;
    atomic64_t internal_errors;

    /* --------------------------------------------------------
     * Runtime statistics
     * -------------------------------------------------------- */

    atomic64_t total_packets;
    atomic64_t total_bytes;
};


/* ============================================================
 * Per-CPU Statistics
 * ============================================================ */

struct pf_cpu_stats {

    atomic64_t packets;
    atomic64_t bytes;

    atomic64_t allowed;
    atomic64_t dropped;
    atomic64_t monitored;

    atomic64_t parse_errors;
    atomic64_t malformed_packets;
};


/* ============================================================
 * Statistics Snapshot
 *
 * Used when exporting statistics to userspace.
 * Unlike atomic counters, this structure contains normal values.
 * ============================================================ */

struct pf_stats_snapshot {

    __u64 rx_packets;
    __u64 rx_bytes;

    __u64 rx_allowed;
    __u64 rx_dropped;
    __u64 rx_monitored;

    __u64 tx_packets;
    __u64 tx_bytes;

    __u64 tx_allowed;
    __u64 tx_dropped;
    __u64 tx_monitored;

    __u64 tcp_packets;
    __u64 udp_packets;
    __u64 icmp_packets;
    __u64 other_packets;

    __u64 parsed_packets;
    __u64 parse_errors;
    __u64 malformed_packets;

    __u64 whitelist_matches;
    __u64 blacklist_matches;
    __u64 monitor_matches;
    __u64 rule_misses;

    __u64 memory_errors;
    __u64 invalid_rule_errors;
    __u64 ioctl_errors;
    __u64 internal_errors;

    __u64 total_packets;
    __u64 total_bytes;
};


/* ============================================================
 * Statistics Context
 * ============================================================ */

struct pf_statistics {

    /*
     * Global atomic counters.
     */
    struct pf_global_stats global;

    /*
     * Protects snapshot/reset operations.
     */
    spinlock_t lock;

    /*
     * Statistics subsystem state.
     */
    bool enabled;

    /*
     * Number of statistics resets.
     */
    atomic64_t reset_count;
};


/* ============================================================
 * Statistics Lifecycle
 * ============================================================ */

/*
 * Initialize statistics subsystem.
 */
int pf_statistics_init(void);


/*
 * Cleanup statistics subsystem.
 */
void pf_statistics_exit(void);


/*
 * Enable statistics collection.
 */
int pf_statistics_enable(void);


/*
 * Disable statistics collection.
 */
int pf_statistics_disable(void);


/*
 * Check statistics state.
 */
bool pf_statistics_is_enabled(void);


/* ============================================================
 * Global Packet Counters
 * ============================================================ */

/*
 * Record received packet.
 */
void pf_stats_record_rx(
    unsigned int packet_length);


/*
 * Record transmitted packet.
 */
void pf_stats_record_tx(
    unsigned int packet_length);


/*
 * Record allowed packet.
 */
void pf_stats_record_allow(
    enum pf_stats_direction direction,
    unsigned int packet_length);


/*
 * Record dropped packet.
 */
void pf_stats_record_drop(
    enum pf_stats_direction direction,
    unsigned int packet_length);


/*
 * Record monitored packet.
 */
void pf_stats_record_monitor(
    enum pf_stats_direction direction,
    unsigned int packet_length);


/* ============================================================
 * Protocol Counters
 * ============================================================ */

/*
 * Record TCP packet.
 */
void pf_stats_record_tcp(void);


/*
 * Record UDP packet.
 */
void pf_stats_record_udp(void);


/*
 * Record ICMP packet.
 */
void pf_stats_record_icmp(void);


/*
 * Record unsupported/other protocol.
 */
void pf_stats_record_other_protocol(void);


/* ============================================================
 * Parser Statistics
 * ============================================================ */

/*
 * Record successfully parsed packet.
 */
void pf_stats_record_parsed(void);


/*
 * Record parser error.
 */
void pf_stats_record_parse_error(void);


/*
 * Record malformed packet.
 */
void pf_stats_record_malformed(void);


/* ============================================================
 * Rule Statistics
 * ============================================================ */

/*
 * Record whitelist match.
 */
void pf_stats_record_whitelist_match(void);


/*
 * Record blacklist match.
 */
void pf_stats_record_blacklist_match(void);


/*
 * Record monitor rule match.
 */
void pf_stats_record_monitor_match(void);


/*
 * Record packet for which no rule matched.
 */
void pf_stats_record_rule_miss(void);


/* ============================================================
 * Error Statistics
 * ============================================================ */

/*
 * Record memory allocation error.
 */
void pf_stats_record_memory_error(void);


/*
 * Record invalid rule error.
 */
void pf_stats_record_invalid_rule(void);


/*
 * Record ioctl error.
 */
void pf_stats_record_ioctl_error(void);


/*
 * Record internal driver error.
 */
void pf_stats_record_internal_error(void);


/* ============================================================
 * Statistics Snapshot
 * ============================================================ */

/*
 * Copy current global statistics into snapshot.
 */
int pf_statistics_get_snapshot(
    struct pf_stats_snapshot *snapshot);


/*
 * Reset all global statistics.
 */
void pf_statistics_reset(void);


/*
 * Reset protocol statistics.
 */
void pf_statistics_reset_protocols(void);


/*
 * Reset packet decision statistics.
 */
void pf_statistics_reset_decisions(void);


/*
 * Reset parser statistics.
 */
void pf_statistics_reset_parser(void);


/*
 * Reset error statistics.
 */
void pf_statistics_reset_errors(void);


/* ============================================================
 * Statistics Display
 * ============================================================ */

/*
 * Print all statistics to kernel log.
 */
void pf_statistics_dump(void);


/*
 * Print packet statistics.
 */
void pf_statistics_dump_packets(void);


/*
 * Print protocol statistics.
 */
void pf_statistics_dump_protocols(void);


/*
 * Print rule statistics.
 */
void pf_statistics_dump_rules(void);


/*
 * Print error statistics.
 */
void pf_statistics_dump_errors(void);


/* ============================================================
 * Rule Statistics Integration
 * ============================================================ */

/*
 * Update global statistics based on a rule match.
 */
void pf_statistics_record_rule_action(
    const struct pf_engine_rule *rule,
    enum pf_action action,
    unsigned int packet_length);


/*
 * Export statistics for a specific rule.
 */
int pf_statistics_get_rule_stats(
    __u32 rule_id,
    struct pf_rule_stats *stats);


/* ============================================================
 * Counter Access Helpers
 * ============================================================ */

/*
 * Get RX packet count.
 */
static inline __u64 pf_stats_rx_packets(void)
{
    return atomic64_read(
        &pf_engine_stats.global.rx_packets
    );
}


/*
 * Get RX byte count.
 */
static inline __u64 pf_stats_rx_bytes(void)
{
    return atomic64_read(
        &pf_engine_stats.global.rx_bytes
    );
}


/*
 * Get TX packet count.
 */
static inline __u64 pf_stats_tx_packets(void)
{
    return atomic64_read(
        &pf_engine_stats.global.tx_packets
    );
}


/*
 * Get TX byte count.
 */
static inline __u64 pf_stats_tx_bytes(void)
{
    return atomic64_read(
        &pf_engine_stats.global.tx_bytes
    );
}


/*
 * Get allowed packet count.
 */
static inline __u64 pf_stats_allowed(void)
{
    return atomic64_read(
        &pf_engine_stats.global.rx_allowed
    ) +
    atomic64_read(
        &pf_engine_stats.global.tx_allowed
    );
}


/*
 * Get dropped packet count.
 */
static inline __u64 pf_stats_dropped(void)
{
    return atomic64_read(
        &pf_engine_stats.global.rx_dropped
    ) +
    atomic64_read(
        &pf_engine_stats.global.tx_dropped
    );
}


/*
 * Get monitored packet count.
 */
static inline __u64 pf_stats_monitored(void)
{
    return atomic64_read(
        &pf_engine_stats.global.rx_monitored
    ) +
    atomic64_read(
        &pf_engine_stats.global.tx_monitored
    );
}


/*
 * Get total packets.
 */
static inline __u64 pf_stats_total_packets(void)
{
    return atomic64_read(
        &pf_engine_stats.global.total_packets
    );
}


/*
 * Get total bytes.
 */
static inline __u64 pf_stats_total_bytes(void)
{
    return atomic64_read(
        &pf_engine_stats.global.total_bytes
    );
}


/* ============================================================
 * Fast Counter Helpers
 *
 * These helpers are intended for use from the packet path.
 * ============================================================ */

static inline void pf_stats_fast_rx(
    unsigned int packet_length)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.rx_packets
    );

    atomic64_add(
        packet_length,
        &pf_engine_stats.global.rx_bytes
    );

    atomic64_inc(
        &pf_engine_stats.global.total_packets
    );

    atomic64_add(
        packet_length,
        &pf_engine_stats.global.total_bytes
    );
}


static inline void pf_stats_fast_tx(
    unsigned int packet_length)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.tx_packets
    );

    atomic64_add(
        packet_length,
        &pf_engine_stats.global.tx_bytes
    );

    atomic64_inc(
        &pf_engine_stats.global.total_packets
    );

    atomic64_add(
        packet_length,
        &pf_engine_stats.global.total_bytes
    );
}


/* ============================================================
 * Statistics Validation Helpers
 * ============================================================ */

/*
 * Check whether snapshot pointer is valid.
 */
static inline bool pf_stats_snapshot_valid(
    const struct pf_stats_snapshot *snapshot)
{
    return snapshot != NULL;
}


/*
 * Check whether statistics subsystem is usable.
 */
static inline bool pf_stats_available(void)
{
    return pf_statistics_is_enabled();
}


#endif /* PACKET_FILTER_STATISTICS_H */
