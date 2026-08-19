/*
 * Packet Filter Driver - Statistics Implementation
 *
 * BeagleBone AI-64 / TI TDA4VM
 *
 * Responsibilities:
 *   - Global packet counters
 *   - RX/TX statistics
 *   - Allow/Drop/Monitor statistics
 *   - Protocol statistics
 *   - Parser/error statistics
 *   - Rule statistics
 *   - Statistics snapshots
 *   - Statistics reset
 *   - Kernel debug output
 */

#include "statistics.h"
#include "logging.h"

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

/* ============================================================
 * Module Information
 * ============================================================ */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BeagleBone AI-64 Packet Filter Project");
MODULE_DESCRIPTION(
    "Statistics subsystem for BeagleBone AI-64 / TI TDA4VM packet filter"
);
MODULE_VERSION("1.0");


/* ============================================================
 * Global Statistics Object
 * ============================================================ */

struct pf_statistics pf_engine_stats;

EXPORT_SYMBOL_GPL(pf_engine_stats);


/* ============================================================
 * Internal State
 * ============================================================ */

static bool pf_statistics_initialized;


/* ============================================================
 * Statistics Initialization
 * ============================================================ */

int pf_statistics_init(void)
{
    memset(&pf_engine_stats, 0, sizeof(pf_engine_stats));

    spin_lock_init(&pf_engine_stats.lock);

    pf_engine_stats.enabled = true;

    atomic64_set(
        &pf_engine_stats.reset_count,
        0
    );

    pf_statistics_initialized = true;

    pf_log_info(
        "statistics subsystem initialized\n"
    );

    return 0;
}


/* ============================================================
 * Statistics Cleanup
 * ============================================================ */

void pf_statistics_exit(void)
{
    if (!pf_statistics_initialized)
        return;

    pf_statistics_reset();

    pf_engine_stats.enabled = false;

    pf_statistics_initialized = false;

    pf_log_info(
        "statistics subsystem removed\n"
    );
}


/* ============================================================
 * Enable / Disable
 * ============================================================ */

int pf_statistics_enable(void)
{
    unsigned long flags;

    if (!pf_statistics_initialized)
        return -ENODEV;

    spin_lock_irqsave(
        &pf_engine_stats.lock,
        flags
    );

    pf_engine_stats.enabled = true;

    spin_unlock_irqrestore(
        &pf_engine_stats.lock,
        flags
    );

    pf_log_info(
        "statistics collection enabled\n"
    );

    return 0;
}


int pf_statistics_disable(void)
{
    unsigned long flags;

    if (!pf_statistics_initialized)
        return -ENODEV;

    spin_lock_irqsave(
        &pf_engine_stats.lock,
        flags
    );

    pf_engine_stats.enabled = false;

    spin_unlock_irqrestore(
        &pf_engine_stats.lock,
        flags
    );

    pf_log_info(
        "statistics collection disabled\n"
    );

    return 0;
}


bool pf_statistics_is_enabled(void)
{
    bool enabled;
    unsigned long flags;

    if (!pf_statistics_initialized)
        return false;

    spin_lock_irqsave(
        &pf_engine_stats.lock,
        flags
    );

    enabled = pf_engine_stats.enabled;

    spin_unlock_irqrestore(
        &pf_engine_stats.lock,
        flags
    );

    return enabled;
}


/* ============================================================
 * RX Statistics
 * ============================================================ */

void pf_stats_record_rx(
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


/* ============================================================
 * TX Statistics
 * ============================================================ */

void pf_stats_record_tx(
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
 * Allow Statistics
 * ============================================================ */

void pf_stats_record_allow(
    enum pf_stats_direction direction,
    unsigned int packet_length)
{
    if (!pf_statistics_is_enabled())
        return;

    switch (direction) {

    case PF_STATS_RX:

        atomic64_inc(
            &pf_engine_stats.global.rx_allowed
        );

        break;

    case PF_STATS_TX:

        atomic64_inc(
            &pf_engine_stats.global.tx_allowed
        );

        break;

    default:
        return;
    }
}


/* ============================================================
 * Drop Statistics
 * ============================================================ */

void pf_stats_record_drop(
    enum pf_stats_direction direction,
    unsigned int packet_length)
{
    if (!pf_statistics_is_enabled())
        return;

    switch (direction) {

    case PF_STATS_RX:

        atomic64_inc(
            &pf_engine_stats.global.rx_dropped
        );

        break;

    case PF_STATS_TX:

        atomic64_inc(
            &pf_engine_stats.global.tx_dropped
        );

        break;

    default:
        return;
    }
}


/* ============================================================
 * Monitor Statistics
 * ============================================================ */

void pf_stats_record_monitor(
    enum pf_stats_direction direction,
    unsigned int packet_length)
{
    if (!pf_statistics_is_enabled())
        return;

    switch (direction) {

    case PF_STATS_RX:

        atomic64_inc(
            &pf_engine_stats.global.rx_monitored
        );

        break;

    case PF_STATS_TX:

        atomic64_inc(
            &pf_engine_stats.global.tx_monitored
        );

        break;

    default:
        return;
    }
}


/* ============================================================
 * Protocol Statistics
 * ============================================================ */

void pf_stats_record_tcp(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.tcp_packets
    );
}


void pf_stats_record_udp(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.udp_packets
    );
}


void pf_stats_record_icmp(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.icmp_packets
    );
}


void pf_stats_record_other_protocol(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.other_packets
    );
}


/* ============================================================
 * Parser Statistics
 * ============================================================ */

void pf_stats_record_parsed(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.parsed_packets
    );
}


void pf_stats_record_parse_error(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.parse_errors
    );
}


void pf_stats_record_malformed(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.malformed_packets
    );
}


/* ============================================================
 * Rule Statistics
 * ============================================================ */

void pf_stats_record_whitelist_match(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.whitelist_matches
    );
}


void pf_stats_record_blacklist_match(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.blacklist_matches
    );
}


void pf_stats_record_monitor_match(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.monitor_matches
    );
}


void pf_stats_record_rule_miss(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.rule_misses
    );
}


/* ============================================================
 * Error Statistics
 * ============================================================ */

void pf_stats_record_memory_error(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.memory_errors
    );
}


void pf_stats_record_invalid_rule(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.invalid_rule_errors
    );
}


void pf_stats_record_ioctl_error(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.ioctl_errors
    );
}


void pf_stats_record_internal_error(void)
{
    if (!pf_statistics_is_enabled())
        return;

    atomic64_inc(
        &pf_engine_stats.global.internal_errors
    );
}


/* ============================================================
 * Statistics Snapshot
 * ============================================================ */

int pf_statistics_get_snapshot(
    struct pf_stats_snapshot *snapshot)
{
    unsigned long flags;

    if (!snapshot)
        return -EINVAL;

    if (!pf_statistics_initialized)
        return -ENODEV;

    memset(
        snapshot,
        0,
        sizeof(*snapshot)
    );

    spin_lock_irqsave(
        &pf_engine_stats.lock,
        flags
    );

    snapshot->rx_packets =
        atomic64_read(
            &pf_engine_stats.global.rx_packets
        );

    snapshot->rx_bytes =
        atomic64_read(
            &pf_engine_stats.global.rx_bytes
        );

    snapshot->rx_allowed =
        atomic64_read(
            &pf_engine_stats.global.rx_allowed
        );

    snapshot->rx_dropped =
        atomic64_read(
            &pf_engine_stats.global.rx_dropped
        );

    snapshot->rx_monitored =
        atomic64_read(
            &pf_engine_stats.global.rx_monitored
        );

    snapshot->tx_packets =
        atomic64_read(
            &pf_engine_stats.global.tx_packets
        );

    snapshot->tx_bytes =
        atomic64_read(
            &pf_engine_stats.global.tx_bytes
        );

    snapshot->tx_allowed =
        atomic64_read(
            &pf_engine_stats.global.tx_allowed
        );

    snapshot->tx_dropped =
        atomic64_read(
            &pf_engine_stats.global.tx_dropped
        );

    snapshot->tx_monitored =
        atomic64_read(
            &pf_engine_stats.global.tx_monitored
        );

    snapshot->tcp_packets =
        atomic64_read(
            &pf_engine_stats.global.tcp_packets
        );

    snapshot->udp_packets =
        atomic64_read(
            &pf_engine_stats.global.udp_packets
        );

    snapshot->icmp_packets =
        atomic64_read(
            &pf_engine_stats.global.icmp_packets
        );

    snapshot->other_packets =
        atomic64_read(
            &pf_engine_stats.global.other_packets
        );

    snapshot->parsed_packets =
        atomic64_read(
            &pf_engine_stats.global.parsed_packets
        );

    snapshot->parse_errors =
        atomic64_read(
            &pf_engine_stats.global.parse_errors
        );

    snapshot->malformed_packets =
        atomic64_read(
            &pf_engine_stats.global.malformed_packets
        );

    snapshot->whitelist_matches =
        atomic64_read(
            &pf_engine_stats.global.whitelist_matches
        );

    snapshot->blacklist_matches =
        atomic64_read(
            &pf_engine_stats.global.blacklist_matches
        );

    snapshot->monitor_matches =
        atomic64_read(
            &pf_engine_stats.global.monitor_matches
        );

    snapshot->rule_misses =
        atomic64_read(
            &pf_engine_stats.global.rule_misses
        );

    snapshot->memory_errors =
        atomic64_read(
            &pf_engine_stats.global.memory_errors
        );

    snapshot->invalid_rule_errors =
        atomic64_read(
            &pf_engine_stats.global.invalid_rule_errors
        );

    snapshot->ioctl_errors =
        atomic64_read(
            &pf_engine_stats.global.ioctl_errors
        );

    snapshot->internal_errors =
        atomic64_read(
            &pf_engine_stats.global.internal_errors
        );

    snapshot->total_packets =
        atomic64_read(
            &pf_engine_stats.global.total_packets
        );

    snapshot->total_bytes =
        atomic64_read(
            &pf_engine_stats.global.total_bytes
        );

    spin_unlock_irqrestore(
        &pf_engine_stats.lock,
        flags
    );

    return 0;
}


/* ============================================================
 * Reset Packet Statistics
 * ============================================================ */

static void pf_statistics_reset_packet_counters(void)
{
    atomic64_set(
        &pf_engine_stats.global.rx_packets,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.rx_bytes,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.tx_packets,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.tx_bytes,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.total_packets,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.total_bytes,
        0
    );
}


/* ============================================================
 * Reset Decision Statistics
 * ============================================================ */

void pf_statistics_reset_decisions(void)
{
    if (!pf_statistics_initialized)
        return;

    atomic64_set(
        &pf_engine_stats.global.rx_allowed,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.rx_dropped,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.rx_monitored,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.tx_allowed,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.tx_dropped,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.tx_monitored,
        0
    );
}


/* ============================================================
 * Reset Protocol Statistics
 * ============================================================ */

void pf_statistics_reset_protocols(void)
{
    if (!pf_statistics_initialized)
        return;

    atomic64_set(
        &pf_engine_stats.global.tcp_packets,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.udp_packets,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.icmp_packets,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.other_packets,
        0
    );
}


/* ============================================================
 * Reset Parser Statistics
 * ============================================================ */

void pf_statistics_reset_parser(void)
{
    if (!pf_statistics_initialized)
        return;

    atomic64_set(
        &pf_engine_stats.global.parsed_packets,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.parse_errors,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.malformed_packets,
        0
    );
}


/* ============================================================
 * Reset Rule Statistics
 * ============================================================ */

static void pf_statistics_reset_rule_counters(void)
{
    atomic64_set(
        &pf_engine_stats.global.whitelist_matches,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.blacklist_matches,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.monitor_matches,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.rule_misses,
        0
    );
}


/* ============================================================
 * Reset Error Statistics
 * ============================================================ */

void pf_statistics_reset_errors(void)
{
    if (!pf_statistics_initialized)
        return;

    atomic64_set(
        &pf_engine_stats.global.memory_errors,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.invalid_rule_errors,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.ioctl_errors,
        0
    );

    atomic64_set(
        &pf_engine_stats.global.internal_errors,
        0
    );
}


/* ============================================================
 * Reset All Statistics
 * ============================================================ */

void pf_statistics_reset(void)
{
    unsigned long flags;

    if (!pf_statistics_initialized)
        return;

    spin_lock_irqsave(
        &pf_engine_stats.lock,
        flags
    );

    pf_statistics_reset_packet_counters();

    pf_statistics_reset_decisions();

    pf_statistics_reset_protocols();

    pf_statistics_reset_parser();

    pf_statistics_reset_rule_counters();

    pf_statistics_reset_errors();

    atomic64_inc(
        &pf_engine_stats.reset_count
    );

    spin_unlock_irqrestore(
        &pf_engine_stats.lock,
        flags
    );

    /*
     * Reset per-rule counters too.
     */
    pf_rule_engine_reset_statistics();

    pf_log_info(
        "all statistics reset\n"
    );
}


/* ============================================================
 * Rule Action Integration
 * ============================================================ */

void pf_statistics_record_rule_action(
    const struct pf_engine_rule *rule,
    enum pf_action action,
    unsigned int packet_length)
{
    if (!pf_statistics_is_enabled())
        return;

    if (!rule)
        return;

    switch (rule->type) {

    case PF_RULE_WHITELIST:

        pf_stats_record_whitelist_match();

        break;

    case PF_RULE_BLACKLIST:

        pf_stats_record_blacklist_match();

        break;

    case PF_RULE_MONITOR:

        pf_stats_record_monitor_match();

        break;

    default:

        pf_stats_record_rule_miss();

        break;
    }

    switch (action) {

    case PF_ACTION_ALLOW:

        pf_stats_record_allow(
            PF_STATS_RX,
            packet_length
        );

        break;

    case PF_ACTION_DROP:

        pf_stats_record_drop(
            PF_STATS_RX,
            packet_length
        );

        break;

    case PF_ACTION_MONITOR:

        pf_stats_record_monitor(
            PF_STATS_RX,
            packet_length
        );

        break;

    default:
        break;
    }
}


/* ============================================================
 * Get Rule Statistics
 * ============================================================ */

int pf_statistics_get_rule_stats(
    __u32 rule_id,
    struct pf_rule_stats *stats)
{
    struct pf_engine_rule *rule;

    if (!stats)
        return -EINVAL;

    if (!pf_statistics_initialized)
        return -ENODEV;

    memset(
        stats,
        0,
        sizeof(*stats)
    );

    mutex_lock(&pf_engine.lock);

    rule = pf_rule_engine_find_rule(rule_id);

    if (!rule) {

        mutex_unlock(&pf_engine.lock);

        return -ENOENT;
    }

    stats->packets_matched =
        rule->stats.packets_matched;

    stats->bytes_matched =
        rule->stats.bytes_matched;

    stats->packets_allowed =
        rule->stats.packets_allowed;

    stats->packets_dropped =
        rule->stats.packets_dropped;

    stats->packets_monitored =
        rule->stats.packets_monitored;

    mutex_unlock(&pf_engine.lock);

    return 0;
}


/* ============================================================
 * Dump Packet Statistics
 * ============================================================ */

void pf_statistics_dump_packets(void)
{
    pr_info(
        PF_LOG_PREFIX
        "========== PACKET STATISTICS ==========\n"
    );

    pr_info(
        PF_LOG_PREFIX
        "RX packets       : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.rx_packets
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "RX bytes         : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.rx_bytes
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "TX packets       : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.tx_packets
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "TX bytes         : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.tx_bytes
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Total packets    : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.total_packets
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Total bytes      : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.total_bytes
        )
    );
}


/* ============================================================
 * Dump Protocol Statistics
 * ============================================================ */

void pf_statistics_dump_protocols(void)
{
    pr_info(
        PF_LOG_PREFIX
        "========== PROTOCOL STATISTICS ==========\n"
    );

    pr_info(
        PF_LOG_PREFIX
        "TCP packets      : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.tcp_packets
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "UDP packets      : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.udp_packets
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "ICMP packets     : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.icmp_packets
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Other packets    : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.other_packets
        )
    );
}


/* ============================================================
 * Dump Rule Statistics
 * ============================================================ */

void pf_statistics_dump_rules(void)
{
    pr_info(
        PF_LOG_PREFIX
        "========== RULE STATISTICS ==========\n"
    );

    pr_info(
        PF_LOG_PREFIX
        "Whitelist matches : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.whitelist_matches
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Blacklist matches : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.blacklist_matches
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Monitor matches   : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.monitor_matches
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Rule misses       : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.rule_misses
        )
    );

    /*
     * Dump individual rule counters.
     */
    pf_rule_engine_dump_rules();
}


/* ============================================================
 * Dump Error Statistics
 * ============================================================ */

void pf_statistics_dump_errors(void)
{
    pr_info(
        PF_LOG_PREFIX
        "========== ERROR STATISTICS ==========\n"
    );

    pr_info(
        PF_LOG_PREFIX
        "Parse errors       : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.parse_errors
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Malformed packets  : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.malformed_packets
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Memory errors      : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.memory_errors
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Invalid rules      : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.invalid_rule_errors
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "IOCTL errors       : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.ioctl_errors
        )
    );

    pr_info(
        PF_LOG_PREFIX
        "Internal errors    : %lld\n",
        atomic64_read(
            &pf_engine_stats.global.internal_errors
        )
    );
}


/* ============================================================
 * Dump Complete Statistics
 * ============================================================ */

void pf_statistics_dump(void)
{
    if (!pf_statistics_initialized)
        return;

    pr_info(
        PF_LOG_PREFIX
        "\n"
        "============================================\n"
        "       PACKET FILTER STATISTICS\n"
        "============================================\n"
    );

    pf_statistics_dump_packets();

    pf_statistics_dump_protocols();

    pf_statistics_dump_rules();

    pf_statistics_dump_errors();

    pr_info(
        PF_LOG_PREFIX
        "============================================\n"
    );
}


/* ============================================================
 * Module Initialization / Cleanup
 * ============================================================ */

/*
 * The statistics subsystem is normally initialized by
 * packet_filter.c rather than independently as a module.
 *
 * These symbols are exported so the main packet filter
 * driver can use them.
 */

EXPORT_SYMBOL_GPL(pf_statistics_init);
EXPORT_SYMBOL_GPL(pf_statistics_exit);

EXPORT_SYMBOL_GPL(pf_statistics_enable);
EXPORT_SYMBOL_GPL(pf_statistics_disable);
EXPORT_SYMBOL_GPL(pf_statistics_is_enabled);

EXPORT_SYMBOL_GPL(pf_stats_record_rx);
EXPORT_SYMBOL_GPL(pf_stats_record_tx);

EXPORT_SYMBOL_GPL(pf_stats_record_allow);
EXPORT_SYMBOL_GPL(pf_stats_record_drop);
EXPORT_SYMBOL_GPL(pf_stats_record_monitor);

EXPORT_SYMBOL_GPL(pf_stats_record_tcp);
EXPORT_SYMBOL_GPL(pf_stats_record_udp);
EXPORT_SYMBOL_GPL(pf_stats_record_icmp);
EXPORT_SYMBOL_GPL(pf_stats_record_other_protocol);

EXPORT_SYMBOL_GPL(pf_stats_record_parsed);
EXPORT_SYMBOL_GPL(pf_stats_record_parse_error);
EXPORT_SYMBOL_GPL(pf_stats_record_malformed);

EXPORT_SYMBOL_GPL(pf_stats_record_whitelist_match);
EXPORT_SYMBOL_GPL(pf_stats_record_blacklist_match);
EXPORT_SYMBOL_GPL(pf_stats_record_monitor_match);
EXPORT_SYMBOL_GPL(pf_stats_record_rule_miss);

EXPORT_SYMBOL_GPL(pf_stats_record_memory_error);
EXPORT_SYMBOL_GPL(pf_stats_record_invalid_rule);
EXPORT_SYMBOL_GPL(pf_stats_record_ioctl_error);
EXPORT_SYMBOL_GPL(pf_stats_record_internal_error);

EXPORT_SYMBOL_GPL(pf_statistics_get_snapshot);
EXPORT_SYMBOL_GPL(pf_statistics_reset);

EXPORT_SYMBOL_GPL(pf_statistics_reset_protocols);
EXPORT_SYMBOL_GPL(pf_statistics_reset_decisions);
EXPORT_SYMBOL_GPL(pf_statistics_reset_parser);
EXPORT_SYMBOL_GPL(pf_statistics_reset_errors);

EXPORT_SYMBOL_GPL(pf_statistics_dump);
EXPORT_SYMBOL_GPL(pf_statistics_dump_packets);
EXPORT_SYMBOL_GPL(pf_statistics_dump_protocols);
EXPORT_SYMBOL_GPL(pf_statistics_dump_rules);
EXPORT_SYMBOL_GPL(pf_statistics_dump_errors);

EXPORT_SYMBOL_GPL(pf_statistics_record_rule_action);
EXPORT_SYMBOL_GPL(pf_statistics_get_rule_stats);
