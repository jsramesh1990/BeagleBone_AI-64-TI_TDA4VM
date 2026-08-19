/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter
 *
 * Userspace Filter Library API
 *
 * File:
 *     userspace/include/libfilter.h
 *
 * Purpose:
 *     Public userspace API for communicating with the
 *     kernel packet-filter driver.
 *
 * Architecture:
 *
 *     Application
 *          |
 *          v
 *     libfilter
 *          |
 *          | ioctl()
 *          v
 *     /dev/packet_filter
 *          |
 *          v
 *     Kernel Packet Filter Driver
 *          |
 *          +--> Packet Parser
 *          |
 *          +--> Rule Engine
 *          |
 *          +--> Statistics
 *
 * ============================================================
 */

#ifndef LIBFILTER_H
#define LIBFILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*
 * ============================================================
 * Version
 * ============================================================
 */

#define FILTER_API_VERSION_MAJOR     1
#define FILTER_API_VERSION_MINOR     0

#define FILTER_API_VERSION_STRING    "1.0"

/*
 * ============================================================
 * Device
 * ============================================================
 */

#define FILTER_DEVICE_PATH           "/dev/packet_filter"

/*
 * ============================================================
 * Limits
 * ============================================================
 */

#define FILTER_MAX_RULES             256

#define FILTER_MAX_INTERFACE_NAME    32

#define FILTER_MAX_LOG_MESSAGE       256

/*
 * ============================================================
 * Return Codes
 * ============================================================
 */

#define FILTER_SUCCESS               0

#define FILTER_ERROR                 (-1)

#define FILTER_INVALID_ARGUMENT      (-2)

#define FILTER_DEVICE_ERROR          (-3)

#define FILTER_IOCTL_ERROR           (-4)

#define FILTER_MEMORY_ERROR          (-5)

#define FILTER_NOT_FOUND             (-6)

#define FILTER_ALREADY_EXISTS       (-7)

#define FILTER_PERMISSION_ERROR      (-8)

#define FILTER_NOT_INITIALIZED       (-9)

#define FILTER_BUFFER_TOO_SMALL      (-10)

/*
 * ============================================================
 * Rule Actions
 * ============================================================
 */

typedef enum {

    FILTER_ACTION_ALLOW = 0,

    FILTER_ACTION_DROP = 1,

    FILTER_ACTION_MONITOR = 2

} filter_action_t;


/*
 * ============================================================
 * Protocol Types
 * ============================================================
 */

typedef enum {

    FILTER_PROTOCOL_ANY = 0,

    FILTER_PROTOCOL_TCP = 6,

    FILTER_PROTOCOL_UDP = 17,

    FILTER_PROTOCOL_ICMP = 1

} filter_protocol_t;


/*
 * ============================================================
 * Rule Definition
 * ============================================================
 *
 * IPv4 addresses are stored in network byte order.
 *
 * A value of zero means wildcard.
 *
 * Example:
 *
 *     src_ip = 192.168.1.100
 *     dst_ip = 0
 *
 * means:
 *
 *     Match packets from 192.168.1.100
 *     to any destination.
 *
 * ============================================================
 */

typedef struct {

    uint32_t src_ip;

    uint32_t dst_ip;

    uint32_t src_ip_mask;

    uint32_t dst_ip_mask;

    uint16_t src_port;

    uint16_t dst_port;

    uint8_t protocol;

    uint8_t action;

    uint16_t priority;

    uint32_t rule_id;

    uint8_t enabled;

    uint8_t reserved[3];

} filter_rule_t;


/*
 * ============================================================
 * Packet Information
 * ============================================================
 */

typedef struct {

    uint32_t src_ip;

    uint32_t dst_ip;

    uint16_t src_port;

    uint16_t dst_port;

    uint8_t protocol;

    uint8_t reserved;

    uint16_t packet_length;

} filter_packet_info_t;


/*
 * ============================================================
 * Filter Decision
 * ============================================================
 */

typedef struct {

    uint32_t rule_id;

    uint8_t action;

    uint8_t matched;

    uint16_t reserved;

} filter_decision_t;


/*
 * ============================================================
 * Statistics
 * ============================================================
 */

typedef struct {

    uint64_t packets_total;

    uint64_t packets_allowed;

    uint64_t packets_dropped;

    uint64_t packets_monitored;

    uint64_t bytes_total;

    uint64_t bytes_allowed;

    uint64_t bytes_dropped;

    uint64_t bytes_monitored;

    uint64_t rule_matches;

    uint64_t parser_errors;

    uint64_t malformed_packets;

} filter_statistics_t;


/*
 * ============================================================
 * Interface Statistics
 * ============================================================
 */

typedef struct {

    char interface_name[
        FILTER_MAX_INTERFACE_NAME];

    uint64_t rx_packets;

    uint64_t tx_packets;

    uint64_t rx_bytes;

    uint64_t tx_bytes;

    uint64_t dropped_packets;

} filter_interface_stats_t;


/*
 * ============================================================
 * Library Configuration
 * ============================================================
 */

typedef struct {

    const char *device;

    const char *rule_file;

    const char *interface;

    uint32_t flags;

} filter_config_t;


/*
 * ============================================================
 * Library Handle
 * ============================================================
 */

typedef struct filter_handle filter_handle_t;


/*
 * ============================================================
 * Initialization
 * ============================================================
 */

/**
 * filter_init()
 *
 * Initialize the userspace filter library.
 *
 * Returns:
 *
 *     FILTER_SUCCESS
 *     FILTER_ERROR
 *     FILTER_DEVICE_ERROR
 */
int filter_init(void);


/**
 * filter_init_with_config()
 *
 * Initialize the library using custom configuration.
 */
int filter_init_with_config(
        const filter_config_t *config);


/**
 * filter_cleanup()
 *
 * Release all userspace resources.
 */
void filter_cleanup(void);


/*
 * ============================================================
 * Device Operations
 * ============================================================
 */

/**
 * filter_open()
 *
 * Open the packet-filter device.
 *
 * Returns:
 *
 *     File descriptor >= 0
 *     Negative error code on failure.
 */
int filter_open(void);


/**
 * filter_close()
 *
 * Close packet-filter device.
 */
void filter_close(void);


/**
 * filter_is_open()
 *
 * Check whether the device is currently open.
 *
 * Returns:
 *
 *     1 - open
 *     0 - closed
 */
int filter_is_open(void);


/*
 * ============================================================
 * Rule Operations
 * ============================================================
 */

/**
 * filter_add_rule()
 *
 * Add a packet-filter rule.
 */
int filter_add_rule(
        const filter_rule_t *rule);


/**
 * filter_remove_rule()
 *
 * Remove a rule using its rule ID.
 */
int filter_remove_rule(
        uint32_t rule_id);


/**
 * filter_update_rule()
 *
 * Update an existing rule.
 */
int filter_update_rule(
        const filter_rule_t *rule);


/**
 * filter_get_rule()
 *
 * Retrieve a rule using rule ID.
 */
int filter_get_rule(
        uint32_t rule_id,
        filter_rule_t *rule);


/**
 * filter_get_rules()
 *
 * Retrieve all active rules.
 *
 * count:
 *     Input/output number of rules.
 */
int filter_get_rules(
        filter_rule_t *rules,
        uint32_t *count);


/**
 * filter_clear_rules()
 *
 * Remove all rules.
 */
int filter_clear_rules(void);


/**
 * filter_enable_rule()
 *
 * Enable a rule.
 */
int filter_enable_rule(
        uint32_t rule_id);


/**
 * filter_disable_rule()
 *
 * Disable a rule.
 */
int filter_disable_rule(
        uint32_t rule_id);


/*
 * ============================================================
 * Rule File Operations
 * ============================================================
 */

/**
 * filter_load_rules()
 *
 * Load rules from a configuration file.
 *
 * Example:
 *
 *     filter_load_rules(
 *         "userspace/config/default_rules.conf");
 */
int filter_load_rules(
        const char *filename);


/**
 * filter_save_rules()
 *
 * Save current rules to a configuration file.
 */
int filter_save_rules(
        const char *filename);


/**
 * filter_reload_rules()
 *
 * Reload rules from the configured rule file.
 */
int filter_reload_rules(void);


/*
 * ============================================================
 * Statistics Operations
 * ============================================================
 */

/**
 * filter_get_statistics()
 *
 * Read packet-filter statistics from the driver.
 */
int filter_get_statistics(
        filter_statistics_t *statistics);


/**
 * filter_reset_statistics()
 *
 * Reset packet counters.
 */
int filter_reset_statistics(void);


/**
 * filter_get_interface_statistics()
 *
 * Get statistics for a network interface.
 */
int filter_get_interface_statistics(
        const char *interface,
        filter_interface_stats_t *statistics);


/*
 * ============================================================
 * Packet Operations
 * ============================================================
 */

/**
 * filter_process_packet()
 *
 * Process packet metadata through the filter.
 *
 * This API is primarily useful for userspace testing.
 *
 * Normal packet filtering is performed inside the
 * kernel driver.
 */
int filter_process_packet(
        const filter_packet_info_t *packet,
        filter_decision_t *decision);


/*
 * ============================================================
 * Configuration Operations
 * ============================================================
 */

/**
 * filter_set_default_action()
 *
 * Set action used when no rule matches.
 */
int filter_set_default_action(
        filter_action_t action);


/**
 * filter_get_default_action()
 *
 * Retrieve current default action.
 */
int filter_get_default_action(
        filter_action_t *action);


/**
 * filter_set_interface()
 *
 * Select network interface monitored by the filter.
 */
int filter_set_interface(
        const char *interface);


/**
 * filter_get_interface()
 *
 * Retrieve active network interface.
 */
int filter_get_interface(
        char *interface,
        size_t size);


/*
 * ============================================================
 * Monitoring
 * ============================================================
 */

/**
 * filter_enable_monitoring()
 *
 * Enable packet monitoring.
 */
int filter_enable_monitoring(void);


/**
 * filter_disable_monitoring()
 *
 * Disable packet monitoring.
 */
int filter_disable_monitoring(void);


/**
 * filter_monitoring_enabled()
 *
 * Returns:
 *
 *     1 - enabled
 *     0 - disabled
 */
int filter_monitoring_enabled(void);


/*
 * ============================================================
 * Driver Information
 * ============================================================
 */

/**
 * filter_get_driver_version()
 *
 * Get kernel driver version.
 */
int filter_get_driver_version(
        char *version,
        size_t size);


/**
 * filter_get_api_version()
 *
 * Get userspace/kernel API version.
 */
int filter_get_api_version(
        uint32_t *major,
        uint32_t *minor);


/**
 * filter_get_rule_count()
 *
 * Return number of active rules.
 */
int filter_get_rule_count(
        uint32_t *count);


/*
 * ============================================================
 * Utility Functions
 * ============================================================
 */

/**
 * filter_action_to_string()
 *
 * Convert action enum to readable string.
 */
const char *filter_action_to_string(
        filter_action_t action);


/**
 * filter_protocol_to_string()
 *
 * Convert protocol number to readable string.
 */
const char *filter_protocol_to_string(
        uint8_t protocol);


/**
 * filter_error_string()
 *
 * Convert library error code to readable string.
 */
const char *filter_error_string(
        int error);


/**
 * filter_parse_ip()
 *
 * Convert IPv4 string into network byte order.
 */
int filter_parse_ip(
        const char *address,
        uint32_t *ip);


/**
 * filter_format_ip()
 *
 * Convert IPv4 address into string.
 */
int filter_format_ip(
        uint32_t ip,
        char *buffer,
        size_t size);


/*
 * ============================================================
 * Rule Construction Helpers
 * ============================================================
 */

/**
 * filter_rule_init()
 *
 * Initialize rule structure.
 */
void filter_rule_init(
        filter_rule_t *rule);


/**
 * filter_rule_set_source_ip()
 *
 * Configure source IPv4 address.
 */
int filter_rule_set_source_ip(
        filter_rule_t *rule,
        const char *ip);


/**
 * filter_rule_set_destination_ip()
 *
 * Configure destination IPv4 address.
 */
int filter_rule_set_destination_ip(
        filter_rule_t *rule,
        const char *ip);


/**
 * filter_rule_set_source_network()
 *
 * Configure source IPv4 network.
 *
 * Example:
 *
 *     192.168.1.0/24
 */
int filter_rule_set_source_network(
        filter_rule_t *rule,
        const char *network);


/**
 * filter_rule_set_destination_network()
 *
 * Configure destination IPv4 network.
 */
int filter_rule_set_destination_network(
        filter_rule_t *rule,
        const char *network);


/**
 * filter_rule_set_ports()
 *
 * Configure source and destination ports.
 */
int filter_rule_set_ports(
        filter_rule_t *rule,
        uint16_t source_port,
        uint16_t destination_port);


/**
 * filter_rule_set_protocol()
 *
 * Configure protocol.
 */
int filter_rule_set_protocol(
        filter_rule_t *rule,
        filter_protocol_t protocol);


/**
 * filter_rule_set_action()
 *
 * Configure rule action.
 */
int filter_rule_set_action(
        filter_rule_t *rule,
        filter_action_t action);


/**
 * filter_rule_set_priority()
 *
 * Configure rule priority.
 */
int filter_rule_set_priority(
        filter_rule_t *rule,
        uint16_t priority);


/*
 * ============================================================
 * Debugging
 * ============================================================
 */

/**
 * filter_dump_rule()
 *
 * Print a rule to stdout.
 */
void filter_dump_rule(
        const filter_rule_t *rule);


/**
 * filter_dump_statistics()
 *
 * Print statistics to stdout.
 */
void filter_dump_statistics(
        const filter_statistics_t *statistics);


/*
 * ============================================================
 * Library Version
 * ============================================================
 */

const char *filter_library_version(void);


/*
 * ============================================================
 * C++ Compatibility
 * ============================================================
 */

#ifdef __cplusplus
}
#endif

#endif /* LIBFILTER_H */
