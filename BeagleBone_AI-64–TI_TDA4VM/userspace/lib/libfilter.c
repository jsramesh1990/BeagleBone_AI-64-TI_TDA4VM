/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter
 *
 * Userspace Filter Library
 *
 * File:
 *     userspace/lib/libfilter.c
 *
 * Purpose:
 *     Userspace library used by applications and utilities to
 *     communicate with the kernel packet-filter driver through
 *     /dev/packet_filter and ioctl().
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
 *     Kernel Packet Filter
 *
 * ============================================================
 */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/libfilter.h"

/*
 * Kernel ioctl definitions.
 *
 * Expected location:
 *
 *     kernel/packet_filter/ioctl_defs.h
 *
 * Adjust the include path if the project build system exposes
 * this header through an include directory.
 */
#include "../../kernel/packet_filter/ioctl_defs.h"


/*
 * ============================================================
 * Internal Library State
 * ============================================================
 */

static int filter_fd = -1;

static int filter_initialized = 0;

static filter_action_t default_action =
    FILTER_ACTION_ALLOW;

static int monitoring_enabled = 0;

static char active_interface[
    FILTER_MAX_INTERFACE_NAME] = "eth0";

static char active_rule_file[
    512] = "userspace/config/default_rules.conf";


/*
 * ============================================================
 * Internal Helpers
 * ============================================================
 */

static int check_initialized(void)
{
    if (!filter_initialized) {
        return FILTER_NOT_INITIALIZED;
    }

    return FILTER_SUCCESS;
}


static int check_rule(
        const filter_rule_t *rule)
{
    if (rule == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    if (rule->action != FILTER_ACTION_ALLOW &&
        rule->action != FILTER_ACTION_DROP &&
        rule->action != FILTER_ACTION_MONITOR) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (rule->protocol != FILTER_PROTOCOL_ANY &&
        rule->protocol != FILTER_PROTOCOL_TCP &&
        rule->protocol != FILTER_PROTOCOL_UDP &&
        rule->protocol != FILTER_PROTOCOL_ICMP) {

        return FILTER_INVALID_ARGUMENT;
    }

    return FILTER_SUCCESS;
}


/*
 * ============================================================
 * Initialization
 * ============================================================
 */

int filter_init(void)
{
    filter_config_t config;

    memset(
        &config,
        0,
        sizeof(config));

    config.device =
        FILTER_DEVICE_PATH;

    config.rule_file =
        active_rule_file;

    config.interface =
        active_interface;

    config.flags = 0;

    return filter_init_with_config(
        &config);
}


int filter_init_with_config(
        const filter_config_t *config)
{
    int ret;

    if (config == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    if (filter_initialized) {
        return FILTER_SUCCESS;
    }

    if (config->device != NULL) {

        /*
         * Store only the known default device path.
         *
         * The current public API does not expose a dynamic
         * device-path buffer.
         */
    }

    if (config->interface != NULL) {

        strncpy(
            active_interface,
            config->interface,
            sizeof(active_interface) - 1);

        active_interface[
            sizeof(active_interface) - 1] = '\0';
    }

    if (config->rule_file != NULL) {

        strncpy(
            active_rule_file,
            config->rule_file,
            sizeof(active_rule_file) - 1);

        active_rule_file[
            sizeof(active_rule_file) - 1] = '\0';
    }

    ret = filter_open();

    if (ret < 0) {
        return ret;
    }

    filter_initialized = 1;

    return FILTER_SUCCESS;
}


void filter_cleanup(void)
{
    if (!filter_initialized) {
        return;
    }

    filter_close();

    filter_initialized = 0;
}


/*
 * ============================================================
 * Device Operations
 * ============================================================
 */

int filter_open(void)
{
    if (filter_fd >= 0) {
        return filter_fd;
    }

    filter_fd =
        open(
            FILTER_DEVICE_PATH,
            O_RDWR | O_CLOEXEC);

    if (filter_fd < 0) {

        perror(
            "packet_filter: open");

        return FILTER_DEVICE_ERROR;
    }

    return filter_fd;
}


void filter_close(void)
{
    if (filter_fd >= 0) {

        close(filter_fd);

        filter_fd = -1;
    }
}


int filter_is_open(void)
{
    return filter_fd >= 0;
}


/*
 * ============================================================
 * Rule Operations
 * ============================================================
 */

int filter_add_rule(
        const filter_rule_t *rule)
{
    int ret;

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    ret = check_rule(rule);

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_ADD_RULE

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_ADD_RULE,
            rule) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    /*
     * Compatibility fallback.
     *
     * If ioctl_defs.h uses another command name, map it here.
     */
    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


int filter_remove_rule(
        uint32_t rule_id)
{
    int ret;

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_REMOVE_RULE

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_REMOVE_RULE,
            &rule_id) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


int filter_update_rule(
        const filter_rule_t *rule)
{
    int ret;

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    ret = check_rule(rule);

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_UPDATE_RULE

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_UPDATE_RULE,
            rule) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


int filter_get_rule(
        uint32_t rule_id,
        filter_rule_t *rule)
{
    int ret;

    if (rule == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    memset(
        rule,
        0,
        sizeof(*rule));

    /*
     * Use a temporary rule object because most kernel ioctl
     * implementations use the complete structure for lookup.
     */
    rule->rule_id = rule_id;

#ifdef PACKET_FILTER_IOCTL_GET_RULE

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_GET_RULE,
            rule) < 0) {

        if (errno == ENOENT) {
            return FILTER_NOT_FOUND;
        }

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


int filter_get_rules(
        filter_rule_t *rules,
        uint32_t *count)
{
    int ret;

    if (rules == NULL ||
        count == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (*count == 0) {
        return FILTER_INVALID_ARGUMENT;
    }

    if (*count > FILTER_MAX_RULES) {
        *count = FILTER_MAX_RULES;
    }

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_GET_RULES

    /*
     * The exact ioctl ABI should define whether the kernel
     * expects an array directly or a wrapper structure.
     *
     * The current project API assumes a wrapper.
     */

    struct {
        filter_rule_t *rules;
        uint32_t count;
    } request;

    request.rules = rules;
    request.count = *count;

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_GET_RULES,
            &request) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    *count =
        request.count;

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


int filter_clear_rules(void)
{
    int ret;

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_CLEAR_RULES

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_CLEAR_RULES) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


int filter_enable_rule(
        uint32_t rule_id)
{
    int ret;

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_ENABLE_RULE

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_ENABLE_RULE,
            &rule_id) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


int filter_disable_rule(
        uint32_t rule_id)
{
    int ret;

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_DISABLE_RULE

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_DISABLE_RULE,
            &rule_id) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


/*
 * ============================================================
 * Rule File Operations
 * ============================================================
 */

int filter_load_rules(
        const char *filename)
{
    FILE *file;

    char line[512];

    unsigned int line_number = 0;

    int loaded = 0;

    if (filename == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    if (!filter_initialized) {
        return FILTER_NOT_INITIALIZED;
    }

    file =
        fopen(
            filename,
            "r");

    if (file == NULL) {
        return FILTER_ERROR;
    }

    while (fgets(
                line,
                sizeof(line),
                file) != NULL) {

        char action[32];
        char src_ip[64];
        char dst_ip[64];
        char protocol[32];

        unsigned int src_port;
        unsigned int dst_port;

        int fields;

        line_number++;

        /*
         * Skip comments.
         */

        if (line[0] == '#' ||
            line[0] == '\n' ||
            line[0] == '\0') {

            continue;
        }

        fields =
            sscanf(
                line,
                "%31s %63s %63s %u %u %31s",
                action,
                src_ip,
                dst_ip,
                &src_port,
                &dst_port,
                protocol);

        if (fields != 6) {

            fprintf(
                stderr,
                "packet_filter: invalid rule "
                "at line %u\n",
                line_number);

            continue;
        }

        filter_rule_t rule;

        filter_rule_init(
            &rule);

        /*
         * Action.
         */

        if (strcmp(
                action,
                "ALLOW") == 0) {

            rule.action =
                FILTER_ACTION_ALLOW;

        } else if (strcmp(
                action,
                "DROP") == 0) {

            rule.action =
                FILTER_ACTION_DROP;

        } else if (strcmp(
                action,
                "MONITOR") == 0) {

            rule.action =
                FILTER_ACTION_MONITOR;

        } else {

            fprintf(
                stderr,
                "packet_filter: unknown action "
                "at line %u\n",
                line_number);

            continue;
        }

        /*
         * Source IP.
         *
         * 0.0.0.0 means wildcard.
         */

        if (strcmp(
                src_ip,
                "0.0.0.0") != 0) {

            if (filter_parse_ip(
                    src_ip,
                    &rule.src_ip) !=
                FILTER_SUCCESS) {

                fprintf(
                    stderr,
                    "packet_filter: invalid source IP "
                    "at line %u\n",
                    line_number);

                continue;
            }

            rule.src_ip_mask =
                0xffffffffU;
        }

        /*
         * Destination IP.
         */

        if (strcmp(
                dst_ip,
                "0.0.0.0") != 0) {

            if (filter_parse_ip(
                    dst_ip,
                    &rule.dst_ip) !=
                FILTER_SUCCESS) {

                fprintf(
                    stderr,
                    "packet_filter: invalid destination IP "
                    "at line %u\n",
                    line_number);

                continue;
            }

            rule.dst_ip_mask =
                0xffffffffU;
        }

        /*
         * Ports.
         */

        if (src_port > 65535 ||
            dst_port > 65535) {

            fprintf(
                stderr,
                "packet_filter: invalid port "
                "at line %u\n",
                line_number);

            continue;
        }

        rule.src_port =
            (uint16_t)src_port;

        rule.dst_port =
            (uint16_t)dst_port;

        /*
         * Protocol.
         */

        if (strcmp(
                protocol,
                "TCP") == 0) {

            rule.protocol =
                FILTER_PROTOCOL_TCP;

        } else if (strcmp(
                protocol,
                "UDP") == 0) {

            rule.protocol =
                FILTER_PROTOCOL_UDP;

        } else if (strcmp(
                protocol,
                "ICMP") == 0) {

            rule.protocol =
                FILTER_PROTOCOL_ICMP;

        } else if (strcmp(
                protocol,
                "ANY") == 0) {

            rule.protocol =
                FILTER_PROTOCOL_ANY;

        } else {

            fprintf(
                stderr,
                "packet_filter: unknown protocol "
                "at line %u\n",
                line_number);

            continue;
        }

        if (filter_add_rule(
                &rule) == FILTER_SUCCESS) {

            loaded++;
        }
    }

    fclose(file);

    strncpy(
        active_rule_file,
        filename,
        sizeof(active_rule_file) - 1);

    active_rule_file[
        sizeof(active_rule_file) - 1] = '\0';

    return loaded;
}


int filter_save_rules(
        const char *filename)
{
    FILE *file;

    filter_rule_t rules[FILTER_MAX_RULES];

    uint32_t count =
        FILTER_MAX_RULES;

    uint32_t i;

    int ret;

    if (filename == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    file =
        fopen(
            filename,
            "w");

    if (file == NULL) {
        return FILTER_ERROR;
    }

    ret =
        filter_get_rules(
            rules,
            &count);

    if (ret != FILTER_SUCCESS) {

        fclose(file);

        return ret;
    }

    fprintf(
        file,
        "# BeagleBone AI-64 Packet Filter Rules\n");

    fprintf(
        file,
        "# ACTION SRC_IP DST_IP SRC_PORT DST_PORT PROTOCOL\n\n");

    for (i = 0; i < count; i++) {

        char src_ip[INET_ADDRSTRLEN];
        char dst_ip[INET_ADDRSTRLEN];

        filter_format_ip(
            rules[i].src_ip,
            src_ip,
            sizeof(src_ip));

        filter_format_ip(
            rules[i].dst_ip,
            dst_ip,
            sizeof(dst_ip));

        fprintf(
            file,
            "%s %s %s %u %u %s\n",
            filter_action_to_string(
                (filter_action_t)rules[i].action),
            src_ip,
            dst_ip,
            rules[i].src_port,
            rules[i].dst_port,
            filter_protocol_to_string(
                rules[i].protocol));
    }

    fclose(file);

    return FILTER_SUCCESS;
}


int filter_reload_rules(void)
{
    int ret;

    ret = filter_clear_rules();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    return filter_load_rules(
        active_rule_file);
}


/*
 * ============================================================
 * Statistics
 * ============================================================
 */

int filter_get_statistics(
        filter_statistics_t *statistics)
{
    int ret;

    if (statistics == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    memset(
        statistics,
        0,
        sizeof(*statistics));

#ifdef PACKET_FILTER_IOCTL_GET_STATS

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_GET_STATS,
            statistics) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


int filter_reset_statistics(void)
{
    int ret;

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_RESET_STATS

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_RESET_STATS) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


int filter_get_interface_statistics(
        const char *interface,
        filter_interface_stats_t *statistics)
{
    int ret;

    if (interface == NULL ||
        statistics == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    memset(
        statistics,
        0,
        sizeof(*statistics));

    strncpy(
        statistics->interface_name,
        interface,
        sizeof(statistics->interface_name) - 1);

#ifdef PACKET_FILTER_IOCTL_GET_INTERFACE_STATS

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_GET_INTERFACE_STATS,
            statistics) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    errno = ENOTSUP;

    return FILTER_IOCTL_ERROR;

#endif
}


/*
 * ============================================================
 * Packet Operations
 * ============================================================
 */

int filter_process_packet(
        const filter_packet_info_t *packet,
        filter_decision_t *decision)
{
    int ret;

    if (packet == NULL ||
        decision == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    memset(
        decision,
        0,
        sizeof(*decision));

#ifdef PACKET_FILTER_IOCTL_PROCESS_PACKET

    struct {

        filter_packet_info_t packet;

        filter_decision_t decision;

    } request;

    memset(
        &request,
        0,
        sizeof(request));

    request.packet =
        *packet;

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_PROCESS_PACKET,
            &request) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    *decision =
        request.decision;

    return FILTER_SUCCESS;

#else

    /*
     * Userspace fallback.
     *
     * This does not replace kernel filtering.
     * It is useful for unit/integration testing.
     */

    *decision =
        (filter_decision_t) {
            .rule_id = 0,
            .action = default_action,
            .matched = 0,
            .reserved = 0
        };

    return FILTER_SUCCESS;

#endif
}


/*
 * ============================================================
 * Default Action
 * ============================================================
 */

int filter_set_default_action(
        filter_action_t action)
{
    if (action != FILTER_ACTION_ALLOW &&
        action != FILTER_ACTION_DROP &&
        action != FILTER_ACTION_MONITOR) {

        return FILTER_INVALID_ARGUMENT;
    }

    default_action =
        action;

#ifdef PACKET_FILTER_IOCTL_SET_DEFAULT_ACTION

    if (filter_initialized) {

        if (ioctl(
                filter_fd,
                PACKET_FILTER_IOCTL_SET_DEFAULT_ACTION,
                &action) < 0) {

            return FILTER_IOCTL_ERROR;
        }
    }

#endif

    return FILTER_SUCCESS;
}


int filter_get_default_action(
        filter_action_t *action)
{
    if (action == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    *action =
        default_action;

#ifdef PACKET_FILTER_IOCTL_GET_DEFAULT_ACTION

    if (filter_initialized) {

        if (ioctl(
                filter_fd,
                PACKET_FILTER_IOCTL_GET_DEFAULT_ACTION,
                action) < 0) {

            return FILTER_IOCTL_ERROR;
        }
    }

#endif

    return FILTER_SUCCESS;
}


/*
 * ============================================================
 * Network Interface
 * ============================================================
 */

int filter_set_interface(
        const char *interface)
{
    if (interface == NULL ||
        interface[0] == '\0') {

        return FILTER_INVALID_ARGUMENT;
    }

    if (strlen(interface) >=
        sizeof(active_interface)) {

        return FILTER_INVALID_ARGUMENT;
    }

    strcpy(
        active_interface,
        interface);

#ifdef PACKET_FILTER_IOCTL_SET_INTERFACE

    if (filter_initialized) {

        if (ioctl(
                filter_fd,
                PACKET_FILTER_IOCTL_SET_INTERFACE,
                active_interface) < 0) {

            return FILTER_IOCTL_ERROR;
        }
    }

#endif

    return FILTER_SUCCESS;
}


int filter_get_interface(
        char *interface,
        size_t size)
{
    if (interface == NULL ||
        size == 0) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (strlen(active_interface) + 1 >
        size) {

        return FILTER_BUFFER_TOO_SMALL;
    }

    strcpy(
        interface,
        active_interface);

    return FILTER_SUCCESS;
}


/*
 * ============================================================
 * Monitoring
 * ============================================================
 */

int filter_enable_monitoring(void)
{
    int ret;

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_ENABLE_MONITORING

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_ENABLE_MONITORING) < 0) {

        return FILTER_IOCTL_ERROR;
    }

#endif

    monitoring_enabled = 1;

    return FILTER_SUCCESS;
}


int filter_disable_monitoring(void)
{
    int ret;

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_DISABLE_MONITORING

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_DISABLE_MONITORING) < 0) {

        return FILTER_IOCTL_ERROR;
    }

#endif

    monitoring_enabled = 0;

    return FILTER_SUCCESS;
}


int filter_monitoring_enabled(void)
{
    return monitoring_enabled;
}


/*
 * ============================================================
 * Driver Information
 * ============================================================
 */

int filter_get_driver_version(
        char *version,
        size_t size)
{
    int ret;

    if (version == NULL ||
        size == 0) {

        return FILTER_INVALID_ARGUMENT;
    }

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    memset(
        version,
        0,
        size);

#ifdef PACKET_FILTER_IOCTL_GET_VERSION

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_GET_VERSION,
            version) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    version[size - 1] = '\0';

    return FILTER_SUCCESS;

#else

    snprintf(
        version,
        size,
        "packet-filter-1.0");

    return FILTER_SUCCESS;

#endif
}


int filter_get_api_version(
        uint32_t *major,
        uint32_t *minor)
{
    if (major == NULL ||
        minor == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    *major =
        FILTER_API_VERSION_MAJOR;

    *minor =
        FILTER_API_VERSION_MINOR;

    return FILTER_SUCCESS;
}


int filter_get_rule_count(
        uint32_t *count)
{
    int ret;

    if (count == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    ret = check_initialized();

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

#ifdef PACKET_FILTER_IOCTL_GET_RULE_COUNT

    if (ioctl(
            filter_fd,
            PACKET_FILTER_IOCTL_GET_RULE_COUNT,
            count) < 0) {

        return FILTER_IOCTL_ERROR;
    }

    return FILTER_SUCCESS;

#else

    /*
     * Fall back to reading the rule table.
     */

    filter_rule_t rules[FILTER_MAX_RULES];

    *count =
        FILTER_MAX_RULES;

    ret =
        filter_get_rules(
            rules,
            count);

    return ret;

#endif
}


/*
 * ============================================================
 * Utility Functions
 * ============================================================
 */

const char *filter_action_to_string(
        filter_action_t action)
{
    switch (action) {

    case FILTER_ACTION_ALLOW:
        return "ALLOW";

    case FILTER_ACTION_DROP:
        return "DROP";

    case FILTER_ACTION_MONITOR:
        return "MONITOR";

    default:
        return "UNKNOWN";
    }
}


const char *filter_protocol_to_string(
        uint8_t protocol)
{
    switch (protocol) {

    case FILTER_PROTOCOL_ANY:
        return "ANY";

    case FILTER_PROTOCOL_TCP:
        return "TCP";

    case FILTER_PROTOCOL_UDP:
        return "UDP";

    case FILTER_PROTOCOL_ICMP:
        return "ICMP";

    default:
        return "UNKNOWN";
    }
}


const char *filter_error_string(
        int error)
{
    switch (error) {

    case FILTER_SUCCESS:
        return "Success";

    case FILTER_ERROR:
        return "General error";

    case FILTER_INVALID_ARGUMENT:
        return "Invalid argument";

    case FILTER_DEVICE_ERROR:
        return "Device error";

    case FILTER_IOCTL_ERROR:
        return "IOCTL error";

    case FILTER_MEMORY_ERROR:
        return "Memory error";

    case FILTER_NOT_FOUND:
        return "Object not found";

    case FILTER_ALREADY_EXISTS:
        return "Object already exists";

    case FILTER_PERMISSION_ERROR:
        return "Permission denied";

    case FILTER_NOT_INITIALIZED:
        return "Library not initialized";

    case FILTER_BUFFER_TOO_SMALL:
        return "Buffer too small";

    default:
        return "Unknown error";
    }
}


int filter_parse_ip(
        const char *address,
        uint32_t *ip)
{
    struct in_addr addr;

    if (address == NULL ||
        ip == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (inet_pton(
            AF_INET,
            address,
            &addr) != 1) {

        return FILTER_INVALID_ARGUMENT;
    }

    *ip =
        addr.s_addr;

    return FILTER_SUCCESS;
}


int filter_format_ip(
        uint32_t ip,
        char *buffer,
        size_t size)
{
    struct in_addr addr;

    const char *result;

    if (buffer == NULL ||
        size == 0) {

        return FILTER_INVALID_ARGUMENT;
    }

    addr.s_addr =
        ip;

    result =
        inet_ntop(
            AF_INET,
            &addr,
            buffer,
            size);

    if (result == NULL) {
        return FILTER_ERROR;
    }

    return FILTER_SUCCESS;
}


/*
 * ============================================================
 * Rule Construction Helpers
 * ============================================================
 */

void filter_rule_init(
        filter_rule_t *rule)
{
    if (rule == NULL) {
        return;
    }

    memset(
        rule,
        0,
        sizeof(*rule));

    rule->protocol =
        FILTER_PROTOCOL_ANY;

    rule->action =
        FILTER_ACTION_ALLOW;

    rule->enabled =
        1;
}


int filter_rule_set_source_ip(
        filter_rule_t *rule,
        const char *ip)
{
    int ret;

    if (rule == NULL ||
        ip == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    ret =
        filter_parse_ip(
            ip,
            &rule->src_ip);

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    rule->src_ip_mask =
        0xffffffffU;

    return FILTER_SUCCESS;
}


int filter_rule_set_destination_ip(
        filter_rule_t *rule,
        const char *ip)
{
    int ret;

    if (rule == NULL ||
        ip == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    ret =
        filter_parse_ip(
            ip,
            &rule->dst_ip);

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    rule->dst_ip_mask =
        0xffffffffU;

    return FILTER_SUCCESS;
}


int filter_rule_set_source_network(
        filter_rule_t *rule,
        const char *network)
{
    char address[64];

    unsigned int prefix;

    char *slash;

    uint32_t ip;

    uint32_t mask;

    if (rule == NULL ||
        network == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (strlen(network) >=
        sizeof(address)) {

        return FILTER_INVALID_ARGUMENT;
    }

    strcpy(
        address,
        network);

    slash =
        strchr(
            address,
            '/');

    if (slash == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    *slash = '\0';

    prefix =
        (unsigned int)strtoul(
            slash + 1,
            NULL,
            10);

    if (prefix > 32) {
        return FILTER_INVALID_ARGUMENT;
    }

    if (filter_parse_ip(
            address,
            &ip) != FILTER_SUCCESS) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (prefix == 0) {

        mask = 0;

    } else {

        mask =
            htonl(
                0xffffffffU <<
                (32 - prefix));
    }

    rule->src_ip =
        ip & mask;

    rule->src_ip_mask =
        mask;

    return FILTER_SUCCESS;
}


int filter_rule_set_destination_network(
        filter_rule_t *rule,
        const char *network)
{
    char address[64];

    unsigned int prefix;

    char *slash;

    uint32_t ip;

    uint32_t mask;

    if (rule == NULL ||
        network == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (strlen(network) >=
        sizeof(address)) {

        return FILTER_INVALID_ARGUMENT;
    }

    strcpy(
        address,
        network);

    slash =
        strchr(
            address,
            '/');

    if (slash == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    *slash = '\0';

    prefix =
        (unsigned int)strtoul(
            slash + 1,
            NULL,
            10);

    if (prefix > 32) {
        return FILTER_INVALID_ARGUMENT;
    }

    if (filter_parse_ip(
            address,
            &ip) != FILTER_SUCCESS) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (prefix == 0) {

        mask = 0;

    } else {

        mask =
            htonl(
                0xffffffffU <<
                (32 - prefix));
    }

    rule->dst_ip =
        ip & mask;

    rule->dst_ip_mask =
        mask;

    return FILTER_SUCCESS;
}


int filter_rule_set_ports(
        filter_rule_t *rule,
        uint16_t source_port,
        uint16_t destination_port)
{
    if (rule == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    rule->src_port =
        source_port;

    rule->dst_port =
        destination_port;

    return FILTER_SUCCESS;
}


int filter_rule_set_protocol(
        filter_rule_t *rule,
        filter_protocol_t protocol)
{
    if (rule == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    if (protocol != FILTER_PROTOCOL_ANY &&
        protocol != FILTER_PROTOCOL_TCP &&
        protocol != FILTER_PROTOCOL_UDP &&
        protocol != FILTER_PROTOCOL_ICMP) {

        return FILTER_INVALID_ARGUMENT;
    }

    rule->protocol =
        protocol;

    return FILTER_SUCCESS;
}


int filter_rule_set_action(
        filter_rule_t *rule,
        filter_action_t action)
{
    if (rule == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    if (action != FILTER_ACTION_ALLOW &&
        action != FILTER_ACTION_DROP &&
        action != FILTER_ACTION_MONITOR) {

        return FILTER_INVALID_ARGUMENT;
    }

    rule->action =
        action;

    return FILTER_SUCCESS;
}


int filter_rule_set_priority(
        filter_rule_t *rule,
        uint16_t priority)
{
    if (rule == NULL) {
        return FILTER_INVALID_ARGUMENT;
    }

    rule->priority =
        priority;

    return FILTER_SUCCESS;
}


/*
 * ============================================================
 * Debugging
 * ============================================================
 */

void filter_dump_rule(
        const filter_rule_t *rule)
{
    char src_ip[INET_ADDRSTRLEN];

    char dst_ip[INET_ADDRSTRLEN];

    if (rule == NULL) {
        return;
    }

    filter_format_ip(
        rule->src_ip,
        src_ip,
        sizeof(src_ip));

    filter_format_ip(
        rule->dst_ip,
        dst_ip,
        sizeof(dst_ip));

    printf(
        "Rule:\n"
        "  ID       : %u\n"
        "  Source   : %s:%u\n"
        "  Dest     : %s:%u\n"
        "  Protocol : %s\n"
        "  Action   : %s\n"
        "  Priority : %u\n"
        "  Enabled  : %u\n",
        rule->rule_id,
        src_ip,
        rule->src_port,
        dst_ip,
        rule->dst_port,
        filter_protocol_to_string(
            rule->protocol),
        filter_action_to_string(
            (filter_action_t)rule->action),
        rule->priority,
        rule->enabled);
}


void filter_dump_statistics(
        const filter_statistics_t *statistics)
{
    if (statistics == NULL) {
        return;
    }

    printf(
        "\n"
        "================ PACKET FILTER STATISTICS ================\n"
        "Packets Total      : %llu\n"
        "Packets Allowed    : %llu\n"
        "Packets Dropped    : %llu\n"
        "Packets Monitored  : %llu\n"
        "Bytes Total        : %llu\n"
        "Bytes Allowed      : %llu\n"
        "Bytes Dropped      : %llu\n"
        "Bytes Monitored    : %llu\n"
        "Rule Matches       : %llu\n"
        "Parser Errors      : %llu\n"
        "Malformed Packets  : %llu\n"
        "============================================================\n",
        (unsigned long long)
            statistics->packets_total,

        (unsigned long long)
            statistics->packets_allowed,

        (unsigned long long)
            statistics->packets_dropped,

        (unsigned long long)
            statistics->packets_monitored,

        (unsigned long long)
            statistics->bytes_total,

        (unsigned long long)
            statistics->bytes_allowed,

        (unsigned long long)
            statistics->bytes_dropped,

        (unsigned long long)
            statistics->bytes_monitored,

        (unsigned long long)
            statistics->rule_matches,

        (unsigned long long)
            statistics->parser_errors,

        (unsigned long long)
            statistics->malformed_packets);
}


/*
 * ============================================================
 * Library Version
 * ============================================================
 */

const char *filter_library_version(void)
{
    return FILTER_API_VERSION_STRING;
}
