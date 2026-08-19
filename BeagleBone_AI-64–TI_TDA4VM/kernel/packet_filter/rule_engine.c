/*
 * Packet Filter Driver - Rule Engine
 *
 * BeagleBone AI-64 / TI TDA4VM
 *
 * Responsibilities:
 *   - Rule database management
 *   - Rule validation
 *   - Rule priority
 *   - Whitelist matching
 *   - Blacklist matching
 *   - Monitoring rules
 *   - Packet/rule statistics
 *   - Final packet-filter decision
 */

#include "rule_engine.h"
#include "logging.h"

#include <linux/etherdevice.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/string.h>

/* ============================================================
 * Module Information
 * ============================================================ */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BeagleBone AI-64 Packet Filter Project");
MODULE_DESCRIPTION(
    "Packet filtering rule engine for BeagleBone AI-64 / TI TDA4VM"
);
MODULE_VERSION("1.0");


/* ============================================================
 * Global Rule Engine
 * ============================================================ */

static struct pf_rule_engine pf_engine;


/* ============================================================
 * Internal Helpers
 * ============================================================ */

static bool pf_engine_initialized;

static bool pf_mac_match(const __u8 *packet_mac,
                         const __u8 *rule_mac,
                         const __u8 *mask)
{
    int i;

    if (!packet_mac || !rule_mac || !mask)
        return false;

    for (i = 0; i < PF_ETH_ADDR_LEN; i++) {
        if ((packet_mac[i] & mask[i]) !=
            (rule_mac[i] & mask[i]))
            return false;
    }

    return true;
}


static bool pf_rule_is_type(
    const struct pf_engine_rule *rule,
    enum pf_rule_type type)
{
    return rule && rule->type == type;
}


/* ============================================================
 * Rule Engine Initialization
 * ============================================================ */

int pf_rule_engine_init(void)
{
    if (pf_engine_initialized)
        return -EEXIST;

    memset(&pf_engine, 0, sizeof(pf_engine));

    INIT_LIST_HEAD(&pf_engine.rules);

    mutex_init(&pf_engine.lock);

    pf_engine.rule_count = 0;
    pf_engine.next_rule_id = 1;

    /*
     * Safe default:
     *
     * If no rule matches, allow the packet.
     */
    pf_engine.default_action = PF_ACTION_ALLOW;

    pf_engine.enabled = true;

    pf_engine_initialized = true;

    pf_log_info("rule engine initialized\n");

    return 0;
}


/* ============================================================
 * Rule Engine Cleanup
 * ============================================================ */

void pf_rule_engine_exit(void)
{
    struct pf_engine_rule *rule;
    struct pf_engine_rule *tmp;

    if (!pf_engine_initialized)
        return;

    mutex_lock(&pf_engine.lock);

    list_for_each_entry_safe(
        rule,
        tmp,
        &pf_engine.rules,
        list) {

        list_del(&rule->list);

        kfree(rule);
    }

    pf_engine.rule_count = 0;

    mutex_unlock(&pf_engine.lock);

    pf_engine_initialized = false;

    pf_log_info("rule engine removed\n");
}


/* ============================================================
 * Enable / Disable
 * ============================================================ */

int pf_rule_engine_enable(void)
{
    if (!pf_engine_initialized)
        return -ENODEV;

    mutex_lock(&pf_engine.lock);

    pf_engine.enabled = true;

    mutex_unlock(&pf_engine.lock);

    pf_log_info("rule engine enabled\n");

    return 0;
}


int pf_rule_engine_disable(void)
{
    if (!pf_engine_initialized)
        return -ENODEV;

    mutex_lock(&pf_engine.lock);

    pf_engine.enabled = false;

    mutex_unlock(&pf_engine.lock);

    pf_log_info("rule engine disabled\n");

    return 0;
}


bool pf_rule_engine_is_enabled(void)
{
    bool enabled;

    if (!pf_engine_initialized)
        return false;

    mutex_lock(&pf_engine.lock);

    enabled = pf_engine.enabled;

    mutex_unlock(&pf_engine.lock);

    return enabled;
}


/* ============================================================
 * Rule Validation
 * ============================================================ */

bool pf_rule_engine_validate_ip(__be32 ip,
                                __be32 mask)
{
    /*
     * A zero mask represents ANY address.
     */
    if (mask == 0)
        return true;

    /*
     * Address can be any valid IPv4 address.
     */
    return true;
}


bool pf_rule_engine_validate_port_range(__be16 start,
                                        __be16 end)
{
    __u16 start_port;
    __u16 end_port;

    start_port = ntohs(start);
    end_port = ntohs(end);

    if (start_port > end_port)
        return false;

    return true;
}


bool pf_rule_engine_validate_priority(__u32 priority)
{
    return priority >= PF_RULE_PRIORITY_MIN &&
           priority <= PF_RULE_PRIORITY_MAX;
}


bool pf_rule_engine_validate_protocol(__u8 protocol)
{
    switch (protocol) {

    case PF_PROTOCOL_ANY:
    case PF_PROTOCOL_TCP:
    case PF_PROTOCOL_UDP:
    case PF_PROTOCOL_ICMP:
        return true;

    default:
        return false;
    }
}


int pf_rule_engine_validate_rule(
    const struct pf_engine_rule *rule)
{
    if (!rule)
        return -EINVAL;

    if (!pf_rule_engine_validate_priority(
            rule->priority))
        return -EINVAL;

    if (!pf_rule_engine_validate_protocol(
            rule->protocol))
        return -EINVAL;

    if (!pf_rule_engine_validate_ip(
            rule->src_ip,
            rule->src_mask))
        return -EINVAL;

    if (!pf_rule_engine_validate_ip(
            rule->dst_ip,
            rule->dst_mask))
        return -EINVAL;

    if (rule->match_flags & PF_MATCH_SRC_PORT) {

        if (!pf_rule_engine_validate_port_range(
                rule->src_port_start,
                rule->src_port_end))
            return -EINVAL;
    }

    if (rule->match_flags & PF_MATCH_DST_PORT) {

        if (!pf_rule_engine_validate_port_range(
                rule->dst_port_start,
                rule->dst_port_end))
            return -EINVAL;
    }

    switch (rule->type) {

    case PF_RULE_WHITELIST:
    case PF_RULE_BLACKLIST:
    case PF_RULE_MONITOR:
        break;

    default:
        return -EINVAL;
    }

    switch (rule->action) {

    case PF_ACTION_ALLOW:
    case PF_ACTION_DROP:
    case PF_ACTION_MONITOR:
        break;

    default:
        return -EINVAL;
    }

    switch (rule->direction) {

    case PF_RULE_DIR_ANY:
    case PF_RULE_DIR_INGRESS:
    case PF_RULE_DIR_EGRESS:
        break;

    default:
        return -EINVAL;
    }

    return 0;
}


/* ============================================================
 * Rule Creation
 * ============================================================ */

int pf_rule_engine_create_rule(
    struct pf_engine_rule *rule)
{
    int ret;

    if (!rule)
        return -EINVAL;

    if (!pf_engine_initialized)
        return -ENODEV;

    mutex_lock(&pf_engine.lock);

    rule->id = pf_engine.next_rule_id++;

    mutex_unlock(&pf_engine.lock);

    ret = pf_rule_engine_add_rule(rule);

    if (ret)
        return ret;

    return rule->id;
}


/* ============================================================
 * Find Rule
 * ============================================================ */

struct pf_engine_rule *
pf_rule_engine_find_rule(__u32 rule_id)
{
    struct pf_engine_rule *rule;

    if (!pf_engine_initialized)
        return NULL;

    list_for_each_entry(
        rule,
        &pf_engine.rules,
        list) {

        if (rule->id == rule_id)
            return rule;
    }

    return NULL;
}


/* ============================================================
 * Add Rule
 * ============================================================ */

int pf_rule_engine_add_rule(
    const struct pf_engine_rule *rule)
{
    struct pf_engine_rule *new_rule;
    int ret;

    if (!rule)
        return -EINVAL;

    if (!pf_engine_initialized)
        return -ENODEV;

    ret = pf_rule_engine_validate_rule(rule);

    if (ret)
        return ret;

    new_rule = kmemdup(
        rule,
        sizeof(*new_rule),
        GFP_KERNEL
    );

    if (!new_rule)
        return -ENOMEM;

    INIT_LIST_HEAD(&new_rule->list);

    pf_rule_engine_reset_rule_stats(new_rule);

    mutex_lock(&pf_engine.lock);

    if (pf_engine.rule_count >=
        PF_RULE_ENGINE_MAX_RULES) {

        mutex_unlock(&pf_engine.lock);

        kfree(new_rule);

        return -ENOSPC;
    }

    if (new_rule->id == 0)
        new_rule->id = pf_engine.next_rule_id++;

    if (pf_rule_engine_find_rule(new_rule->id)) {

        mutex_unlock(&pf_engine.lock);

        kfree(new_rule);

        return -EEXIST;
    }

    list_add_tail(
        &new_rule->list,
        &pf_engine.rules
    );

    pf_engine.rule_count++;

    mutex_unlock(&pf_engine.lock);

    /*
     * Reorder rules by priority.
     */
    pf_rule_engine_sort_rules();

    pf_log_rule_added(new_rule->name);

    return 0;
}


/* ============================================================
 * Delete Rule
 * ============================================================ */

int pf_rule_engine_delete_rule(__u32 rule_id)
{
    struct pf_engine_rule *rule;

    if (!pf_engine_initialized)
        return -ENODEV;

    mutex_lock(&pf_engine.lock);

    rule = pf_rule_engine_find_rule(rule_id);

    if (!rule) {
        mutex_unlock(&pf_engine.lock);
        return -ENOENT;
    }

    list_del(&rule->list);

    if (pf_engine.rule_count > 0)
        pf_engine.rule_count--;

    mutex_unlock(&pf_engine.lock);

    pf_log_rule_removed(rule->name);

    kfree(rule);

    return 0;
}


/* ============================================================
 * Clear Rules
 * ============================================================ */

void pf_rule_engine_clear_rules(void)
{
    struct pf_engine_rule *rule;
    struct pf_engine_rule *tmp;

    if (!pf_engine_initialized)
        return;

    mutex_lock(&pf_engine.lock);

    list_for_each_entry_safe(
        rule,
        tmp,
        &pf_engine.rules,
        list) {

        list_del(&rule->list);

        kfree(rule);
    }

    pf_engine.rule_count = 0;

    mutex_unlock(&pf_engine.lock);

    pf_log_info("all rule-engine rules cleared\n");
}


/* ============================================================
 * Enable / Disable Individual Rule
 * ============================================================ */

int pf_rule_engine_enable_rule(__u32 rule_id)
{
    struct pf_engine_rule *rule;

    if (!pf_engine_initialized)
        return -ENODEV;

    mutex_lock(&pf_engine.lock);

    rule = pf_rule_engine_find_rule(rule_id);

    if (!rule) {
        mutex_unlock(&pf_engine.lock);
        return -ENOENT;
    }

    rule->state = PF_RULE_STATE_ENABLED;

    mutex_unlock(&pf_engine.lock);

    return 0;
}


int pf_rule_engine_disable_rule(__u32 rule_id)
{
    struct pf_engine_rule *rule;

    if (!pf_engine_initialized)
        return -ENODEV;

    mutex_lock(&pf_engine.lock);

    rule = pf_rule_engine_find_rule(rule_id);

    if (!rule) {
        mutex_unlock(&pf_engine.lock);
        return -ENOENT;
    }

    rule->state = PF_RULE_STATE_DISABLED;

    mutex_unlock(&pf_engine.lock);

    return 0;
}


/* ============================================================
 * Update Rule
 * ============================================================ */

int pf_rule_engine_update_rule(
    const struct pf_engine_rule *rule)
{
    struct pf_engine_rule *existing;
    int ret;

    if (!rule)
        return -EINVAL;

    ret = pf_rule_engine_validate_rule(rule);

    if (ret)
        return ret;

    mutex_lock(&pf_engine.lock);

    existing = pf_rule_engine_find_rule(rule->id);

    if (!existing) {
        mutex_unlock(&pf_engine.lock);
        return -ENOENT;
    }

    /*
     * Preserve linked-list node and statistics.
     */
    existing->priority = rule->priority;
    existing->type = rule->type;
    existing->action = rule->action;
    existing->state = rule->state;
    existing->direction = rule->direction;
    existing->match_flags = rule->match_flags;

    existing->src_ip = rule->src_ip;
    existing->src_mask = rule->src_mask;

    existing->dst_ip = rule->dst_ip;
    existing->dst_mask = rule->dst_mask;

    existing->src_port_start = rule->src_port_start;
    existing->src_port_end = rule->src_port_end;

    existing->dst_port_start = rule->dst_port_start;
    existing->dst_port_end = rule->dst_port_end;

    existing->protocol = rule->protocol;

    memcpy(
        existing->src_mac,
        rule->src_mac,
        PF_ETH_ADDR_LEN
    );

    memcpy(
        existing->src_mac_mask,
        rule->src_mac_mask,
        PF_ETH_ADDR_LEN
    );

    memcpy(
        existing->dst_mac,
        rule->dst_mac,
        PF_ETH_ADDR_LEN
    );

    memcpy(
        existing->dst_mac_mask,
        rule->dst_mac_mask,
        PF_ETH_ADDR_LEN
    );

    strscpy(
        existing->name,
        rule->name,
        sizeof(existing->name)
    );

    strscpy(
        existing->interface,
        rule->interface,
        sizeof(existing->interface)
    );

    mutex_unlock(&pf_engine.lock);

    pf_rule_engine_sort_rules();

    return 0;
}


/* ============================================================
 * Source IP Matching
 * ============================================================ */

bool pf_rule_match_source_ip(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet)
{
    if (!rule || !packet)
        return false;

    if (!(rule->match_flags & PF_MATCH_SRC_IP))
        return true;

    return pf_ipv4_address_match(
        packet->ipv4.src_ip,
        rule->src_ip,
        rule->src_mask
    );
}


/* ============================================================
 * Destination IP Matching
 * ============================================================ */

bool pf_rule_match_destination_ip(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet)
{
    if (!rule || !packet)
        return false;

    if (!(rule->match_flags & PF_MATCH_DST_IP))
        return true;

    return pf_ipv4_address_match(
        packet->ipv4.dst_ip,
        rule->dst_ip,
        rule->dst_mask
    );
}


/* ============================================================
 * Source Port Matching
 * ============================================================ */

bool pf_rule_match_source_port(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet)
{
    __be16 port;

    if (!rule || !packet)
        return false;

    if (!(rule->match_flags & PF_MATCH_SRC_PORT))
        return true;

    if (!pf_packet_is_tcp(packet) &&
        !pf_packet_is_udp(packet))
        return false;

    port = pf_packet_src_port(packet);

    return pf_port_match(
        port,
        rule->src_port_start,
        rule->src_port_end
    );
}


/* ============================================================
 * Destination Port Matching
 * ============================================================ */

bool pf_rule_match_destination_port(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet)
{
    __be16 port;

    if (!rule || !packet)
        return false;

    if (!(rule->match_flags & PF_MATCH_DST_PORT))
        return true;

    if (!pf_packet_is_tcp(packet) &&
        !pf_packet_is_udp(packet))
        return false;

    port = pf_packet_dst_port(packet);

    return pf_port_match(
        port,
        rule->dst_port_start,
        rule->dst_port_end
    );
}


/* ============================================================
 * Protocol Matching
 * ============================================================ */

bool pf_rule_match_protocol(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet)
{
    if (!rule || !packet)
        return false;

    if (!(rule->match_flags & PF_MATCH_PROTOCOL))
        return true;

    if (rule->protocol == PF_PROTOCOL_ANY)
        return true;

    switch (rule->protocol) {

    case PF_PROTOCOL_TCP:
        return pf_packet_is_tcp(packet);

    case PF_PROTOCOL_UDP:
        return pf_packet_is_udp(packet);

    case PF_PROTOCOL_ICMP:
        return pf_packet_is_icmp(packet);

    default:
        return false;
    }
}


/* ============================================================
 * Source MAC Matching
 * ============================================================ */

bool pf_rule_match_source_mac(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet)
{
    if (!rule || !packet)
        return false;

    if (!(rule->match_flags & PF_MATCH_MAC_SRC))
        return true;

    return pf_mac_match(
        packet->eth.src_mac,
        rule->src_mac,
        rule->src_mac_mask
    );
}


/* ============================================================
 * Destination MAC Matching
 * ============================================================ */

bool pf_rule_match_destination_mac(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet)
{
    if (!rule || !packet)
        return false;

    if (!(rule->match_flags & PF_MATCH_MAC_DST))
        return true;

    return pf_mac_match(
        packet->eth.dst_mac,
        rule->dst_mac,
        rule->dst_mac_mask
    );
}


/* ============================================================
 * Interface Matching
 * ============================================================ */

bool pf_rule_match_interface(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet)
{
    if (!rule || !packet)
        return false;

    if (!(rule->match_flags & PF_MATCH_INTERFACE))
        return true;

    if (!packet->dev)
        return false;

    if (rule->interface[0] == '\0')
        return true;

    return strncmp(
        packet->dev->name,
        rule->interface,
        PF_RULE_ENGINE_MAX_INTERFACE
    ) == 0;
}


/* ============================================================
 * Complete Rule Matching
 * ============================================================ */

bool pf_rule_engine_rule_matches(
    const struct pf_engine_rule *rule,
    const struct pf_parsed_packet *packet)
{
    if (!rule || !packet)
        return false;

    if (!pf_rule_is_enabled(rule))
        return false;

    if (!pf_rule_match_source_ip(rule, packet))
        return false;

    if (!pf_rule_match_destination_ip(rule, packet))
        return false;

    if (!pf_rule_match_source_port(rule, packet))
        return false;

    if (!pf_rule_match_destination_port(rule, packet))
        return false;

    if (!pf_rule_match_protocol(rule, packet))
        return false;

    if (!pf_rule_match_source_mac(rule, packet))
        return false;

    if (!pf_rule_match_destination_mac(rule, packet))
        return false;

    if (!pf_rule_match_interface(rule, packet))
        return false;

    return true;
}


/* ============================================================
 * Find Highest Priority Match
 * ============================================================ */

struct pf_engine_rule *
pf_rule_engine_find_match(
    const struct pf_parsed_packet *packet)
{
    struct pf_engine_rule *rule;

    if (!packet || !pf_engine_initialized)
        return NULL;

    /*
     * Rules are kept sorted by priority.
     * First match is therefore the highest-priority match.
     */
    list_for_each_entry(
        rule,
        &pf_engine.rules,
        list) {

        if (pf_rule_engine_rule_matches(
                rule,
                packet))
            return rule;
    }

    return NULL;
}


/* ============================================================
 * Rule Evaluation
 * ============================================================ */

enum pf_rule_result pf_rule_engine_evaluate(
    const struct pf_parsed_packet *packet,
    struct pf_rule_match *match)
{
    struct pf_engine_rule *rule;
    enum pf_action action;

    if (match)
        memset(match, 0, sizeof(*match));

    if (!packet)
        return PF_RULE_NO_MATCH;

    if (!pf_engine_initialized)
        return PF_RULE_NO_MATCH;

    if (!pf_rule_engine_is_enabled()) {

        if (match) {
            match->matched = false;
            match->rule = NULL;
            match->action =
                pf_engine.default_action;
        }

        return PF_RULE_NO_MATCH;
    }

    mutex_lock(&pf_engine.lock);

    rule = pf_rule_engine_find_match(packet);

    if (!rule) {

        action = pf_engine.default_action;

        mutex_unlock(&pf_engine.lock);

        if (match) {
            match->matched = false;
            match->rule = NULL;
            match->action = action;
            match->rule_id = 0;
            match->priority = 0;
        }

        return PF_RULE_NO_MATCH;
    }

    action = rule->action;

    if (match) {
        match->matched = true;
        match->rule = rule;
        match->action = action;
        match->rule_id = rule->id;
        match->priority = rule->priority;
    }

    pf_rule_engine_update_statistics(
        rule,
        action,
        packet->packet_length
    );

    mutex_unlock(&pf_engine.lock);

    switch (action) {

    case PF_ACTION_ALLOW:
        return PF_RULE_MATCH_ALLOW;

    case PF_ACTION_DROP:
        return PF_RULE_MATCH_DROP;

    case PF_ACTION_MONITOR:
        return PF_RULE_MATCH_MONITOR;

    default:
        return PF_RULE_NO_MATCH;
    }
}


/* ============================================================
 * Get Final Action
 * ============================================================ */

enum pf_action pf_rule_engine_get_action(
    const struct pf_parsed_packet *packet)
{
    struct pf_rule_match match;
    enum pf_rule_result result;

    result = pf_rule_engine_evaluate(
        packet,
        &match
    );

    switch (result) {

    case PF_RULE_MATCH_ALLOW:
        return PF_ACTION_ALLOW;

    case PF_RULE_MATCH_DROP:
        return PF_ACTION_DROP;

    case PF_RULE_MATCH_MONITOR:
        return PF_ACTION_MONITOR;

    case PF_RULE_NO_MATCH:
    default:
        return pf_engine.default_action;
    }
}


/* ============================================================
 * Default Policy
 * ============================================================ */

int pf_rule_engine_set_default_action(
    enum pf_action action)
{
    switch (action) {

    case PF_ACTION_ALLOW:
    case PF_ACTION_DROP:
    case PF_ACTION_MONITOR:
        break;

    default:
        return -EINVAL;
    }

    if (!pf_engine_initialized)
        return -ENODEV;

    mutex_lock(&pf_engine.lock);

    pf_engine.default_action = action;

    mutex_unlock(&pf_engine.lock);

    return 0;
}


enum pf_action pf_rule_engine_get_default_action(void)
{
    enum pf_action action;

    if (!pf_engine_initialized)
        return PF_ACTION_ALLOW;

    mutex_lock(&pf_engine.lock);

    action = pf_engine.default_action;

    mutex_unlock(&pf_engine.lock);

    return action;
}


/* ============================================================
 * Whitelist
 * ============================================================ */

int pf_rule_engine_add_whitelist(
    const struct pf_engine_rule *rule)
{
    struct pf_engine_rule whitelist_rule;

    if (!rule)
        return -EINVAL;

    memcpy(
        &whitelist_rule,
        rule,
        sizeof(whitelist_rule)
    );

    whitelist_rule.type = PF_RULE_WHITELIST;
    whitelist_rule.action = PF_ACTION_ALLOW;

    return pf_rule_engine_add_rule(
        &whitelist_rule
    );
}


bool pf_rule_engine_whitelist_match(
    const struct pf_parsed_packet *packet)
{
    struct pf_engine_rule *rule;

    if (!packet || !pf_engine_initialized)
        return false;

    mutex_lock(&pf_engine.lock);

    list_for_each_entry(
        rule,
        &pf_engine.rules,
        list) {

        if (!pf_rule_is_type(
                rule,
                PF_RULE_WHITELIST))
            continue;

        if (pf_rule_engine_rule_matches(
                rule,
                packet)) {

            mutex_unlock(&pf_engine.lock);

            return true;
        }
    }

    mutex_unlock(&pf_engine.lock);

    return false;
}


enum pf_action pf_rule_engine_process_whitelist(
    const struct pf_parsed_packet *packet)
{
    if (pf_rule_engine_whitelist_match(packet))
        return PF_ACTION_ALLOW;

    return pf_engine.default_action;
}


/* ============================================================
 * Blacklist
 * ============================================================ */

int pf_rule_engine_add_blacklist(
    const struct pf_engine_rule *rule)
{
    struct pf_engine_rule blacklist_rule;

    if (!rule)
        return -EINVAL;

    memcpy(
        &blacklist_rule,
        rule,
        sizeof(blacklist_rule)
    );

    blacklist_rule.type = PF_RULE_BLACKLIST;
    blacklist_rule.action = PF_ACTION_DROP;

    return pf_rule_engine_add_rule(
        &blacklist_rule
    );
}


bool pf_rule_engine_blacklist_match(
    const struct pf_parsed_packet *packet)
{
    struct pf_engine_rule *rule;

    if (!packet || !pf_engine_initialized)
        return false;

    mutex_lock(&pf_engine.lock);

    list_for_each_entry(
        rule,
        &pf_engine.rules,
        list) {

        if (!pf_rule_is_type(
                rule,
                PF_RULE_BLACKLIST))
            continue;

        if (pf_rule_engine_rule_matches(
                rule,
                packet)) {

            mutex_unlock(&pf_engine.lock);

            return true;
        }
    }

    mutex_unlock(&pf_engine.lock);

    return false;
}


enum pf_action pf_rule_engine_process_blacklist(
    const struct pf_parsed_packet *packet)
{
    if (pf_rule_engine_blacklist_match(packet))
        return PF_ACTION_DROP;

    return pf_engine.default_action;
}


/* ============================================================
 * Monitoring
 * ============================================================ */

int pf_rule_engine_add_monitor_rule(
    const struct pf_engine_rule *rule)
{
    struct pf_engine_rule monitor_rule;

    if (!rule)
        return -EINVAL;

    memcpy(
        &monitor_rule,
        rule,
        sizeof(monitor_rule)
    );

    monitor_rule.type = PF_RULE_MONITOR;
    monitor_rule.action = PF_ACTION_MONITOR;

    return pf_rule_engine_add_rule(
        &monitor_rule
    );
}


bool pf_rule_engine_monitor_match(
    const struct pf_parsed_packet *packet)
{
    struct pf_engine_rule *rule;

    if (!packet || !pf_engine_initialized)
        return false;

    mutex_lock(&pf_engine.lock);

    list_for_each_entry(
        rule,
        &pf_engine.rules,
        list) {

        if (!pf_rule_is_type(
                rule,
                PF_RULE_MONITOR))
            continue;

        if (pf_rule_engine_rule_matches(
                rule,
                packet)) {

            mutex_unlock(&pf_engine.lock);

            return true;
        }
    }

    mutex_unlock(&pf_engine.lock);

    return false;
}


enum pf_action pf_rule_engine_process_monitor(
    const struct pf_parsed_packet *packet)
{
    if (pf_rule_engine_monitor_match(packet))
        return PF_ACTION_MONITOR;

    return pf_engine.default_action;
}


/* ============================================================
 * Rule Statistics
 * ============================================================ */

void pf_rule_engine_reset_rule_stats(
    struct pf_engine_rule *rule)
{
    if (!rule)
        return;

    atomic64_set(
        &rule->stats.packets_matched,
        0
    );

    atomic64_set(
        &rule->stats.bytes_matched,
        0
    );

    atomic64_set(
        &rule->stats.packets_allowed,
        0
    );

    atomic64_set(
        &rule->stats.packets_dropped,
        0
    );

    atomic64_set(
        &rule->stats.packets_monitored,
        0
    );
}


void pf_rule_engine_reset_statistics(void)
{
    struct pf_engine_rule *rule;

    if (!pf_engine_initialized)
        return;

    mutex_lock(&pf_engine.lock);

    list_for_each_entry(
        rule,
        &pf_engine.rules,
        list) {

        pf_rule_engine_reset_rule_stats(rule);
    }

    mutex_unlock(&pf_engine.lock);

    pf_log_info(
        "rule statistics reset\n"
    );
}


void pf_rule_engine_update_statistics(
    struct pf_engine_rule *rule,
    enum pf_action action,
    unsigned int packet_length)
{
    if (!rule)
        return;

    atomic64_inc(
        &rule->stats.packets_matched
    );

    atomic64_add(
        packet_length,
        &rule->stats.bytes_matched
    );

    switch (action) {

    case PF_ACTION_ALLOW:

        atomic64_inc(
            &rule->stats.packets_allowed
        );

        break;

    case PF_ACTION_DROP:

        atomic64_inc(
            &rule->stats.packets_dropped
        );

        break;

    case PF_ACTION_MONITOR:

        atomic64_inc(
            &rule->stats.packets_monitored
        );

        break;

    default:
        break;
    }
}


__u32 pf_rule_engine_rule_count(void)
{
    __u32 count;

    if (!pf_engine_initialized)
        return 0;

    mutex_lock(&pf_engine.lock);

    count = pf_engine.rule_count;

    mutex_unlock(&pf_engine.lock);

    return count;
}


/* ============================================================
 * Rule Priority
 * ============================================================ */

int pf_rule_engine_insert_by_priority(
    struct pf_engine_rule *rule)
{
    struct pf_engine_rule *current;

    if (!rule)
        return -EINVAL;

    list_for_each_entry(
        current,
        &pf_engine.rules,
        list) {

        if (rule->priority <
            current->priority) {

            list_add_tail(
                &rule->list,
                &current->list
            );

            return 0;
        }
    }

    list_add_tail(
        &rule->list,
        &pf_engine.rules
    );

    return 0;
}


void pf_rule_engine_sort_rules(void)
{
    struct pf_engine_rule *rule;
    struct pf_engine_rule *tmp;

    LIST_HEAD(sorted_rules);

    if (!pf_engine_initialized)
        return;

    mutex_lock(&pf_engine.lock);

    while (!list_empty(&pf_engine.rules)) {

        rule = list_first_entry(
            &pf_engine.rules,
            struct pf_engine_rule,
            list
        );

        list_del_init(&rule->list);

        if (list_empty(&sorted_rules)) {

            list_add(
                &rule->list,
                &sorted_rules
            );

            continue;
        }

        {
            struct pf_engine_rule *current;
            bool inserted = false;

            list_for_each_entry(
                current,
                &sorted_rules,
                list) {

                if (rule->priority <
                    current->priority) {

                    list_add_tail(
                        &rule->list,
                        &current->list
                    );

                    inserted = true;

                    break;
                }
            }

            if (!inserted) {

                list_add_tail(
                    &rule->list,
                    &sorted_rules
                );
            }
        }
    }

    list_for_each_entry_safe(
        rule,
        tmp,
        &sorted_rules,
        list) {

        list_move_tail(
            &rule->list,
            &pf_engine.rules
        );
    }

    mutex_unlock(&pf_engine.lock);
}


/* ============================================================
 * Rule Debug Helpers
 * ============================================================ */

const char *pf_rule_engine_type_name(
    enum pf_rule_type type)
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


const char *pf_rule_engine_action_name(
    enum pf_action action)
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


const char *pf_rule_engine_direction_name(
    enum pf_rule_direction direction)
{
    switch (direction) {

    case PF_RULE_DIR_ANY:
        return "ANY";

    case PF_RULE_DIR_INGRESS:
        return "INGRESS";

    case PF_RULE_DIR_EGRESS:
        return "EGRESS";

    default:
        return "UNKNOWN";
    }
}


void pf_rule_engine_dump_rule(
    const struct pf_engine_rule *rule)
{
    if (!rule)
        return;

    pr_info(
        PF_LOG_PREFIX
        "Rule ID=%u "
        "Name=%s "
        "Priority=%u "
        "Type=%s "
        "Action=%s "
        "State=%s\n",
        rule->id,
        rule->name,
        rule->priority,
        pf_rule_engine_type_name(rule->type),
        pf_rule_engine_action_name(rule->action),
        rule->state == PF_RULE_STATE_ENABLED ?
            "ENABLED" : "DISABLED"
    );

    pr_info(
        PF_LOG_PREFIX
        "  src_ip=%pI4/%pI4 "
        "dst_ip=%pI4/%pI4 "
        "protocol=%u\n",
        &rule->src_ip,
        &rule->src_mask,
        &rule->dst_ip,
        &rule->dst_mask,
        rule->protocol
    );

    pr_info(
        PF_LOG_PREFIX
        "  src_port=%u-%u "
        "dst_port=%u-%u "
        "interface=%s\n",
        ntohs(rule->src_port_start),
        ntohs(rule->src_port_end),
        ntohs(rule->dst_port_start),
        ntohs(rule->dst_port_end),
        rule->interface
    );
}


void pf_rule_engine_dump_rules(void)
{
    struct pf_engine_rule *rule;

    if (!pf_engine_initialized)
        return;

    mutex_lock(&pf_engine.lock);

    list_for_each_entry(
        rule,
        &pf_engine.rules,
        list) {

        pf_rule_engine_dump_rule(rule);
    }

    mutex_unlock(&pf_engine.lock);
}


void pf_rule_engine_dump_statistics(
    const struct pf_engine_rule *rule)
{
    if (!rule)
        return;

    pr_info(
        PF_LOG_PREFIX
        "Rule %u statistics: "
        "matched=%lld "
        "bytes=%lld "
        "allowed=%lld "
        "dropped=%lld "
        "monitored=%lld\n",
        rule->id,

        atomic64_read(
            &rule->stats.packets_matched
        ),

        atomic64_read(
            &rule->stats.bytes_matched
        ),

        atomic64_read(
            &rule->stats.packets_allowed
        ),

        atomic64_read(
            &rule->stats.packets_dropped
        ),

        atomic64_read(
            &rule->stats.packets_monitored
        )
    );
}
