#ifndef PACKET_FILTER_LOGGING_H
#define PACKET_FILTER_LOGGING_H

/*
 * Packet Filter Driver - Logging Interface
 *
 * Provides logging and diagnostic interfaces for the kernel
 * packet-filter driver.
 */

#include <linux/types.h>
#include <linux/netdevice.h>

/* ============================================================
 * Logging Levels
 * ============================================================ */

enum pf_log_level {
    PF_LOG_ERROR = 0,
    PF_LOG_WARN,
    PF_LOG_INFO,
    PF_LOG_DEBUG,
};

/* ============================================================
 * Logging Configuration
 * ============================================================ */

#define PF_LOG_PREFIX          "packet_filter: "

/*
 * Default logging configuration.
 *
 * Logging can be controlled from the driver implementation.
 */
#define PF_LOG_DEFAULT_LEVEL   PF_LOG_INFO


/* ============================================================
 * Driver Logging APIs
 * ============================================================ */

/*
 * Generic packet-filter logging function.
 *
 * @level: Logging level.
 * @fmt:   printk-style format string.
 */
void pf_log(enum pf_log_level level, const char *fmt, ...);


/*
 * Driver initialization logging.
 */
void pf_log_driver_init(void);


/*
 * Driver cleanup logging.
 */
void pf_log_driver_exit(void);


/*
 * Driver error logging.
 *
 * @fmt: printk-style format string.
 */
void pf_log_error(const char *fmt, ...);


/*
 * Driver warning logging.
 *
 * @fmt: printk-style format string.
 */
void pf_log_warn(const char *fmt, ...);


/*
 * Driver informational logging.
 *
 * @fmt: printk-style format string.
 */
void pf_log_info(const char *fmt, ...);


/*
 * Driver debug logging.
 *
 * @fmt: printk-style format string.
 */
void pf_log_debug(const char *fmt, ...);


/* ============================================================
 * Packet Logging
 * ============================================================ */

/*
 * Log an allowed packet.
 *
 * @src_ip:    Source IPv4 address string.
 * @dst_ip:    Destination IPv4 address string.
 * @src_port:  Source port.
 * @dst_port:  Destination port.
 */
void pf_log_packet_allowed(const char *src_ip,
                           const char *dst_ip,
                           __u16 src_port,
                           __u16 dst_port);


/*
 * Log a dropped packet.
 *
 * @src_ip:    Source IPv4 address string.
 * @dst_ip:    Destination IPv4 address string.
 * @src_port:  Source port.
 * @dst_port:  Destination port.
 */
void pf_log_packet_dropped(const char *src_ip,
                           const char *dst_ip,
                           __u16 src_port,
                           __u16 dst_port);


/*
 * Log a monitored packet.
 *
 * @src_ip:    Source IPv4 address string.
 * @dst_ip:    Destination IPv4 address string.
 * @src_port:  Source port.
 * @dst_port:  Destination port.
 */
void pf_log_packet_monitored(const char *src_ip,
                             const char *dst_ip,
                             __u16 src_port,
                             __u16 dst_port);


/*
 * Log a packet associated with a network device.
 *
 * @dev:       Network device.
 * @src_ip:    Source IPv4 address string.
 * @dst_ip:    Destination IPv4 address string.
 * @src_port:  Source port.
 * @dst_port:  Destination port.
 */
void pf_log_packet_device(const struct net_device *dev,
                          const char *src_ip,
                          const char *dst_ip,
                          __u16 src_port,
                          __u16 dst_port);


/* ============================================================
 * Rule Logging
 * ============================================================ */

/*
 * Log addition of a packet-filter rule.
 *
 * @rule_name: Rule identifier/name.
 */
void pf_log_rule_added(const char *rule_name);


/*
 * Log removal of a packet-filter rule.
 *
 * @rule_name: Rule identifier/name.
 */
void pf_log_rule_removed(const char *rule_name);


/*
 * Log rule match.
 *
 * @rule_name: Rule that matched the packet.
 */
void pf_log_rule_match(const char *rule_name);


/*
 * Log rule mismatch.
 */
void pf_log_rule_mismatch(void);


/* ============================================================
 * IOCTL Logging
 * ============================================================ */

/*
 * Log an IOCTL command received from userspace.
 *
 * @cmd: IOCTL command number.
 */
void pf_log_ioctl(unsigned int cmd);


/*
 * Log an invalid IOCTL command.
 *
 * @cmd: IOCTL command number.
 */
void pf_log_ioctl_invalid(unsigned int cmd);


/* ============================================================
 * Statistics Logging
 * ============================================================ */

/*
 * Log packet-filter statistics.
 *
 * @received: Number of received packets.
 * @allowed:  Number of allowed packets.
 * @dropped:  Number of dropped packets.
 * @monitored:Number of monitored packets.
 */
void pf_log_statistics(__u64 received,
                       __u64 allowed,
                       __u64 dropped,
                       __u64 monitored);


/* ============================================================
 * Network Error Logging
 * ============================================================ */

/*
 * Log network-device errors.
 *
 * @dev: Network device associated with the error.
 * @err: Error code.
 */
void pf_log_network_error(const struct net_device *dev, int err);


/* ============================================================
 * Debugging Helpers
 * ============================================================ */

/*
 * Enable or disable debug logging.
 *
 * @enable: true to enable debug logging.
 */
void pf_logging_set_debug(bool enable);


/*
 * Check whether debug logging is enabled.
 */
bool pf_logging_debug_enabled(void);


/* ============================================================
 * Logging Statistics
 * ============================================================ */

/*
 * Logging counters.
 */
struct pf_log_stats {
    __u64 error_count;
    __u64 warning_count;
    __u64 info_count;
    __u64 debug_count;
};


/*
 * Retrieve logging statistics.
 *
 * @stats: Pointer to statistics structure.
 */
void pf_logging_get_stats(struct pf_log_stats *stats);


/*
 * Reset logging statistics.
 */
void pf_logging_reset_stats(void);

#endif /* PACKET_FILTER_LOGGING_H */
