/*
 * Packet Filter Driver - Logging Implementation
 *
 * Provides logging and diagnostic functionality for the
 * BeagleBone AI-64 / TI TDA4VM packet-filter driver.
 */

#include "logging.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/spinlock.h>
#include <linux/string.h>

/* ============================================================
 * Module Information
 * ============================================================ */

#define PF_LOG_MODULE_NAME    "packet_filter"

/* ============================================================
 * Internal Logging State
 * ============================================================ */

static enum pf_log_level pf_current_log_level = PF_LOG_DEFAULT_LEVEL;

static bool pf_debug_enabled;

static struct pf_log_stats pf_log_statistics;

static DEFINE_SPINLOCK(pf_log_lock);


/* ============================================================
 * Internal Helper
 * ============================================================ */

static const char *pf_log_level_string(enum pf_log_level level)
{
    switch (level) {
    case PF_LOG_ERROR:
        return "ERROR";

    case PF_LOG_WARN:
        return "WARN";

    case PF_LOG_INFO:
        return "INFO";

    case PF_LOG_DEBUG:
        return "DEBUG";

    default:
        return "UNKNOWN";
    }
}


/* ============================================================
 * Generic Logging
 * ============================================================ */

void pf_log(enum pf_log_level level, const char *fmt, ...)
{
    va_list args;
    unsigned long flags;

    if (level > pf_current_log_level)
        return;

    if (level == PF_LOG_DEBUG && !pf_debug_enabled)
        return;

    spin_lock_irqsave(&pf_log_lock, flags);

    switch (level) {
    case PF_LOG_ERROR:
        pf_log_statistics.error_count++;
        break;

    case PF_LOG_WARN:
        pf_log_statistics.warning_count++;
        break;

    case PF_LOG_INFO:
        pf_log_statistics.info_count++;
        break;

    case PF_LOG_DEBUG:
        pf_log_statistics.debug_count++;
        break;

    default:
        break;
    }

    spin_unlock_irqrestore(&pf_log_lock, flags);

    va_start(args, fmt);

    switch (level) {
    case PF_LOG_ERROR:
        vprintk(KERN_ERR PF_LOG_PREFIX "%s: " , args);
        break;

    case PF_LOG_WARN:
        vprintk(KERN_WARNING PF_LOG_PREFIX "%s: " , args);
        break;

    case PF_LOG_INFO:
        vprintk(KERN_INFO PF_LOG_PREFIX "%s: " , args);
        break;

    case PF_LOG_DEBUG:
        vprintk(KERN_DEBUG PF_LOG_PREFIX "%s: " , args);
        break;

    default:
        vprintk(KERN_INFO PF_LOG_PREFIX "%s: " , args);
        break;
    }

    va_end(args);
}


/* ============================================================
 * Error Logging
 * ============================================================ */

void pf_log_error(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    pr_err(PF_LOG_PREFIX);

    vprintk(KERN_ERR, fmt, args);

    va_end(args);
}


/* ============================================================
 * Warning Logging
 * ============================================================ */

void pf_log_warn(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    pr_warn(PF_LOG_PREFIX);

    vprintk(KERN_WARNING, fmt, args);

    va_end(args);
}


/* ============================================================
 * Information Logging
 * ============================================================ */

void pf_log_info(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    pr_info(PF_LOG_PREFIX);

    vprintk(KERN_INFO, fmt, args);

    va_end(args);
}


/* ============================================================
 * Debug Logging
 * ============================================================ */

void pf_log_debug(const char *fmt, ...)
{
    va_list args;

    if (!pf_debug_enabled)
        return;

    va_start(args, fmt);

    pr_debug(PF_LOG_PREFIX);

    vprintk(KERN_DEBUG, fmt, args);

    va_end(args);
}


/* ============================================================
 * Driver Lifecycle Logging
 * ============================================================ */

void pf_log_driver_init(void)
{
    pf_log_info("driver initialization started\n");
}


void pf_log_driver_exit(void)
{
    pf_log_info("driver shutdown started\n");
}


/* ============================================================
 * Packet Logging
 * ============================================================ */

void pf_log_packet_allowed(const char *src_ip,
                           const char *dst_ip,
                           __u16 src_port,
                           __u16 dst_port)
{
    pf_log_info(
        "PACKET ALLOWED: %s:%u -> %s:%u\n",
        src_ip ? src_ip : "unknown",
        src_port,
        dst_ip ? dst_ip : "unknown",
        dst_port
    );
}


void pf_log_packet_dropped(const char *src_ip,
                           const char *dst_ip,
                           __u16 src_port,
                           __u16 dst_port)
{
    pf_log_warn(
        "PACKET DROPPED: %s:%u -> %s:%u\n",
        src_ip ? src_ip : "unknown",
        src_port,
        dst_ip ? dst_ip : "unknown",
        dst_port
    );
}


void pf_log_packet_monitored(const char *src_ip,
                             const char *dst_ip,
                             __u16 src_port,
                             __u16 dst_port)
{
    pf_log_debug(
        "PACKET MONITORED: %s:%u -> %s:%u\n",
        src_ip ? src_ip : "unknown",
        src_port,
        dst_ip ? dst_ip : "unknown",
        dst_port
    );
}


void pf_log_packet_device(const struct net_device *dev,
                          const char *src_ip,
                          const char *dst_ip,
                          __u16 src_port,
                          __u16 dst_port)
{
    const char *device_name = "unknown";

    if (dev)
        device_name = dev->name;

    pf_log_info(
        "PACKET: dev=%s %s:%u -> %s:%u\n",
        device_name,
        src_ip ? src_ip : "unknown",
        src_port,
        dst_ip ? dst_ip : "unknown",
        dst_port
    );
}


/* ============================================================
 * Rule Logging
 * ============================================================ */

void pf_log_rule_added(const char *rule_name)
{
    pf_log_info(
        "RULE ADDED: %s\n",
        rule_name ? rule_name : "unknown"
    );
}


void pf_log_rule_removed(const char *rule_name)
{
    pf_log_info(
        "RULE REMOVED: %s\n",
        rule_name ? rule_name : "unknown"
    );
}


void pf_log_rule_match(const char *rule_name)
{
    pf_log_debug(
        "RULE MATCH: %s\n",
        rule_name ? rule_name : "unknown"
    );
}


void pf_log_rule_mismatch(void)
{
    pf_log_debug("RULE MATCH: no matching rule\n");
}


/* ============================================================
 * IOCTL Logging
 * ============================================================ */

void pf_log_ioctl(unsigned int cmd)
{
    pf_log_debug(
        "IOCTL received: cmd=0x%x\n",
        cmd
    );
}


void pf_log_ioctl_invalid(unsigned int cmd)
{
    pf_log_warn(
        "Invalid IOCTL command: cmd=0x%x\n",
        cmd
    );
}


/* ============================================================
 * Statistics Logging
 * ============================================================ */

void pf_log_statistics(__u64 received,
                       __u64 allowed,
                       __u64 dropped,
                       __u64 monitored)
{
    pf_log_info(
        "STATISTICS: received=%llu allowed=%llu "
        "dropped=%llu monitored=%llu\n",
        (unsigned long long)received,
        (unsigned long long)allowed,
        (unsigned long long)dropped,
        (unsigned long long)monitored
    );
}


/* ============================================================
 * Network Error Logging
 * ============================================================ */

void pf_log_network_error(const struct net_device *dev, int err)
{
    const char *device_name = "unknown";

    if (dev)
        device_name = dev->name;

    pf_log_error(
        "NETWORK ERROR: dev=%s error=%d\n",
        device_name,
        err
    );
}


/* ============================================================
 * Debug Control
 * ============================================================ */

void pf_logging_set_debug(bool enable)
{
    unsigned long flags;

    spin_lock_irqsave(&pf_log_lock, flags);

    pf_debug_enabled = enable;

    spin_unlock_irqrestore(&pf_log_lock, flags);

    if (enable)
        pr_info(PF_LOG_PREFIX "debug logging enabled\n");
    else
        pr_info(PF_LOG_PREFIX "debug logging disabled\n");
}


bool pf_logging_debug_enabled(void)
{
    return pf_debug_enabled;
}


/* ============================================================
 * Logging Statistics
 * ============================================================ */

void pf_logging_get_stats(struct pf_log_stats *stats)
{
    unsigned long flags;

    if (!stats)
        return;

    spin_lock_irqsave(&pf_log_lock, flags);

    memcpy(stats, &pf_log_statistics, sizeof(*stats));

    spin_unlock_irqrestore(&pf_log_lock, flags);
}


void pf_logging_reset_stats(void)
{
    unsigned long flags;

    spin_lock_irqsave(&pf_log_lock, flags);

    memset(&pf_log_statistics, 0, sizeof(pf_log_statistics));

    spin_unlock_irqrestore(&pf_log_lock, flags);

    pr_info(PF_LOG_PREFIX "logging statistics reset\n");
}


/* ============================================================
 * Module Metadata
 * ============================================================ */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BeagleBone AI-64 Packet Filter Project");
MODULE_DESCRIPTION(
    "Logging subsystem for BeagleBone AI-64 TI TDA4VM packet filter"
);
MODULE_VERSION("1.0");
