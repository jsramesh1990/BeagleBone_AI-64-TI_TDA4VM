/*
 * Packet Filter Driver - Rule Engine Header
 *
 * BeagleBone AI-64 / TI TDA4VM
 *
 * Responsible for:
 *   - Rule representation
 *   - Rule matching
 *   - Whitelist processing
 *   - Blacklist processing
 *   - Monitoring rules
 *   - Rule priority
 *   - Rule statistics
 */

#ifndef PACKET_FILTER_RULE_ENGINE_H
#define PACKET_FILTER_RULE_ENGINE_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/skbuff.h>

#include "packet_filter.h"
#include "packet_parser.h"


/* ============================================================
 * Rule Engine Configuration
 * ============================================================ */

#define PF_RULE_ENGINE_MAX_RULES       256
#define PF_RULE_ENGINE_MAX_NAME        64
#define PF_RULE_ENGINE_MAX_INTERFACE   16

#define PF_RULE_PRIORITY_MIN           0
#define PF_RULE_PRIORITY_MAX           65535

/*
 * Priority rule:
 *
 * Lower numerical value = higher priority.
 *
 * Example:
 *
 * Rule 10 -> priority 10
 * Rule 20 -> priority 20
 *
 * Rule 10 is evaluated first.
 */


/* ============================================================
 * Rule Match Flags
 * ============================================================ */

#define PF_MATCH_SRC_IP        BIT(0)
#define PF_MATCH_DST_IP        BIT(1)
#define PF_MATCH_SRC_PORT      BIT(2)
#define PF_MATCH_DST_PORT      BIT(3)
#define PF_MATCH_PROTOCOL      BIT(4)
#define PF_MATCH_INTERFACE     BIT(5)
#define PF_MATCH_MAC_SRC       BIT(6)
#define PF_MATCH_MAC_DST       BIT(7)


/* ============================================================
 * Rule Direction
 * ============================================================ */

enum pf_rule_direction {
    PF_RULE_DIR_ANY = 0,
    PF_RULE_DIR_INGRESS,
    PF_RULE_DIR_EGRESS,
};


/* ============================================================
 * Rule State
 * ============================================================ */

enum pf_rule_state {
    PF_RULE_STATE_DISABLED = 0,
    PF_RULE_STATE_ENABLED,
};


/* ============================================================
 * Rule Evaluation Result
 * ============================================================ */

enum pf_rule_result {
    PF_RULE_NO_MATCH = 0,
    PF_RULE_MATCH_ALLOW,
    PF_RULE_MATCH_DROP,
    PF_RULE_MATCH_MONITOR,
};


/* ============================================================
 * Rule Statistics
 * ============================================================ */

struct pf_rule_stats {
    atomic64_t packets_matched;
    atomic64_t bytes_matched;

    atomic64_t packets_allowed;
    atomic64_t packets_dropped;
    atomic64_t packets_monitored;
};


/* ============================================================
 * Rule Definition
 * ============================================================ */

struct pf_engine_rule {
    /*
     * Rule list node.
     */
    struct list_head list;

    /*
     * Unique rule identifier.
     */
    __u32 id;

    /*
     * Human-readable rule name.
     */
    char name[PF_RULE_ENGINE_MAX_NAME];

    /*
     * Rule priority.
     */
    __u32 priority;

    /*
     * Rule type.
     */
    enum pf_rule_type type;

    /*
     * Action when rule matches.
     */
    enum pf_action action;

    /*
     * Rule state.
     */
    enum pf_rule_state state;

    /*
     * Direction.
     */
    enum pf_rule_direction direction;

    /*
     * Fields participating in matching.
     */
    __u32 match_flags;

    /*
     * Source IPv4 address and mask.
     */
    __be32 src_ip;
    __be32 src_mask;

    /*
     * Destination IPv4 address and mask.
     */
    __be32 dst_ip;
    __be32 dst_mask;

    /*
     * Source port range.
     */
    __be16 src_port_start;
    __be16 src_port_end;

    /*
     * Destination port range.
     */
    __be16 dst_port_start;
    __be16 dst_port_end;

    /*
     * IP protocol.
     *
     * 0 = ANY
     * 6 = TCP
     * 17 = UDP
     * 1 = ICMP
     */
    __u8 protocol;

    /*
     * MAC address matching.
     */
    __u8 src_mac[PF_ETH_ADDR_LEN];
    __u8 src_mac_mask[PF_ETH_ADDR_LEN];

    __u8 dst_mac[PF_ETH_ADDR_LEN];
    __u8 dst_mac_mask[PF_ETH_ADDR_LEN];

    /*
     * Network interface.
     */
    char interface[PF_RULE_ENGINE_MAX_INTERFACE];

    /*
     * Runtime statistics.
     */
    struct pf_rule_stats stats;
};


/* ============================================================
 * Rule Engine Context
 * ============================================================ */

struct pf_rule_engine {
    /*
     * Rule database.
     */
    struct list_head rules;

    /*
     * Protects rule database.
     */
    struct mutex lock;

    /*
     * Number of configured rules.
     */
    __u32 rule_count;

    /*
     * Next automatically assigned rule ID.
     */
    __u32 next_rule_id;

    /*
     * Engine state.
     */
    bool enabled;

    /*
     * Default action when no rule matches.
     */
    enum pf_action default_action;
};


/* ============================================================
 * Match Result Information
 * ============================================================ */

struct pf_rule_match {
    /*
     * Whether a rule matched.
     */
    bool matched;

    /*
     * Matching rule.
     */
    struct pf_engine_rule *rule;

    /*
     * Resulting action.
     */
    enum pf_action action;

    /*
     * Rule ID.
     */
    __u32 rule_id;

    /*
     * Rule priority.
     */
    __u32 priority;
};


/* ============================================================
 * Rule Engine Lifecycle
 * ============================================================ */

/*
 * Initialize rule engine.
 */
int pf_rule_engine_init(void);


/*
 * Cleanup rule engine.
 */
void pf_rule_engine_exit(void);


/*
 * Enable rule engine.
 */
int pf_rule_engine_enable(void);


/*
 * Disable rule engine.
 */
int pf_rule_engine_disable(void);


/*
 * Check whether rule engine is enabled.
 */
bool pf_rule_engine_is_enabled(void);


/* ============================================================
 * Rule Creation
 * ============================================================ */

/*
 * Add a rule.
 */
int pf_rule_engine_add_rule(
    const struct pf_engine_rule *rule);


/*
 * Create a rule and automatically assign ID.
 */
int pf_rule_engine_create_rule(
    struct pf_engine_rule *rule);


/*
 * Delete rule by ID.
 */
int pf_rule_engine_delete_rule(__u32 rule_id);


/*
 * Delete all rules.
 */
void pf_rule_engine_clear_rules(void);


/*
 * Find rule by ID.
 */
struct pf_engine_rule *
pf_rule_engine_find_rule(__u32 rule_id);


/*
 * Enable rule.
 */
int pf_rule_engine_enable_rule(__u32 rule_id);


/*
 * Disable rule.
 */
int pf_rule_engine_disable_rule(__u32 rule_id);


/*
 * Update an existing rule.
 */
int pf_rule_engine_update_rule(
    const struct pf_engine_rule *rule);


/* ============================================================
 * Rule Validation
 * ============================================================ */

/*
 * Validate complete rule definition.
 */
int pf_rule_engine_validate_rule(
    const struct pf_engine_rule *rule);


/*
 * Validate IP configuration.
 */
bool pf_rule_engine_validate_ip(
    __be32 ip,
    __be32 mask);


/*
 * Validate port range.
 */
bool pf_rule_engine_validate_port_range(
    __be16 start,
    __be16 end);


/*
 * Validate priority.
 */
bool pf_rule_engine_validate_priority(
    __u32 priority);


/*
 * Validate protocol.
 */
bool pf_rule_engine_validate_protocol(
    __u8 protocol);


/* ============================================================
 * Rule Matching
 * ============================================================ */

/*
 * Check one rule against one packet.
 */
bool pf_rule_engine_rule_matches(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet);


/*
 * Match source IP.
 */
bool pf_rule_match_source_ip(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet);


/*
 * Match destination IP.
 */
bool pf_rule_match_destination_ip(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet);


/*
 * Match source port.
 */
bool pf_rule_match_source_port(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet);


/*
 * Match destination port.
 */
bool pf_rule_match_destination_port(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet);


/*
 * Match protocol.
 */
bool pf_rule_match_protocol(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet);


/*
 * Match source MAC.
 */
bool pf_rule_match_source_mac(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet);


/*
 * Match destination MAC.
 */
bool pf_rule_match_destination_mac(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet);


/*
 * Match network interface.
 */
bool pf_rule_match_interface(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet);


/* ============================================================
 * Rule Evaluation
 * ============================================================ */

/*
 * Evaluate all rules against a packet.
 *
 * Returns:
 *
 * PF_RULE_NO_MATCH
 * PF_RULE_MATCH_ALLOW
 * PF_RULE_MATCH_DROP
 * PF_RULE_MATCH_MONITOR
 */
enum pf_rule_result pf_rule_engine_evaluate(
    const struct pf_parsed_packet *packet,
    struct pf_rule_match *match);


/*
 * Get action for packet.
 */
enum pf_action pf_rule_engine_get_action(
    const struct pf_parsed_packet *packet);


/*
 * Find highest-priority matching rule.
 */
struct pf_engine_rule *
pf_rule_engine_find_match(
    const struct pf_parsed_packet *packet);


/* ============================================================
 * Default Policy
 * ============================================================ */

/*
 * Set default action.
 */
int pf_rule_engine_set_default_action(
    enum pf_action action);


/*
 * Get default action.
 */
enum pf_action pf_rule_engine_get_default_action(void);


/* ============================================================
 * Whitelist
 * ============================================================ */

/*
 * Add whitelist rule.
 */
int pf_rule_engine_add_whitelist(
    const struct pf_engine_rule *rule);


/*
 * Check whitelist.
 */
bool pf_rule_engine_whitelist_match(
    const struct pf_parsed_packet *packet);


/*
 * Process whitelist rules.
 */
enum pf_action pf_rule_engine_process_whitelist(
    const struct pf_parsed_packet *packet);


/* ============================================================
 * Blacklist
 * ============================================================ */

/*
 * Add blacklist rule.
 */
int pf_rule_engine_add_blacklist(
    const struct pf_engine_rule *rule);


/*
 * Check blacklist.
 */
bool pf_rule_engine_blacklist_match(
    const struct pf_parsed_packet *packet);


/*
 * Process blacklist rules.
 */
enum pf_action pf_rule_engine_process_blacklist(
    const struct pf_parsed_packet *packet);


/* ============================================================
 * Monitoring
 * ============================================================ */

/*
 * Add monitoring rule.
 */
int pf_rule_engine_add_monitor_rule(
    const struct pf_engine_rule *rule);


/*
 * Check monitoring rules.
 */
bool pf_rule_engine_monitor_match(
    const struct pf_parsed_packet *packet);


/*
 * Process monitoring rules.
 */
enum pf_action pf_rule_engine_process_monitor(
    const struct pf_parsed_packet *packet);


/* ============================================================
 * Statistics
 * ============================================================ */

/*
 * Reset statistics for one rule.
 */
void pf_rule_engine_reset_rule_stats(
    struct pf_engine_rule *rule);


/*
 * Reset all rule statistics.
 */
void pf_rule_engine_reset_statistics(void);


/*
 * Update rule statistics after a match.
 */
void pf_rule_engine_update_statistics(
    struct pf_engine_rule *rule,
    enum pf_action action,
    unsigned int packet_length);


/*
 * Get total number of rules.
 */
__u32 pf_rule_engine_rule_count(void);


/* ============================================================
 * Rule Ordering
 * ============================================================ */

/*
 * Insert rule according to priority.
 */
int pf_rule_engine_insert_by_priority(
    struct pf_engine_rule *rule);


/*
 * Reorder all rules according to priority.
 */
void pf_rule_engine_sort_rules(void);


/* ============================================================
 * Rule Dump / Debug
 * ============================================================ */

/*
 * Print one rule.
 */
void pf_rule_engine_dump_rule(
    const struct pf_engine_rule *rule);


/*
 * Print all configured rules.
 */
void pf_rule_engine_dump_rules(void);


/*
 * Print rule statistics.
 */
void pf_rule_engine_dump_statistics(
    const struct pf_engine_rule *rule);


/* ============================================================
 * Helper Functions
 * ============================================================ */

/*
 * Convert rule type to string.
 */
const char *pf_rule_engine_type_name(
    enum pf_rule_type type);


/*
 * Convert action to string.
 */
const char *pf_rule_engine_action_name(
    enum pf_action action);


/*
 * Convert direction to string.
 */
const char *pf_rule_engine_direction_name(
    enum pf_rule_direction direction);


/*
 * Check whether rule is enabled.
 */
static inline bool pf_rule_is_enabled(
    const struct pf_engine_rule *rule)
{
    return rule &&
           rule->state == PF_RULE_STATE_ENABLED;
}


/*
 * Check whether rule matches any protocol.
 */
static inline bool pf_rule_matches_any_protocol(
    const struct pf_engine_rule *rule)
{
    return rule &&
           rule->protocol == PF_PROTOCOL_ANY;
}


/*
 * Check whether rule has source-IP matching enabled.
 */
static inline bool pf_rule_has_source_ip_match(
    const struct pf_engine_rule *rule)
{
    return rule &&
           (rule->match_flags & PF_MATCH_SRC_IP);
}


/*
 * Check whether rule has destination-IP matching enabled.
 */
static inline bool pf_rule_has_destination_ip_match(
    const struct pf_engine_rule *rule)
{
    return rule &&
           (rule->match_flags & PF_MATCH_DST_IP);
}


/*
 * Check whether rule has source-port matching enabled.
 */
static inline bool pf_rule_has_source_port_match(
    const struct pf_engine_rule *rule)
{
    return rule &&
           (rule->match_flags & PF_MATCH_SRC_PORT);
}


/*
 * Check whether rule has destination-port matching enabled.
 */
static inline bool pf_rule_has_destination_port_match(
    const struct pf_engine_rule *rule)
{
    return rule &&
           (rule->match_flags & PF_MATCH_DST_PORT);
}


/*
 * Check whether rule has protocol matching enabled.
 */
static inline bool pf_rule_has_protocol_match(
    const struct pf_engine_rule *rule)
{
    return rule &&
           (rule->match_flags & PF_MATCH_PROTOCOL);
}


/*
 * Check whether rule has interface matching enabled.
 */
static inline bool pf_rule_has_interface_match(
    const struct pf_engine_rule *rule)
{
    return rule &&
           (rule->match_flags & PF_MATCH_INTERFACE);
}


#endif /* PACKET_FILTER_RULE_ENGINE_H */
