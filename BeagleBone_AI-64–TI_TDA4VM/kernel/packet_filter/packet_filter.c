/*
 * Packet Filter Driver - Main Implementation
 *
 * BeagleBone AI-64 / TI TDA4VM
 *
 * Main kernel module for packet filtering.
 *
 * Responsibilities:
 *   - Driver initialization
 *   - Character device creation
 *   - Packet processing
 *   - Rule management
 *   - Packet statistics
 *   - Enable / disable filtering
 *   - Network RX integration
 *   - Driver cleanup
 */

#include "packet_filter.h"
#include "ioctl_defs.h"
#include "logging.h"

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/if_ether.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/uaccess.h>

/* ============================================================
 * Module Information
 * ============================================================ */

#define PF_MODULE_NAME        "packet_filter"
#define PF_CLASS_NAME         "packet_filter"
#define PF_DEVICE_NODE        "packet_filter"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BeagleBone AI-64 Packet Filter Project");
MODULE_DESCRIPTION(
    "Packet Filter Driver for BeagleBone AI-64 / TI TDA4VM"
);
MODULE_VERSION(PF_DRIVER_VERSION);


/* ============================================================
 * Global Driver Instance
 * ============================================================ */

struct pf_device *pf_dev;

static dev_t pf_dev_number;

static struct cdev pf_cdev;

static struct class *pf_class;

static struct device *pf_device;


/* ============================================================
 * Driver State Lock
 * ============================================================ */

static DEFINE_MUTEX(pf_state_lock);


/* ============================================================
 * Character Device Operations
 * ============================================================ */

static int pf_open(struct inode *inode, struct file *file)
{
    if (!pf_dev)
        return -ENODEV;

    file->private_data = pf_dev;

    pf_log_info("device opened\n");

    return 0;
}


static int pf_release(struct inode *inode, struct file *file)
{
    pf_log_debug("device released\n");

    return 0;
}


/* ============================================================
 * IOCTL Interface
 * ============================================================ */

static long pf_ioctl(struct file *file,
                     unsigned int cmd,
                     unsigned long arg)
{
    struct pf_device *dev;
    int ret = 0;

    dev = file->private_data;

    if (!dev)
        return -ENODEV;

    pf_log_ioctl(cmd);

    switch (cmd) {

    case PF_IOCTL_ENABLE:
        ret = pf_enable();
        break;

    case PF_IOCTL_DISABLE:
        ret = pf_disable();
        break;

    case PF_IOCTL_CLEAR_RULES:
        pf_clear_rules();
        break;

    case PF_IOCTL_RESET_STATS:
        pf_reset_statistics();
        break;

    default:
        pf_log_ioctl_invalid(cmd);
        ret = -EINVAL;
        break;
    }

    return ret;
}


/* ============================================================
 * File Operations
 * ============================================================ */

static const struct file_operations pf_fops = {
    .owner          = THIS_MODULE,
    .open           = pf_open,
    .release        = pf_release,
    .unlocked_ioctl = pf_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = pf_ioctl,
#endif
};


/* ============================================================
 * Driver Enable / Disable
 * ============================================================ */

int pf_enable(void)
{
    if (!pf_dev)
        return -ENODEV;

    mutex_lock(&pf_state_lock);

    if (pf_dev->enabled) {
        mutex_unlock(&pf_state_lock);
        return 0;
    }

    pf_dev->enabled = true;

    mutex_unlock(&pf_state_lock);

    pf_log_info("packet filtering enabled\n");

    return 0;
}


int pf_disable(void)
{
    if (!pf_dev)
        return -ENODEV;

    mutex_lock(&pf_state_lock);

    if (!pf_dev->enabled) {
        mutex_unlock(&pf_state_lock);
        return 0;
    }

    pf_dev->enabled = false;

    mutex_unlock(&pf_state_lock);

    pf_log_info("packet filtering disabled\n");

    return 0;
}


bool pf_is_enabled(void)
{
    bool enabled;

    if (!pf_dev)
        return false;

    mutex_lock(&pf_state_lock);

    enabled = pf_dev->enabled;

    mutex_unlock(&pf_state_lock);

    return enabled;
}


/* ============================================================
 * Packet Statistics
 * ============================================================ */

void pf_reset_statistics(void)
{
    if (!pf_dev)
        return;

    atomic64_set(&pf_dev->stats.packets_received, 0);
    atomic64_set(&pf_dev->stats.packets_allowed, 0);
    atomic64_set(&pf_dev->stats.packets_dropped, 0);
    atomic64_set(&pf_dev->stats.packets_monitored, 0);

    atomic64_set(&pf_dev->stats.bytes_received, 0);
    atomic64_set(&pf_dev->stats.bytes_allowed, 0);
    atomic64_set(&pf_dev->stats.bytes_dropped, 0);
    atomic64_set(&pf_dev->stats.bytes_monitored, 0);

    atomic64_set(&pf_dev->stats.rule_matches, 0);
    atomic64_set(&pf_dev->stats.rule_misses, 0);

    atomic64_set(&pf_dev->stats.invalid_packets, 0);
    atomic64_set(&pf_dev->stats.errors, 0);

    pf_log_info("packet statistics reset\n");
}


void pf_stats_packet_received(unsigned int bytes)
{
    if (!pf_dev)
        return;

    atomic64_inc(&pf_dev->stats.packets_received);
    atomic64_add(bytes, &pf_dev->stats.bytes_received);
}


void pf_stats_packet_allowed(unsigned int bytes)
{
    if (!pf_dev)
        return;

    atomic64_inc(&pf_dev->stats.packets_allowed);
    atomic64_add(bytes, &pf_dev->stats.bytes_allowed);
}


void pf_stats_packet_dropped(unsigned int bytes)
{
    if (!pf_dev)
        return;

    atomic64_inc(&pf_dev->stats.packets_dropped);
    atomic64_add(bytes, &pf_dev->stats.bytes_dropped);
}


void pf_stats_packet_monitored(unsigned int bytes)
{
    if (!pf_dev)
        return;

    atomic64_inc(&pf_dev->stats.packets_monitored);
    atomic64_add(bytes, &pf_dev->stats.bytes_monitored);
}


/* ============================================================
 * Packet Information Extraction
 * ============================================================ */

int pf_extract_packet_info(struct sk_buff *skb,
                           struct pf_packet_info *info)
{
    struct iphdr *iph;
    unsigned int ip_header_length;

    if (!skb || !info)
        return -EINVAL;

    memset(info, 0, sizeof(*info));

    info->skb = skb;
    info->dev = skb->dev;
    info->packet_length = skb->len;

    if (!pskb_may_pull(skb, sizeof(struct ethhdr)))
        return -EINVAL;

    /*
     * Only IPv4 packets are processed by this basic implementation.
     */
    if (skb->protocol != htons(ETH_P_IP))
        return -EPROTONOSUPPORT;

    if (!pskb_may_pull(
            skb,
            sizeof(struct ethhdr) + sizeof(struct iphdr)))
        return -EINVAL;

    iph = ip_hdr(skb);

    if (!iph)
        return -EINVAL;

    if (iph->version != 4)
        return -EINVAL;

    ip_header_length = iph->ihl * 4;

    if (ip_header_length < sizeof(struct iphdr))
        return -EINVAL;

    if (!pskb_may_pull(
            skb,
            sizeof(struct ethhdr) + ip_header_length))
        return -EINVAL;

    info->src_ip = iph->saddr;
    info->dst_ip = iph->daddr;
    info->protocol = iph->protocol;

    /*
     * TCP source/destination ports.
     */
    if (iph->protocol == IPPROTO_TCP) {

        struct tcphdr *tcph;

        tcph = tcp_hdr(skb);

        if (!tcph)
            return -EINVAL;

        info->src_port = tcph->source;
        info->dst_port = tcph->dest;
    }

    /*
     * UDP source/destination ports.
     */
    else if (iph->protocol == IPPROTO_UDP) {

        struct udphdr *udph;

        udph = udp_hdr(skb);

        if (!udph)
            return -EINVAL;

        info->src_port = udph->source;
        info->dst_port = udph->dest;
    }

    return 0;
}


/* ============================================================
 * Packet Processing
 * ============================================================ */

int pf_process_packet(struct sk_buff *skb)
{
    struct pf_packet_info packet;
    enum pf_action action;
    int ret;

    if (!skb)
        return -EINVAL;

    if (!pf_dev)
        return -ENODEV;

    /*
     * If filtering is disabled, allow packet.
     */
    if (!pf_is_enabled())
        return PF_ACTION_ALLOW;

    pf_stats_packet_received(skb->len);

    ret = pf_extract_packet_info(skb, &packet);

    if (ret) {
        atomic64_inc(&pf_dev->stats.invalid_packets);

        pf_log_debug(
            "unable to extract packet information: %d\n",
            ret
        );

        return PF_ACTION_ALLOW;
    }

    /*
     * Apply filtering rules.
     */
    action = pf_filter_packet(&packet);

    switch (action) {

    case PF_ACTION_ALLOW:
        pf_allow_packet(skb);
        return PF_ACTION_ALLOW;

    case PF_ACTION_DROP:
        pf_drop_packet(skb);
        return PF_ACTION_DROP;

    case PF_ACTION_MONITOR:
        pf_monitor_packet(skb);
        return PF_ACTION_MONITOR;

    default:
        atomic64_inc(&pf_dev->stats.errors);
        return PF_ACTION_ALLOW;
    }
}


/* ============================================================
 * Packet Actions
 * ============================================================ */

int pf_allow_packet(struct sk_buff *skb)
{
    if (!skb)
        return -EINVAL;

    pf_stats_packet_allowed(skb->len);

    pf_log_debug(
        "packet allowed: length=%u\n",
        skb->len
    );

    /*
     * Returning zero from the RX handler allows the packet to
     * continue through the normal Linux networking stack.
     */
    return 0;
}


int pf_drop_packet(struct sk_buff *skb)
{
    if (!skb)
        return -EINVAL;

    pf_stats_packet_dropped(skb->len);

    pf_log_debug(
        "packet dropped: length=%u\n",
        skb->len
    );

    return 0;
}


int pf_monitor_packet(struct sk_buff *skb)
{
    if (!skb)
        return -EINVAL;

    pf_stats_packet_monitored(skb->len);

    pf_log_debug(
        "packet monitored: length=%u\n",
        skb->len
    );

    /*
     * MONITOR does not block the packet.
     */
    return 0;
}


/* ============================================================
 * Network RX Handler
 * ============================================================ */

rx_handler_result_t pf_rx_handler(struct sk_buff **pskb)
{
    struct sk_buff *skb;
    int action;

    if (!pskb || !*pskb)
        return RX_HANDLER_PASS;

    skb = *pskb;

    /*
     * If driver is disabled, allow normal networking.
     */
    if (!pf_is_enabled())
        return RX_HANDLER_PASS;

    action = pf_process_packet(skb);

    switch (action) {

    case PF_ACTION_DROP:

        /*
         * The packet must be consumed when dropped.
         */
        kfree_skb(skb);

        *pskb = NULL;

        return RX_HANDLER_CONSUMED;

    case PF_ACTION_MONITOR:
    case PF_ACTION_ALLOW:
    default:

        return RX_HANDLER_PASS;
    }
}


/* ============================================================
 * Network Device Registration
 * ============================================================ */

int pf_register_netdev(void)
{
    /*
     * Network RX handler registration is normally performed
     * against the target Ethernet device.
     *
     * The actual device can be selected from Device Tree,
     * module configuration, or userspace depending on the
     * final project architecture.
     *
     * This basic implementation does not automatically attach
     * to every network device.
     */

    pf_log_info(
        "network packet filtering interface initialized\n"
    );

    return 0;
}


void pf_unregister_netdev(void)
{
    pf_log_info(
        "network packet filtering interface removed\n"
    );
}


/* ============================================================
 * Rule Management
 * ============================================================ */

int pf_add_rule(struct pf_rule *rule)
{
    struct pf_rule *new_rule;

    if (!pf_dev || !rule)
        return -EINVAL;

    new_rule = kmemdup(rule, sizeof(*new_rule), GFP_KERNEL);

    if (!new_rule)
        return -ENOMEM;

    INIT_LIST_HEAD(&new_rule->list);

    atomic64_set(&new_rule->packet_count, 0);
    atomic64_set(&new_rule->byte_count, 0);

    mutex_lock(&pf_dev->rule_lock);

    if (pf_find_rule(rule->id)) {
        mutex_unlock(&pf_dev->rule_lock);

        kfree(new_rule);

        return -EEXIST;
    }

    list_add_tail(&new_rule->list, &pf_dev->rule_list);

    mutex_unlock(&pf_dev->rule_lock);

    pf_log_rule_added(new_rule->name);

    return 0;
}


struct pf_rule *pf_find_rule(__u32 rule_id)
{
    struct pf_rule *rule;

    if (!pf_dev)
        return NULL;

    list_for_each_entry(rule, &pf_dev->rule_list, list) {

        if (rule->id == rule_id)
            return rule;
    }

    return NULL;
}


int pf_delete_rule(__u32 rule_id)
{
    struct pf_rule *rule;

    if (!pf_dev)
        return -ENODEV;

    mutex_lock(&pf_dev->rule_lock);

    rule = pf_find_rule(rule_id);

    if (!rule) {
        mutex_unlock(&pf_dev->rule_lock);
        return -ENOENT;
    }

    list_del(&rule->list);

    mutex_unlock(&pf_dev->rule_lock);

    pf_log_rule_removed(rule->name);

    kfree(rule);

    return 0;
}


void pf_clear_rules(void)
{
    struct pf_rule *rule;
    struct pf_rule *tmp;

    if (!pf_dev)
        return;

    mutex_lock(&pf_dev->rule_lock);

    list_for_each_entry_safe(rule, tmp,
                             &pf_dev->rule_list, list) {

        list_del(&rule->list);

        kfree(rule);
    }

    mutex_unlock(&pf_dev->rule_lock);

    pf_log_info("all packet-filter rules cleared\n");
}


int pf_enable_rule(__u32 rule_id)
{
    struct pf_rule *rule;

    if (!pf_dev)
        return -ENODEV;

    mutex_lock(&pf_dev->rule_lock);

    rule = pf_find_rule(rule_id);

    if (!rule) {
        mutex_unlock(&pf_dev->rule_lock);
        return -ENOENT;
    }

    rule->enabled = true;

    mutex_unlock(&pf_dev->rule_lock);

    return 0;
}


int pf_disable_rule(__u32 rule_id)
{
    struct pf_rule *rule;

    if (!pf_dev)
        return -ENODEV;

    mutex_lock(&pf_dev->rule_lock);

    rule = pf_find_rule(rule_id);

    if (!rule) {
        mutex_unlock(&pf_dev->rule_lock);
        return -ENOENT;
    }

    rule->enabled = false;

    mutex_unlock(&pf_dev->rule_lock);

    return 0;
}


/* ============================================================
 * Driver Initialization
 * ============================================================ */

int pf_init(void)
{
    int ret;

    if (pf_dev)
        return -EEXIST;

    pf_dev = kzalloc(sizeof(*pf_dev), GFP_KERNEL);

    if (!pf_dev)
        return -ENOMEM;

    INIT_LIST_HEAD(&pf_dev->rule_list);

    mutex_init(&pf_dev->rule_lock);

    pf_dev->enabled = false;
    pf_dev->initialized = false;
    pf_dev->debug_enabled = false;

    pf_reset_statistics();

    /*
     * Allocate character device number.
     */
    ret = alloc_chrdev_region(
        &pf_dev_number,
        0,
        1,
        PF_DEVICE_NODE
    );

    if (ret) {
        pf_log_error(
            "failed to allocate character device: %d\n",
            ret
        );

        goto error_device_number;
    }

    pf_dev->dev_number = pf_dev_number;

    /*
     * Initialize character device.
     */
    cdev_init(&pf_cdev, &pf_fops);

    pf_cdev.owner = THIS_MODULE;

    ret = cdev_add(
        &pf_cdev,
        pf_dev_number,
        1
    );

    if (ret) {
        pf_log_error(
            "failed to add character device: %d\n",
            ret
        );

        goto error_cdev;
    }

    /*
     * Create device class.
     */
    pf_class = class_create(PF_CLASS_NAME);

    if (IS_ERR(pf_class)) {

        ret = PTR_ERR(pf_class);

        pf_log_error(
            "failed to create device class: %d\n",
            ret
        );

        goto error_class;
    }

    pf_dev->class = pf_class;

    /*
     * Create /dev/packet_filter.
     */
    pf_device = device_create(
        pf_class,
        NULL,
        pf_dev_number,
        NULL,
        PF_DEVICE_NODE
    );

    if (IS_ERR(pf_device)) {

        ret = PTR_ERR(pf_device);

        pf_log_error(
            "failed to create device node: %d\n",
            ret
        );

        goto error_device;
    }

    pf_dev->device = pf_device;

    /*
     * Register network integration.
     */
    ret = pf_register_netdev();

    if (ret) {
        pf_log_error(
            "failed to initialize network interface: %d\n",
            ret
        );

        goto error_netdev;
    }

    pf_dev->initialized = true;

    /*
     * Filtering starts disabled.
     *
     * Userspace can enable it through IOCTL.
     */
    pf_dev->enabled = false;

    pf_log_driver_init();

    pf_log_info(
        "driver loaded successfully, version=%s\n",
        PF_DRIVER_VERSION
    );

    return 0;


/* ============================================================
 * Initialization Error Handling
 * ============================================================ */

error_netdev:

    device_destroy(
        pf_class,
        pf_dev_number
    );

error_device:

    class_destroy(pf_class);

error_class:

    cdev_del(&pf_cdev);

error_cdev:

    unregister_chrdev_region(
        pf_dev_number,
        1
    );

error_device_number:

    kfree(pf_dev);

    pf_dev = NULL;

    return ret;
}


/* ============================================================
 * Driver Cleanup
 * ============================================================ */

void pf_exit(void)
{
    if (!pf_dev)
        return;

    pf_log_driver_exit();

    /*
     * Disable filtering before cleanup.
     */
    pf_disable();

    /*
     * Remove network integration.
     */
    pf_unregister_netdev();

    /*
     * Remove all rules.
     */
    pf_clear_rules();

    /*
     * Remove device node.
     */
    if (pf_device &&
        !IS_ERR(pf_device)) {

        device_destroy(
            pf_class,
            pf_dev_number
        );
    }

    /*
     * Destroy class.
     */
    if (pf_class &&
        !IS_ERR(pf_class)) {

        class_destroy(pf_class);
    }

    /*
     * Remove character device.
     */
    cdev_del(&pf_cdev);

    /*
     * Release device number.
     */
    unregister_chrdev_region(
        pf_dev_number,
        1
    );

    pf_dev->initialized = false;

    kfree(pf_dev);

    pf_dev = NULL;

    pr_info(
        PF_LOG_PREFIX
        "driver unloaded\n"
    );
}


/* ============================================================
 * Kernel Module Entry / Exit
 * ============================================================ */

static int __init packet_filter_module_init(void)
{
    int ret;

    pr_info(
        PF_LOG_PREFIX
        "initializing BeagleBone AI-64 packet filter\n"
    );

    ret = pf_init();

    if (ret) {

        pr_err(
            PF_LOG_PREFIX
            "initialization failed: %d\n",
            ret
        );

        return ret;
    }

    return 0;
}


static void __exit packet_filter_module_exit(void)
{
    pr_info(
        PF_LOG_PREFIX
        "removing packet filter driver\n"
    );

    pf_exit();
}


module_init(packet_filter_module_init);
module_exit(packet_filter_module_exit);
