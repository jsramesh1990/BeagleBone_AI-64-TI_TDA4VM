/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter
 *
 * Userspace Control Utility
 *
 * File:
 *     userspace/tools/filter_ctl.c
 *
 * Purpose:
 *     Command-line utility used to configure, monitor,
 *     inspect and test the packet-filter kernel driver.
 *
 * Usage:
 *
 *     filter_ctl <command> [options]
 *
 * Commands:
 *
 *     status
 *     version
 *     add
 *     delete
 *     list
 *     clear
 *     enable
 *     disable
 *     load
 *     reload
 *     save
 *     stats
 *     reset-stats
 *     monitor
 *     interface
 *     default-action
 *
 * ============================================================
 */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/libfilter.h"


/*
 * ============================================================
 * Utility
 * ============================================================
 */

static void print_separator(void)
{
    printf(
        "------------------------------------------------------------\n");
}


static void print_banner(void)
{
    printf(
        "\n"
        "============================================================\n"
        " BeagleBone AI-64 - TI TDA4VM\n"
        " Packet Filter Control Utility\n"
        "============================================================\n");
}


static void print_error(
        const char *operation,
        int error)
{
    fprintf(
        stderr,
        "ERROR: %s: %s (%d)\n",
        operation,
        filter_error_string(error),
        error);
}


/*
 * ============================================================
 * Usage
 * ============================================================
 */

static void print_usage(
        const char *program)
{
    printf(
        "\n"
        "Usage:\n"
        "  %s <command> [options]\n"
        "\n"

        "Commands:\n"
        "\n"

        "  status\n"
        "      Show packet-filter status.\n"
        "\n"

        "  version\n"
        "      Show library and driver versions.\n"
        "\n"

        "  add\n"
        "      Add a packet-filter rule.\n"
        "\n"

        "  delete <rule-id>\n"
        "      Delete a rule.\n"
        "\n"

        "  list\n"
        "      Display all configured rules.\n"
        "\n"

        "  clear\n"
        "      Remove all configured rules.\n"
        "\n"

        "  enable <rule-id>\n"
        "      Enable a rule.\n"
        "\n"

        "  disable <rule-id>\n"
        "      Disable a rule.\n"
        "\n"

        "  load <file>\n"
        "      Load rules from a configuration file.\n"
        "\n"

        "  reload\n"
        "      Reload the configured rule file.\n"
        "\n"

        "  save <file>\n"
        "      Save current rules to a configuration file.\n"
        "\n"

        "  stats\n"
        "      Display packet statistics.\n"
        "\n"

        "  reset-stats\n"
        "      Reset packet statistics.\n"
        "\n"

        "  monitor on|off\n"
        "      Enable or disable monitoring.\n"
        "\n"

        "  interface [name]\n"
        "      Show or configure network interface.\n"
        "\n"

        "  default-action [allow|drop|monitor]\n"
        "      Show or configure default action.\n"
        "\n"

        "Add rule options:\n"
        "\n"

        "  --src-ip <address>\n"
        "  --dst-ip <address>\n"
        "  --src-net <network/prefix>\n"
        "  --dst-net <network/prefix>\n"
        "  --src-port <port>\n"
        "  --dst-port <port>\n"
        "  --protocol <tcp|udp|icmp|any>\n"
        "  --action <allow|drop|monitor>\n"
        "  --priority <number>\n"
        "\n"

        "Examples:\n"
        "\n"

        "  %s status\n"
        "\n"

        "  %s list\n"
        "\n"

        "  %s add \\\n"
        "      --src-ip 192.168.1.100 \\\n"
        "      --dst-ip 192.168.1.1 \\\n"
        "      --src-port 12345 \\\n"
        "      --dst-port 22 \\\n"
        "      --protocol tcp \\\n"
        "      --action allow\n"
        "\n"

        "  %s add \\\n"
        "      --src-net 192.168.10.0/24 \\\n"
        "      --dst-port 80 \\\n"
        "      --protocol tcp \\\n"
        "      --action monitor\n"
        "\n"

        "  %s delete 5\n"
        "\n"

        "  %s load userspace/config/default_rules.conf\n"
        "\n"

        "  %s stats\n"
        "\n"

        "  %s monitor on\n"
        "\n",

        program,
        program,
        program,
        program,
        program,
        program,
        program,
        program);
}


/*
 * ============================================================
 * Parse Action
 * ============================================================
 */

static int parse_action(
        const char *text,
        filter_action_t *action)
{
    if (text == NULL ||
        action == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (strcasecmp(
            text,
            "allow") == 0) {

        *action =
            FILTER_ACTION_ALLOW;

        return FILTER_SUCCESS;
    }

    if (strcasecmp(
            text,
            "drop") == 0) {

        *action =
            FILTER_ACTION_DROP;

        return FILTER_SUCCESS;
    }

    if (strcasecmp(
            text,
            "monitor") == 0) {

        *action =
            FILTER_ACTION_MONITOR;

        return FILTER_SUCCESS;
    }

    return FILTER_INVALID_ARGUMENT;
}


/*
 * ============================================================
 * Parse Protocol
 * ============================================================
 */

static int parse_protocol(
        const char *text,
        filter_protocol_t *protocol)
{
    if (text == NULL ||
        protocol == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    if (strcasecmp(
            text,
            "tcp") == 0) {

        *protocol =
            FILTER_PROTOCOL_TCP;

        return FILTER_SUCCESS;
    }

    if (strcasecmp(
            text,
            "udp") == 0) {

        *protocol =
            FILTER_PROTOCOL_UDP;

        return FILTER_SUCCESS;
    }

    if (strcasecmp(
            text,
            "icmp") == 0) {

        *protocol =
            FILTER_PROTOCOL_ICMP;

        return FILTER_SUCCESS;
    }

    if (strcasecmp(
            text,
            "any") == 0) {

        *protocol =
            FILTER_PROTOCOL_ANY;

        return FILTER_SUCCESS;
    }

    return FILTER_INVALID_ARGUMENT;
}


/*
 * ============================================================
 * Parse Number
 * ============================================================
 */

static int parse_uint32(
        const char *text,
        uint32_t *value)
{
    char *end;

    unsigned long number;

    if (text == NULL ||
        value == NULL) {

        return FILTER_INVALID_ARGUMENT;
    }

    errno = 0;

    number =
        strtoul(
            text,
            &end,
            10);

    if (errno != 0 ||
        end == text ||
        *end != '\0' ||
        number > UINT32_MAX) {

        return FILTER_INVALID_ARGUMENT;
    }

    *value =
        (uint32_t)number;

    return FILTER_SUCCESS;
}


static int parse_port(
        const char *text,
        uint16_t *port)
{
    uint32_t value;

    int ret;

    ret =
        parse_uint32(
            text,
            &value);

    if (ret != FILTER_SUCCESS) {
        return ret;
    }

    if (value > 65535) {
        return FILTER_INVALID_ARGUMENT;
    }

    *port =
        (uint16_t)value;

    return FILTER_SUCCESS;
}


/*
 * ============================================================
 * Status
 * ============================================================
 */

static int command_status(void)
{
    uint32_t count = 0;

    filter_action_t action;

    char interface[
        FILTER_MAX_INTERFACE_NAME];

    int ret;

    printf(
        "\nPacket Filter Status\n");

    print_separator();

    printf(
        "Device      : %s\n",
        filter_is_open()
            ? FILTER_DEVICE_PATH
            : "CLOSED");

    ret =
        filter_get_rule_count(
            &count);

    if (ret == FILTER_SUCCESS) {

        printf(
            "Rule Count  : %u\n",
            count);

    } else {

        printf(
            "Rule Count  : unavailable\n");
    }

    ret =
        filter_get_interface(
            interface,
            sizeof(interface));

    if (ret == FILTER_SUCCESS) {

        printf(
            "Interface   : %s\n",
            interface);
    }

    ret =
        filter_get_default_action(
            &action);

    if (ret == FILTER_SUCCESS) {

        printf(
            "Default     : %s\n",
            filter_action_to_string(
                action));
    }

    printf(
        "Monitoring  : %s\n",
        filter_monitoring_enabled()
            ? "ENABLED"
            : "DISABLED");

    return 0;
}


/*
 * ============================================================
 * Version
 * ============================================================
 */

static int command_version(void)
{
    char driver_version[128];

    uint32_t major;

    uint32_t minor;

    int ret;

    printf(
        "\nPacket Filter Version\n");

    print_separator();

    printf(
        "Library Version : %s\n",
        filter_library_version());

    ret =
        filter_get_api_version(
            &major,
            &minor);

    if (ret == FILTER_SUCCESS) {

        printf(
            "API Version     : %u.%u\n",
            major,
            minor);
    }

    ret =
        filter_get_driver_version(
            driver_version,
            sizeof(driver_version));

    if (ret == FILTER_SUCCESS) {

        printf(
            "Driver Version  : %s\n",
            driver_version);

    } else {

        printf(
            "Driver Version  : unavailable\n");
    }

    return 0;
}


/*
 * ============================================================
 * Add Rule
 * ============================================================
 */

static int command_add(
        int argc,
        char **argv)
{
    filter_rule_t rule;

    int option;

    int option_index = 0;

    int ret;

    static struct option long_options[] = {

        {
            "src-ip",
            required_argument,
            0,
            's'
        },

        {
            "dst-ip",
            required_argument,
            0,
            'd'
        },

        {
            "src-net",
            required_argument,
            0,
            'S'
        },

        {
            "dst-net",
            required_argument,
            0,
            'D'
        },

        {
            "src-port",
            required_argument,
            0,
            'p'
        },

        {
            "dst-port",
            required_argument,
            0,
            'P'
        },

        {
            "protocol",
            required_argument,
            0,
            'r'
        },

        {
            "action",
            required_argument,
            0,
            'a'
        },

        {
            "priority",
            required_argument,
            0,
            'o'
        },

        {
            0,
            0,
            0,
            0
        }
    };

    filter_rule_init(
        &rule);

    optind = 1;

    while ((option =
            getopt_long(
                argc,
                argv,
                "s:d:S:D:p:P:r:a:o:",
                long_options,
                &option_index)) != -1) {

        switch (option) {

        case 's':

            ret =
                filter_rule_set_source_ip(
                    &rule,
                    optarg);

            if (ret != FILTER_SUCCESS) {

                print_error(
                    "invalid source IP",
                    ret);

                return 1;
            }

            break;

        case 'd':

            ret =
                filter_rule_set_destination_ip(
                    &rule,
                    optarg);

            if (ret != FILTER_SUCCESS) {

                print_error(
                    "invalid destination IP",
                    ret);

                return 1;
            }

            break;

        case 'S':

            ret =
                filter_rule_set_source_network(
                    &rule,
                    optarg);

            if (ret != FILTER_SUCCESS) {

                print_error(
                    "invalid source network",
                    ret);

                return 1;
            }

            break;

        case 'D':

            ret =
                filter_rule_set_destination_network(
                    &rule,
                    optarg);

            if (ret != FILTER_SUCCESS) {

                print_error(
                    "invalid destination network",
                    ret);

                return 1;
            }

            break;

        case 'p':

            ret =
                parse_port(
                    optarg,
                    &rule.src_port);

            if (ret != FILTER_SUCCESS) {

                print_error(
                    "invalid source port",
                    ret);

                return 1;
            }

            break;

        case 'P':

            ret =
                parse_port(
                    optarg,
                    &rule.dst_port);

            if (ret != FILTER_SUCCESS) {

                print_error(
                    "invalid destination port",
                    ret);

                return 1;
            }

            break;

        case 'r': {

            filter_protocol_t protocol;

            ret =
                parse_protocol(
                    optarg,
                    &protocol);

            if (ret != FILTER_SUCCESS) {

                print_error(
                    "invalid protocol",
                    ret);

                return 1;
            }

            rule.protocol =
                protocol;

            break;
        }

        case 'a': {

            filter_action_t action;

            ret =
                parse_action(
                    optarg,
                    &action);

            if (ret != FILTER_SUCCESS) {

                print_error(
                    "invalid action",
                    ret);

                return 1;
            }

            rule.action =
                action;

            break;
        }

        case 'o': {

            uint32_t priority;

            ret =
                parse_uint32(
                    optarg,
                    &priority);

            if (ret != FILTER_SUCCESS ||
                priority > 65535) {

                fprintf(
                    stderr,
                    "ERROR: invalid priority\n");

                return 1;
            }

            rule.priority =
                (uint16_t)priority;

            break;
        }

        default:

            print_usage(
                argv[0]);

            return 1;
        }
    }

    ret =
        filter_add_rule(
            &rule);

    if (ret != FILTER_SUCCESS) {

        print_error(
            "adding rule",
            ret);

        return 1;
    }

    printf(
        "Rule added successfully.\n");

    filter_dump_rule(
        &rule);

    return 0;
}


/*
 * ============================================================
 * Delete Rule
 * ============================================================
 */

static int command_delete(
        const char *argument)
{
    uint32_t rule_id;

    int ret;

    ret =
        parse_uint32(
            argument,
            &rule_id);

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "ERROR: invalid rule ID\n");

        return 1;
    }

    ret =
        filter_remove_rule(
            rule_id);

    if (ret != FILTER_SUCCESS) {

        print_error(
            "deleting rule",
            ret);

        return 1;
    }

    printf(
        "Rule %u deleted successfully.\n",
        rule_id);

    return 0;
}


/*
 * ============================================================
 * List Rules
 * ============================================================
 */

static int command_list(void)
{
    filter_rule_t rules[
        FILTER_MAX_RULES];

    uint32_t count =
        FILTER_MAX_RULES;

    uint32_t i;

    int ret;

    ret =
        filter_get_rules(
            rules,
            &count);

    if (ret != FILTER_SUCCESS) {

        print_error(
            "getting rules",
            ret);

        return 1;
    }

    printf(
        "\nConfigured Packet Filter Rules\n");

    print_separator();

    printf(
        "%-5s %-18s %-18s %-8s %-8s %-10s %-9s %-8s\n",
        "ID",
        "SOURCE",
        "DESTINATION",
        "S.PORT",
        "D.PORT",
        "PROTOCOL",
        "ACTION",
        "STATUS");

    print_separator();

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

        printf(
            "%-5u %-18s %-18s %-8u %-8u %-10s %-9s %-8s\n",

            rules[i].rule_id,

            src_ip,

            dst_ip,

            rules[i].src_port,

            rules[i].dst_port,

            filter_protocol_to_string(
                rules[i].protocol),

            filter_action_to_string(
                (filter_action_t)
                    rules[i].action),

            rules[i].enabled
                ? "ENABLED"
                : "DISABLED");
    }

    print_separator();

    printf(
        "Total rules: %u\n",
        count);

    return 0;
}


/*
 * ============================================================
 * Clear Rules
 * ============================================================
 */

static int command_clear(void)
{
    int ret;

    ret =
        filter_clear_rules();

    if (ret != FILTER_SUCCESS) {

        print_error(
            "clearing rules",
            ret);

        return 1;
    }

    printf(
        "All packet-filter rules cleared.\n");

    return 0;
}


/*
 * ============================================================
 * Enable / Disable
 * ============================================================
 */

static int command_enable(
        const char *argument)
{
    uint32_t rule_id;

    int ret;

    ret =
        parse_uint32(
            argument,
            &rule_id);

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "ERROR: invalid rule ID\n");

        return 1;
    }

    ret =
        filter_enable_rule(
            rule_id);

    if (ret != FILTER_SUCCESS) {

        print_error(
            "enabling rule",
            ret);

        return 1;
    }

    printf(
        "Rule %u enabled.\n",
        rule_id);

    return 0;
}


static int command_disable(
        const char *argument)
{
    uint32_t rule_id;

    int ret;

    ret =
        parse_uint32(
            argument,
            &rule_id);

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "ERROR: invalid rule ID\n");

        return 1;
    }

    ret =
        filter_disable_rule(
            rule_id);

    if (ret != FILTER_SUCCESS) {

        print_error(
            "disabling rule",
            ret);

        return 1;
    }

    printf(
        "Rule %u disabled.\n",
        rule_id);

    return 0;
}


/*
 * ============================================================
 * Load
 * ============================================================
 */

static int command_load(
        const char *filename)
{
    int ret;

    ret =
        filter_load_rules(
            filename);

    if (ret < 0) {

        print_error(
            "loading rules",
            ret);

        return 1;
    }

    printf(
        "Loaded %d rules from %s\n",
        ret,
        filename);

    return 0;
}


/*
 * ============================================================
 * Reload
 * ============================================================
 */

static int command_reload(void)
{
    int ret;

    ret =
        filter_reload_rules();

    if (ret < 0) {

        print_error(
            "reloading rules",
            ret);

        return 1;
    }

    printf(
        "Reloaded %d rules.\n",
        ret);

    return 0;
}


/*
 * ============================================================
 * Save
 * ============================================================
 */

static int command_save(
        const char *filename)
{
    int ret;

    ret =
        filter_save_rules(
            filename);

    if (ret != FILTER_SUCCESS) {

        print_error(
            "saving rules",
            ret);

        return 1;
    }

    printf(
        "Rules saved to %s\n",
        filename);

    return 0;
}


/*
 * ============================================================
 * Statistics
 * ============================================================
 */

static int command_stats(void)
{
    filter_statistics_t statistics;

    int ret;

    ret =
        filter_get_statistics(
            &statistics);

    if (ret != FILTER_SUCCESS) {

        print_error(
            "getting statistics",
            ret);

        return 1;
    }

    filter_dump_statistics(
        &statistics);

    return 0;
}


/*
 * ============================================================
 * Reset Statistics
 * ============================================================
 */

static int command_reset_stats(void)
{
    int ret;

    ret =
        filter_reset_statistics();

    if (ret != FILTER_SUCCESS) {

        print_error(
            "resetting statistics",
            ret);

        return 1;
    }

    printf(
        "Packet-filter statistics reset.\n");

    return 0;
}


/*
 * ============================================================
 * Monitoring
 * ============================================================
 */

static int command_monitor(
        const char *argument)
{
    int ret;

    if (strcasecmp(
            argument,
            "on") == 0) {

        ret =
            filter_enable_monitoring();

        if (ret != FILTER_SUCCESS) {

            print_error(
                "enabling monitoring",
                ret);

            return 1;
        }

        printf(
            "Packet monitoring enabled.\n");

        return 0;
    }

    if (strcasecmp(
            argument,
            "off") == 0) {

        ret =
            filter_disable_monitoring();

        if (ret != FILTER_SUCCESS) {

            print_error(
                "disabling monitoring",
                ret);

            return 1;
        }

        printf(
            "Packet monitoring disabled.\n");

        return 0;
    }

    fprintf(
        stderr,
        "ERROR: use 'monitor on' or "
        "'monitor off'\n");

    return 1;
}


/*
 * ============================================================
 * Interface
 * ============================================================
 */

static int command_interface(
        const char *argument)
{
    char interface[
        FILTER_MAX_INTERFACE_NAME];

    int ret;

    if (argument == NULL) {

        ret =
            filter_get_interface(
                interface,
                sizeof(interface));

        if (ret != FILTER_SUCCESS) {

            print_error(
                "getting interface",
                ret);

            return 1;
        }

        printf(
            "Active interface: %s\n",
            interface);

        return 0;
    }

    ret =
        filter_set_interface(
            argument);

    if (ret != FILTER_SUCCESS) {

        print_error(
            "setting interface",
            ret);

        return 1;
    }

    printf(
        "Active interface set to: %s\n",
        argument);

    return 0;
}


/*
 * ============================================================
 * Default Action
 * ============================================================
 */

static int command_default_action(
        const char *argument)
{
    filter_action_t action;

    int ret;

    if (argument == NULL) {

        ret =
            filter_get_default_action(
                &action);

        if (ret != FILTER_SUCCESS) {

            print_error(
                "getting default action",
                ret);

            return 1;
        }

        printf(
            "Default action: %s\n",
            filter_action_to_string(
                action));

        return 0;
    }

    ret =
        parse_action(
            argument,
            &action);

    if (ret != FILTER_SUCCESS) {

        fprintf(
            stderr,
            "ERROR: action must be "
            "allow, drop or monitor\n");

        return 1;
    }

    ret =
        filter_set_default_action(
            action);

    if (ret != FILTER_SUCCESS) {

        print_error(
            "setting default action",
            ret);

        return 1;
    }

    printf(
        "Default action set to: %s\n",
        filter_action_to_string(
            action));

    return 0;
}


/*
 * ============================================================
 * Main
 * ============================================================
 */

int main(
        int argc,
        char **argv)
{
    int ret;

    if (argc < 2) {

        print_banner();

        print_usage(
            argv[0]);

        return 1;
    }

    /*
     * Initialize userspace library.
     */

    ret =
        filter_init();

    if (ret != FILTER_SUCCESS) {

        print_error(
            "initializing packet filter",
            ret);

        fprintf(
            stderr,
            "\n"
            "Make sure the kernel driver is loaded:\n"
            "\n"
            "    sudo ./scripts/load_driver.sh\n"
            "\n"
            "and verify:\n"
            "\n"
            "    ls -l /dev/packet_filter\n"
            "\n");

        return 1;
    }

    /*
     * Dispatch command.
     */

    if (strcmp(
            argv[1],
            "status") == 0) {

        ret =
            command_status();

    } else if (strcmp(
            argv[1],
            "version") == 0) {

        ret =
            command_version();

    } else if (strcmp(
            argv[1],
            "add") == 0) {

        ret =
            command_add(
                argc - 1,
                &argv[1]);

    } else if (strcmp(
            argv[1],
            "delete") == 0) {

        if (argc < 3) {

            fprintf(
                stderr,
                "ERROR: rule ID required\n");

            ret = 1;

        } else {

            ret =
                command_delete(
                    argv[2]);
        }

    } else if (strcmp(
            argv[1],
            "list") == 0) {

        ret =
            command_list();

    } else if (strcmp(
            argv[1],
            "clear") == 0) {

        ret =
            command_clear();

    } else if (strcmp(
            argv[1],
            "enable") == 0) {

        if (argc < 3) {

            fprintf(
                stderr,
                "ERROR: rule ID required\n");

            ret = 1;

        } else {

            ret =
                command_enable(
                    argv[2]);
        }

    } else if (strcmp(
            argv[1],
            "disable") == 0) {

        if (argc < 3) {

            fprintf(
                stderr,
                "ERROR: rule ID required\n");

            ret = 1;

        } else {

            ret =
                command_disable(
                    argv[2]);
        }

    } else if (strcmp(
            argv[1],
            "load") == 0) {

        if (argc < 3) {

            fprintf(
                stderr,
                "ERROR: configuration file required\n");

            ret = 1;

        } else {

            ret =
                command_load(
                    argv[2]);
        }

    } else if (strcmp(
            argv[1],
            "reload") == 0) {

        ret =
            command_reload();

    } else if (strcmp(
            argv[1],
            "save") == 0) {

        if (argc < 3) {

            fprintf(
                stderr,
                "ERROR: output file required\n");

            ret = 1;

        } else {

            ret =
                command_save(
                    argv[2]);
        }

    } else if (strcmp(
            argv[1],
            "stats") == 0) {

        ret =
            command_stats();

    } else if (strcmp(
            argv[1],
            "reset-stats") == 0) {

        ret =
            command_reset_stats();

    } else if (strcmp(
            argv[1],
            "monitor") == 0) {

        if (argc < 3) {

            fprintf(
                stderr,
                "ERROR: specify on or off\n");

            ret = 1;

        } else {

            ret =
                command_monitor(
                    argv[2]);
        }

    } else if (strcmp(
            argv[1],
            "interface") == 0) {

        if (argc >= 3) {

            ret =
                command_interface(
                    argv[2]);

        } else {

            ret =
                command_interface(
                    NULL);
        }

    } else if (strcmp(
            argv[1],
            "default-action") == 0) {

        if (argc >= 3) {

            ret =
                command_default_action(
                    argv[2]);

        } else {

            ret =
                command_default_action(
                    NULL);
        }

    } else if (strcmp(
            argv[1],
            "help") == 0 ||
               strcmp(
            argv[1],
            "--help") == 0 ||
               strcmp(
            argv[1],
            "-h") == 0) {

        print_usage(
            argv[0]);

        ret = 0;

    } else {

        fprintf(
            stderr,
            "ERROR: unknown command '%s'\n",
            argv[1]);

        print_usage(
            argv[0]);

        ret = 1;
    }

    filter_cleanup();

    return ret;
}
