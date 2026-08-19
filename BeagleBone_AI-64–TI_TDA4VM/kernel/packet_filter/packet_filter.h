/*
 * Packet Filter Driver - Main Header
 *
 * BeagleBone AI-64 / TI TDA4VM
 *
 * Main interface for the kernel packet-filter driver.
 */

#ifndef PACKET_FILTER_H
#define PACKET_FILTER_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/atomic.h>

/* ============================================================
 * Driver Information
 * ============================================================ */

#define PF_DRIVER_NAME          "packet_filter"
#define PF_DEVICE_NAME         "packet_filter"
#define PF_DRIVER_VERSION       "1.0"
#define PF_DEVICE_PATH          "/dev/packet_filter"

/* ============================================================
 * Driver Limits
 * ============================================================ */

#define PF_MAX_RULES            256
#define PF_MAX_RULE_NAME        64
#define PF_MAX_IP_STRING        16
#define PF_MAX_INTERFACE_NAME   IFNAMSIZ

/* ============================================================
 * Packet Filter Actions
 * ============================================================ */

enum pf_action {
    PF_ACTION_ALLOW = 0,
    PF_ACTION_DROP,
    PF_ACTION_MONITOR,
};

/* ============================================================
 * Rule Types
 * ============================================================ */

enum pf_rule_type {
    PF_RULE_WHITELIST = 0,
    PF_RULE_BLACKLIST,
    PF_RULE_MONITOR,
};

/* ============================================================
 * Protocol Types
 * ============================================================ */

enum pf_protocol {
    PF_PROTOCOL_ANY  = 0,
    PF_PROTOCOL_TCP  = 6,
    PF_PROTOCOL_UDP  = 17,
    PF_PROTOCOL_ICMP = 1,
};

/* ============================================================
 * Rule Structure
 * ============================================================ */

struct pf_rule {
    struct list_head list;

    /*
     * Rule identification.
     */
    __u32 id;
    char name[PF_MAX_RULE_NAME];

    /*
     * Rule classification.
     */
    enum pf_rule_type type;
    enum pf_action action;

    /*
     * Network address information.
     */
    __be32 src_ip;
    __be32 src_mask;

    __be32 dst_ip;
    __be32 dst_mask;

    /*
     * Transport protocol.
     */
    __u8 protocol;

    /*
     * Port range.
     */
    __be16 src_port_start;
    __be16 src_port_end;

    __be16 dst_port_start;
    __be16 dst_port_end;

    /*
     * Interface.
     */
    char interface[PF_MAX_INTERFACE_NAME];

    /*
     * Rule state.
     */
    bool enabled;

    /*
     * Rule statistics.
     */
    atomic64_t packet_count;
    atomic64_t byte_count;
};

/* ============================================================
 * Packet Statistics
 * ============================================================ */

struct pf_statistics {
    atomic64_t packets_received;
    atomic64_t packets_allowed;
    atomic64_t packets_dropped;
    atomic64_t packets_monitored;

    atomic64_t bytes_received;
    atomic64_t bytes_allowed;
    atomic64_t bytes_dropped;
    atomic64_t bytes_monitored;

    atomic64_t rule_matches;
    atomic64_t rule_misses;

    atomic64_t invalid_packets;
    atomic64_t errors;
};

/* ============================================================
 * Driver State
 * ============================================================ */

struct pf_device {
    /*
     * Character device.
     */
    struct cdev *cdev;

    /*
     * Device class/device.
     */
    struct class *class;
    struct device *device;

    /*
     * Device number.
     */
    dev_t dev_number;

    /*
     * Rule database.
     */
    struct list_head rule_list;

    /*
     * Protects rule database.
     */
    struct mutex rule_lock;

    /*
     * Packet statistics.
     */
    struct pf_statistics stats;

    /*
     * Driver state.
     */
    bool enabled;
    bool initialized;

    /*
     * Debug state.
     */
    bool debug_enabled;
};

/* ============================================================
 * Packet Context
 * ============================================================ */

struct pf_packet_info {
    struct sk_buff *skb;

    struct net_device *dev;

    __be32 src_ip;
    __be32 dst_ip;

    __be16 src_port;
    __be16 dst_port;

    __u8 protocol;

    unsigned int packet_length;
};

/* ============================================================
 * Rule Matching Result
 * ============================================================ */

struct pf_match_result {
    bool matched;

    enum pf_action action;

    __u32 rule_id;

    struct pf_rule *rule;
};

/* ============================================================
 * Global Driver Instance
 * ============================================================ */

extern struct pf_device *pf_dev;


/* ============================================================
 * Driver Lifecycle
 * ============================================================ */

/*
 * Initialize packet-filter subsystem.
 */
int pf_init(void);

/*
 * Cleanup packet-filter subsystem.
 */
void pf_exit(void);


/* ============================================================
 * Packet Processing
 * ============================================================ */

/*
 * Process an incoming packet.
 */
int pf_process_packet(struct sk_buff *skb);


/*
 * Extract packet information from an SKB.
 */
int pf_extract_packet_info(struct sk_buff *skb,
                           struct pf_packet_info *info);


/*
 * Apply packet filtering rules.
 */
enum pf_action pf_filter_packet(struct pf_packet_info *packet);


/*
 * Handle allowed packet.
 */
int pf_allow_packet(struct sk_buff *skb);


/*
 * Handle dropped packet.
 */
int pf_drop_packet(struct sk_buff *skb);


/*
 * Handle monitored packet.
 */
int pf_monitor_packet(struct sk_buff *skb);


/* ============================================================
 * Rule Management
 * ============================================================ */

/*
 * Add a packet-filter rule.
 */
int pf_add_rule(struct pf_rule *rule);


/*
 * Remove a packet-filter rule.
 */
int pf_delete_rule(__u32 rule_id);


/*
 * Find a rule by ID.
 */
struct pf_rule *pf_find_rule(__u32 rule_id);


/*
 * Remove all rules.
 */
void pf_clear_rules(void);


/*
 * Enable a rule.
 */
int pf_enable_rule(__u32 rule_id);


/*
 * Disable a rule.
 */
int pf_disable_rule(__u32 rule_id);


/*
 * Check whether a packet matches a rule.
 */
bool pf_rule_matches(const struct pf_rule *rule,
                     const struct pf_packet_info *packet);


/* ============================================================
 * Statistics
 * ============================================================ */

/*
 * Reset packet statistics.
 */
void pf_reset_statistics(void);


/*
 * Update received-packet statistics.
 */
void pf_stats_packet_received(unsigned int bytes);


/*
 * Update allowed-packet statistics.
 */
void pf_stats_packet_allowed(unsigned int bytes);


/*
 * Update dropped-packet statistics.
 */
void pf_stats_packet_dropped(unsigned int bytes);


/*
 * Update monitored-packet statistics.
 */
void pf_stats_packet_monitored(unsigned int bytes);


/* ============================================================
 * Driver Control
 * ============================================================ */

/*
 * Enable packet filtering.
 */
int pf_enable(void);


/*
 * Disable packet filtering.
 */
int pf_disable(void);


/*
 * Check driver state.
 */
bool pf_is_enabled(void);


/* ============================================================
 * Network Device
 * ============================================================ */

/*
 * Register network packet processing.
 */
int pf_register_netdev(void);


/*
 * Unregister network packet processing.
 */
void pf_unregister_netdev(void);


/*
 * Network packet callback.
 */
rx_handler_result_t pf_rx_handler(struct sk_buff **pskb);


/* ============================================================
 * IOCTL Interface
 * ============================================================ */

/*
 * Initialize IOCTL interface.
 */
int pf_ioctl_init(void);


/*
 * Cleanup IOCTL interface.
 */
void pf_ioctl_exit(void);


/* ============================================================
 * Logging Interface
 * ============================================================ */

#include "logging.h"


/* ============================================================
 * Helper Macros
 * ============================================================ */

#define PF_IS_TCP(protocol) \
    ((protocol) == PF_PROTOCOL_TCP)

#define PF_IS_UDP(protocol) \
    ((protocol) == PF_PROTOCOL_UDP)

#define PF_IS_ICMP(protocol) \
    ((protocol) == PF_PROTOCOL_ICMP)

#define PF_IS_ANY_PROTOCOL(protocol) \
    ((protocol) == PF_PROTOCOL_ANY)


/* ============================================================
 * Action Conversion
 * ============================================================ */

static inline const char *pf_action_name(enum pf_action action)
{
    switch (action) {
    case PF_ACTION_ALLOW:
        return "ALLOW";

    case PF_ACTION_DROP:
        return "DROP";

    case PF_ACTION_MONITOR:
        return "MONITOR";

    default:
        return "UNKNOWN";
    }
}


/* ============================================================
 * Rule Type Conversion
 * ============================================================ */

static inline const char *pf_rule_type_name(enum pf_rule_type type)
{
    switch (type) {
    case PF_RULE_WHITELIST:
        return "WHITELIST";

    case PF_RULE_BLACKLIST:
        return "BLACKLIST";

    case PF_RULE_MONITOR:
        return "MONITOR";

    default:
        return "UNKNOWN";
    }
}


/* ============================================================
 * Driver State Helpers
 * ============================================================ */

static inline bool pf_driver_ready(void)
{
    return pf_dev &&
           pf_dev->initialized &&
           pf_dev->enabled;
}


/* ============================================================
 * Statistics Helpers
 * ============================================================ */

static inline void pf_increment_received(unsigned int bytes)
{
    if (!pf_dev)
        return;

    atomic64_inc(&pf_dev->stats.packets_received);
    atomic64_add(bytes, &pf_dev->stats.bytes_received);
}


static inline void pf_increment_allowed(unsigned int bytes)
{
    if (!pf_dev)
        return;

    atomic64_inc(&pf_dev->stats.packets_allowed);
    atomic64_add(bytes, &pf_dev->stats.bytes_allowed);
}


static inline void pf_increment_dropped(unsigned int bytes)
{
    if (!pf_dev)
        return;

    atomic64_inc(&pf_dev->stats.packets_dropped);
    atomic64_add(bytes, &pf_dev->stats.bytes_dropped);
}


static inline void pf_increment_monitored(unsigned int bytes)
{
    if (!pf_dev)
        return;

    atomic64_inc(&pf_dev->stats.packets_monitored);
    atomic64_add(bytes, &pf_dev->stats.bytes_monitored);
}


#endif /* PACKET_FILTER_H */
