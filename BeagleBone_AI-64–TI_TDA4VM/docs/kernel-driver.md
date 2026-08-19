# `docs/kernel-driver.md`

````markdown
# Kernel Driver – BeagleBone AI-64 / TI TDA4VM

## 1. Overview

The packet-filter kernel driver is the kernel-space component responsible for applying packet filtering rules to network traffic on the BeagleBone AI-64 platform based on the TI TDA4VM SoC.

The driver provides:

- Packet filtering
- Whitelist processing
- Blacklist processing
- Monitoring
- Packet statistics
- Runtime configuration
- User-space control through IOCTL
- Integration with the Linux networking stack

Overall architecture:

```text
                    USER SPACE
                         │
              ┌──────────┴──────────┐
              │                     │
              ▼                     ▼
      Configuration Files      Control Application
              │                     │
              └──────────┬──────────┘
                         │
                       ioctl()
                         │
                         ▼
                    /dev/packet_filter
                         │
                         ▼
                    KERNEL SPACE
                         │
                  Packet Filter Driver
                         │
          ┌──────────────┼──────────────┐
          │              │              │
          ▼              ▼              ▼
      Rule Table     Statistics      Monitoring
          │
          ▼
     Packet Matching
          │
          ▼
   Linux Network Stack
          │
          ▼
   Ethernet Driver / PHY
          │
          ▼
      TDA4VM Ethernet
````

---

# 2. Driver Responsibilities

The kernel driver is responsible for the actual enforcement of filtering policy.

Its major responsibilities are:

1. Register the driver.
2. Create the character device.
3. Expose `/dev/packet_filter`.
4. Handle IOCTL commands.
5. Maintain filtering rules.
6. Validate packet information.
7. Apply whitelist rules.
8. Apply blacklist rules.
9. Maintain packet counters.
10. Provide monitoring information.
11. Synchronize shared data.
12. Clean up resources during module removal.

---

# 3. Driver Architecture

The driver can be divided into logical components:

```text
packet_filter.c
│
├── Module Initialization
│
├── Character Device
│
├── File Operations
│
├── IOCTL Handler
│
├── Rule Manager
│
├── Packet Matching Engine
│
├── Statistics Manager
│
├── Monitoring
│
├── Network Integration
│
└── Module Cleanup
```

Recommended project structure:

```text
driver/
├── packet_filter.c
├── packet_filter.h
├── packet_filter_ioctl.c
├── packet_filter_rules.c
├── packet_filter_net.c
├── packet_filter_stats.c
└── Makefile
```

For a small implementation, these components can initially remain inside one source file.

---

# 4. Kernel-Space and User-Space Boundary

Linux separates user space and kernel space.

```text
+----------------------------------+
|          User Space              |
|                                  |
| packet_filter_daemon             |
| configuration parser             |
| test application                 |
+----------------+-----------------+
                 |
                 | System Call
                 ▼
+----------------------------------+
|          Kernel Space            |
|                                  |
| Packet Filter Driver             |
| Rule Database                    |
| Packet Matching                  |
+----------------------------------+
                 |
                 ▼
       Linux Networking Stack
```

User space must not directly access kernel memory.

The driver uses:

```c
copy_from_user()
copy_to_user()
```

for transferring data.

---

# 5. Kernel Module

The driver can initially be implemented as a loadable kernel module.

Example:

```text
packet_filter.ko
```

Load:

```bash
sudo insmod packet_filter.ko
```

Check:

```bash
lsmod | grep packet_filter
```

Remove:

```bash
sudo rmmod packet_filter
```

Check kernel messages:

```bash
dmesg | tail -50
```

---

# 6. Module Initialization

The module initialization function is executed when the driver is loaded.

Example:

```c
static int __init packet_filter_init(void)
{
    int ret;

    pr_info("packet_filter: initializing\n");

    ret = alloc_chrdev_region(&pf_dev,
                              0,
                              1,
                              "packet_filter");
    if (ret)
        return ret;

    cdev_init(&pf_cdev, &pf_fops);

    ret = cdev_add(&pf_cdev, pf_dev, 1);
    if (ret)
        goto unregister_chrdev;

    pf_class = class_create("packet_filter");

    if (IS_ERR(pf_class)) {
        ret = PTR_ERR(pf_class);
        goto del_cdev;
    }

    pf_device = device_create(pf_class,
                              NULL,
                              pf_dev,
                              NULL,
                              "packet_filter");

    if (IS_ERR(pf_device)) {
        ret = PTR_ERR(pf_device);
        goto destroy_class;
    }

    pr_info("packet_filter: initialized\n");

    return 0;

destroy_class:
    class_destroy(pf_class);

del_cdev:
    cdev_del(&pf_cdev);

unregister_chrdev:
    unregister_chrdev_region(pf_dev, 1);

    return ret;
}
```

The exact APIs can vary across kernel versions.

---

# 7. Module Cleanup

The cleanup function reverses initialization.

```c
static void __exit packet_filter_exit(void)
{
    device_destroy(pf_class, pf_dev);
    class_destroy(pf_class);

    cdev_del(&pf_cdev);

    unregister_chrdev_region(pf_dev, 1);

    pr_info("packet_filter: removed\n");
}
```

Register the functions:

```c
module_init(packet_filter_init);
module_exit(packet_filter_exit);
```

---

# 8. Character Device

The character device provides the user-space control interface.

The relationship is:

```text
Kernel Driver
     │
     ▼
Character Device
     │
     ▼
/dev/packet_filter
     │
     ▼
User Application
```

The driver registers:

```c
alloc_chrdev_region()
```

Then:

```c
cdev_init()
cdev_add()
```

Then creates:

```text
/dev/packet_filter
```

---

# 9. File Operations

The driver defines supported file operations:

```c
static const struct file_operations pf_fops = {
    .owner          = THIS_MODULE,
    .open           = pf_open,
    .release        = pf_release,
    .unlocked_ioctl = pf_ioctl,
};
```

Important operations:

```text
open()
release()
ioctl()
```

The driver does not necessarily need `read()` or `write()` if all control operations are performed through IOCTL.

---

# 10. Open Operation

When user space executes:

```c
fd = open("/dev/packet_filter", O_RDWR);
```

the driver's `open()` callback executes.

Example:

```c
static int pf_open(struct inode *inode,
                   struct file *file)
{
    pr_debug("packet_filter: device opened\n");

    return 0;
}
```

Flow:

```text
Application
    │
    ▼
open()
    │
    ▼
VFS
    │
    ▼
file_operations
    │
    ▼
pf_open()
```

---

# 11. Release Operation

When the application closes the device:

```c
close(fd);
```

the driver's release callback is invoked.

```c
static int pf_release(struct inode *inode,
                       struct file *file)
{
    pr_debug("packet_filter: device closed\n");

    return 0;
}
```

---

# 12. IOCTL Interface

The IOCTL interface provides runtime control.

Commands defined in:

```text
include/packet_filter_ioctl.h
```

can include:

```text
PF_IOC_ENABLE
PF_IOC_DISABLE
PF_IOC_ADD_RULE
PF_IOC_DELETE_RULE
PF_IOC_GET_RULE
PF_IOC_CLEAR_RULES
PF_IOC_GET_STATS
PF_IOC_CLEAR_STATS
PF_IOC_ENABLE_MONITOR
PF_IOC_DISABLE_MONITOR
PF_IOC_GET_STATUS
```

Flow:

```text
User Application
       │
       │ ioctl()
       ▼
pf_ioctl()
       │
       ▼
Command Dispatch
       │
       ├── Rule Management
       ├── Statistics
       ├── Monitoring
       └── Filter State
```

Refer to:

```text
docs/ioctl-api.md
```

for the complete API definition.

---

# 13. IOCTL Handler

Example:

```c
static long pf_ioctl(struct file *file,
                     unsigned int cmd,
                     unsigned long arg)
{
    switch (cmd) {

    case PF_IOC_ENABLE:
        return pf_enable();

    case PF_IOC_DISABLE:
        return pf_disable();

    case PF_IOC_ADD_RULE:
        return pf_add_rule((void __user *)arg);

    case PF_IOC_DELETE_RULE:
        return pf_delete_rule((void __user *)arg);

    case PF_IOC_GET_STATS:
        return pf_get_stats((void __user *)arg);

    case PF_IOC_CLEAR_STATS:
        return pf_clear_stats();

    default:
        return -ENOTTY;
    }
}
```

Every command should validate its input before modifying kernel state.

---

# 14. Rule Database

The driver maintains a rule table.

Conceptual structure:

```c
struct pf_rule_entry {
    struct pf_rule rule;
    struct list_head list;
};
```

Global rule table:

```c
static LIST_HEAD(pf_rule_list);
```

Architecture:

```text
                 Rule Database
                      │
        ┌─────────────┼─────────────┐
        ▼             ▼             ▼
      Rule 1        Rule 2        Rule 3
        │             │             │
        ▼             ▼             ▼
     ALLOW          DROP           LOG
```

A maximum rule count should be defined to prevent uncontrolled memory consumption.

---

# 15. Rule Types

The project uses three major concepts:

```text
Whitelist
Blacklist
Monitoring
```

Example:

```text
Whitelist
    │
    └── Explicitly allowed traffic

Blacklist
    │
    └── Explicitly blocked traffic

Monitoring
    │
    └── Traffic observed/logged
```

---

# 16. Packet Filtering Decision

A simplified decision process is:

```text
                Incoming Packet
                       │
                       ▼
                 Packet Parser
                       │
                       ▼
                 Extract Fields
                       │
             ┌─────────┴─────────┐
             │                   │
             ▼                   ▼
       Whitelist Match?    Blacklist Match?
             │                   │
          YES│                YES│
             ▼                   ▼
           ALLOW                DROP
             │
             └─────────┬─────────┘
                       │
                       ▼
                  No Match
                       │
                       ▼
                Default Policy
```

The exact precedence must be explicitly defined in the implementation.

Recommended policy:

```text
Explicit blacklist → DROP
Explicit whitelist → ALLOW
No match           → Default policy
```

---

# 17. Packet Information

The filter may inspect fields such as:

```text
Source IP
Destination IP
Source Port
Destination Port
Protocol
Interface
Packet Length
```

For example:

```text
Packet
  │
  ├── Source IP
  ├── Destination IP
  ├── Protocol
  ├── Source Port
  └── Destination Port
```

These fields are compared against configured rules.

---

# 18. Packet Processing Flow

A simplified receive path:

```text
Ethernet PHY
    │
    ▼
Ethernet MAC
    │
    ▼
Ethernet Driver
    │
    ▼
Linux Network Stack
    │
    ▼
Packet Filter Hook
    │
    ▼
Packet Parser
    │
    ▼
Rule Matching
    │
 ┌──┴─────────────┐
 │                │
 ▼                ▼
ALLOW            DROP
 │                │
 ▼                ▼
Continue        Discard
```

The exact hook location must match the implementation.

---

# 19. Linux Network Integration

The driver should integrate with an appropriate Linux networking mechanism rather than manually intercepting hardware registers unless hardware-level filtering is specifically required.

Possible approaches include:

```text
Netfilter
TC
eBPF
XDP
Network driver integration
```

For a traditional kernel packet-filter implementation, Netfilter is a common option.

Conceptually:

```text
Packet
  │
  ▼
Network Stack
  │
  ▼
Filter Hook
  │
  ▼
Packet Filter Driver
  │
  ├── ACCEPT
  ├── DROP
  └── MONITOR
```

The selected mechanism should be documented in the actual implementation.

---

# 20. Packet Matching Algorithm

A basic matching algorithm:

```text
Receive Packet
      │
      ▼
Extract packet metadata
      │
      ▼
Search rule table
      │
      ▼
Compare:
 ├── Source IP
 ├── Destination IP
 ├── Protocol
 ├── Source Port
 └── Destination Port
      │
      ▼
Rule Match?
 ┌────┴────┐
 │         │
YES       NO
 │         │
 ▼         ▼
Action   Continue
 │
 ├── ALLOW
 ├── DROP
 └── LOG
```

---

# 21. IP Matching

An IP rule can use address and mask.

Example:

```text
Rule:
192.168.1.0/24
```

Conceptually:

```text
(packet_ip & mask) == (rule_ip & mask)
```

Example:

```text
Packet IP:
192.168.1.50

Rule:
192.168.1.0/24

Result:
MATCH
```

---

# 22. Port Matching

A TCP rule can match:

```text
Destination port = 22
```

Example:

```text
TCP packet
Destination Port = 22
       │
       ▼
Rule:
TCP + Port 22
       │
       ▼
MATCH
```

The same mechanism can be used for UDP.

---

# 23. Protocol Matching

Example:

```text
Protocol = TCP
```

Rule:

```text
PF_PROTO_TCP
```

Possible protocols:

```text
TCP
UDP
ICMP
ANY
```

---

# 24. Statistics

The driver maintains statistics.

Example:

```c
struct pf_stats {
    u64 packets_received;
    u64 packets_allowed;
    u64 packets_dropped;
    u64 packets_logged;

    u64 bytes_received;
    u64 bytes_dropped;
};
```

Flow:

```text
Packet
  │
  ▼
Filter
  │
  ├── Allowed → allowed++
  │
  ├── Dropped → dropped++
  │
  └── Logged  → logged++
```

Statistics can be retrieved using:

```text
PF_IOC_GET_STATS
```

---

# 25. Monitoring

Monitoring provides visibility into packet-filter activity.

Possible monitored events:

```text
Packet allowed
Packet dropped
Rule matched
Rule ID
Source IP
Destination IP
Protocol
Port
```

Flow:

```text
Packet
   │
   ▼
Rule Match
   │
   ▼
Monitoring Enabled?
   │
   ├── YES
   │    │
   │    ▼
   │  Record Event
   │
   └── NO
```

The implementation can expose monitoring information through kernel logs, a character-device interface, netlink, or another appropriate event mechanism.

---

# 26. Synchronization

The rule database and statistics can be accessed concurrently.

For example:

```text
CPU0                         CPU1
 │                            │
 │ ioctl(ADD_RULE)            │ Packet Processing
 │                            │
 ▼                            ▼
Rule Table                Rule Lookup
```

Concurrent access must be protected.

Possible mechanisms:

```c
struct mutex
spinlock_t
rwlock_t
atomic64_t
```

Use a mutex when operations can sleep.

Use a spinlock for short non-sleeping critical sections.

Use atomic operations for suitable individual counters.

---

# 27. Rule Table Locking

Example:

```c
static DEFINE_MUTEX(pf_rule_lock);
```

Adding a rule:

```c
mutex_lock(&pf_rule_lock);

list_add_tail(&entry->list, &pf_rule_list);

mutex_unlock(&pf_rule_lock);
```

Lookup:

```c
mutex_lock(&pf_rule_lock);

/* Search rules */

mutex_unlock(&pf_rule_lock);
```

The actual lock type must be chosen based on the packet-processing context.

---

# 28. Memory Management

Kernel memory must be allocated using kernel allocation APIs.

Example:

```c
entry = kzalloc(sizeof(*entry), GFP_KERNEL);
```

Free:

```c
kfree(entry);
```

For every allocation:

```text
Allocation
    │
    ▼
Use
    │
    ▼
Free
```

The driver must avoid:

* Memory leaks
* Use-after-free
* Double-free
* Invalid pointers
* Unbounded allocations

---

# 29. User-to-Kernel Data Validation

Never trust data received from user space.

Example:

```c
struct pf_rule rule;

if (copy_from_user(&rule,
                   (void __user *)arg,
                   sizeof(rule))) {
    return -EFAULT;
}
```

Then validate:

```text
Rule ID
Action
Protocol
IP
Mask
Ports
```

Only after validation should the rule be inserted into the kernel rule database.

---

# 30. Kernel Logging

Use kernel logging facilities:

```c
pr_info()
pr_warn()
pr_err()
pr_debug()
```

Examples:

```c
pr_info("packet_filter: driver loaded\n");

pr_err("packet_filter: failed to create device\n");

pr_debug("packet_filter: rule %u added\n",
         rule.id);
```

Runtime logs:

```bash
dmesg | grep packet_filter
```

---

# 31. Error Handling

Every kernel operation should check its return value.

Example:

```c
ret = cdev_add(&pf_cdev, pf_dev, 1);

if (ret) {
    pr_err("cdev_add failed: %d\n", ret);
    return ret;
}
```

On failure, already allocated resources must be released.

Correct cleanup:

```text
Step 1 succeeds
      │
      ▼
Step 2 succeeds
      │
      ▼
Step 3 fails
      │
      ▼
Undo Step 2
      │
      ▼
Undo Step 1
      │
      ▼
Return error
```

---

# 32. Module Cleanup

The cleanup path must remove:

```text
Network hooks
Rule entries
Timers/workqueues
Character device
Device node
Class
Major/minor number
Allocated memory
```

Recommended order:

```text
Stop packet processing
        ↓
Remove network hooks
        ↓
Flush active operations
        ↓
Free rule database
        ↓
Destroy device
        ↓
Destroy class
        ↓
Delete cdev
        ↓
Unregister device number
```

---

# 33. Kernel Driver Build

For an external kernel module, a Makefile can use:

```makefile
obj-m += packet_filter.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Build:

```bash
make
```

Output:

```text
packet_filter.ko
```

For the TDA4VM target, the module must be built against the target kernel configuration and matching kernel build tree.

---

# 34. Cross Compilation

The BeagleBone AI-64 uses a 64-bit ARM processor architecture.

Typical architecture:

```text
Host PC
Ubuntu x86_64
     │
     │ Cross Compiler
     ▼
AArch64
     │
     ▼
TDA4VM
```

Example:

```bash
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
```

Then:

```bash
make ARCH=arm64 \
     CROSS_COMPILE=aarch64-linux-gnu- \
     -C $KERNEL_SRC \
     M=$PWD \
     modules
```

The exact cross-compiler prefix depends on the Yocto SDK/toolchain.

---

# 35. Yocto Integration

The kernel driver should eventually be integrated into the Yocto build rather than manually copying `.ko` files.

Typical structure:

```text
meta-box-storage/
└── recipes-kernel/
    └── packet-filter/
        ├── packet-filter.bb
        ├── files/
        │   ├── packet_filter.c
        │   ├── packet_filter.h
        │   └── Makefile
        └── ...
```

The recipe builds the module and installs it into the target RootFS.

Conceptual flow:

```text
Driver Source
     │
     ▼
BitBake Recipe
     │
     ▼
Kernel Module Compilation
     │
     ▼
packet_filter.ko
     │
     ▼
RootFS
     │
     ▼
BeagleBone AI-64
```

---

# 36. Driver Installation

After installation:

```bash
modprobe packet_filter
```

Check:

```bash
lsmod | grep packet_filter
```

Check device:

```bash
ls -l /dev/packet_filter
```

Check logs:

```bash
dmesg | grep packet_filter
```

---

# 37. Driver Boot Integration

If the driver must load automatically:

```text
Linux Boot
    │
    ▼
RootFS Mounted
    │
    ▼
Module Loading
    │
    ▼
packet_filter.ko
    │
    ▼
Driver Initialization
    │
    ▼
/dev/packet_filter
    │
    ▼
User-Space Daemon
```

The driver can be configured as:

```text
Built-in
```

or:

```text
Loadable Module
```

depending on project requirements.

---

# 38. Device Tree Integration

If the driver is a platform driver and requires board-specific hardware resources, Device Tree can describe the device.

Example:

```dts
packet_filter {
    compatible = "example,packet-filter";
    status = "okay";
};
```

Driver:

```c
static const struct of_device_id pf_of_match[] = {
    {
        .compatible = "example,packet-filter",
    },
    { }
};

MODULE_DEVICE_TABLE(of, pf_of_match);
```

Flow:

```text
Device Tree
     │
     ▼
compatible
     │
     ▼
Driver Match
     │
     ▼
probe()
```

If the packet filter is purely a software network filter, it may not require a dedicated Device Tree node.

---

# 39. Platform Driver Model

If hardware resources are required, a platform driver may be appropriate.

Architecture:

```text
Device Tree
     │
     ▼
Platform Device
     │
     ▼
Driver Match
     │
     ▼
probe()
```

Example:

```c
static struct platform_driver pf_driver = {
    .probe  = pf_probe,
    .remove = pf_remove,

    .driver = {
        .name = "packet_filter",
        .of_match_table = pf_of_match,
    },
};
```

---

# 40. Driver Probe

The `probe()` function initializes a matched device.

Typical sequence:

```text
probe()
  │
  ├── Read Device Tree
  │
  ├── Allocate resources
  │
  ├── Initialize locks
  │
  ├── Initialize rule table
  │
  ├── Initialize statistics
  │
  ├── Register network hooks
  │
  └── Create device interface
```

---

# 41. Complete Driver Initialization

```text
                    Kernel Boot / Module Load
                              │
                              ▼
                     packet_filter_init()
                              │
                 ┌────────────┴────────────┐
                 ▼                         ▼
           Character Device          Network Integration
                 │                         │
                 ▼                         ▼
        /dev/packet_filter            Filter Hook
                 │                         │
                 └────────────┬────────────┘
                              ▼
                       Driver Ready
```

---

# 42. Packet Processing Runtime

Once the driver is initialized:

```text
                         Packet
                           │
                           ▼
                    Network Stack
                           │
                           ▼
                    Filter Hook
                           │
                           ▼
                  Extract Metadata
                           │
                           ▼
                    Search Rules
                           │
              ┌────────────┴────────────┐
              ▼                         ▼
           Match                      No Match
              │                         │
              ▼                         ▼
           Action                  Default Policy
              │
       ┌──────┼──────┐
       ▼      ▼      ▼
     ALLOW   DROP    LOG
```

---

# 43. Configuration Runtime

The configuration files are:

```text
configs/rules/
├── whitelist.conf
├── blacklist.conf
└── monitoring.conf
```

User-space configuration flow:

```text
whitelist.conf
      │
      ▼
Parser
      │
      ▼
IOCTL
      │
      ▼
Kernel Rule Table
```

Same for:

```text
blacklist.conf
monitoring.conf
```

The kernel driver should not normally parse human-readable configuration files directly.

---

# 44. Separation of Responsibilities

Recommended architecture:

```text
User Space
────────────────────────────────
Configuration Parsing
Policy Management
CLI
Logging
Status Display
IOCTL Calls


Kernel Space
────────────────────────────────
Packet Filtering
Rule Storage
Packet Metadata Processing
Statistics
Network Hook
Security Validation
```

This keeps the kernel driver smaller and reduces unnecessary kernel-space complexity.

---

# 45. Testing

## Driver Load Test

```bash
sudo insmod packet_filter.ko
```

Expected:

```text
packet_filter: initialized
```

---

## Device Test

```bash
ls -l /dev/packet_filter
```

Expected:

```text
/dev/packet_filter
```

---

## IOCTL Test

Run:

```bash
./ioctl_test
```

Test:

```text
GET_STATUS
ENABLE
ADD_RULE
GET_RULE
GET_STATS
DELETE_RULE
DISABLE
```

---

# 46. Packet Test

Example test scenario:

```text
Rule:
TCP destination port 23 → DROP
```

Generate traffic toward port 23.

Then:

```bash
dmesg
```

or inspect statistics through the test application.

Expected:

```text
Packets received : N
Packets dropped  : N
```

---

# 47. Network Debugging

Check interfaces:

```bash
ip link
```

Check addresses:

```bash
ip addr
```

Check routes:

```bash
ip route
```

Check Ethernet:

```bash
ethtool eth0
```

Check network messages:

```bash
dmesg | grep -i ethernet
```

Check packet traffic:

```bash
tcpdump -i eth0
```

The exact interface name may differ from `eth0`.

---

# 48. Driver Debugging Flow

When the driver does not work:

```text
Driver Not Working
       │
       ▼
Check Module
       │
       ▼
Check dmesg
       │
       ▼
Check /dev/packet_filter
       │
       ▼
Check IOCTL
       │
       ▼
Check Rule Database
       │
       ▼
Check Network Hook
       │
       ▼
Check Packet Path
       │
       ▼
Check Statistics
       │
       ▼
Check Actual Network Traffic
```

Useful commands:

```bash
lsmod
```

```bash
dmesg
```

```bash
ls -l /dev/packet_filter
```

```bash
ip link
```

```bash
ip addr
```

```bash
ethtool eth0
```

```bash
tcpdump -i eth0
```

---

# 49. Common Driver Problems

## Module does not load

Check:

```bash
dmesg | tail -50
```

Possible causes:

```text
Kernel version mismatch
Invalid module
Missing symbol
Configuration mismatch
Architecture mismatch
```

---

## `/dev/packet_filter` does not exist

Check:

```bash
dmesg | grep packet_filter
```

Possible causes:

```text
cdev_add() failed
class creation failed
device_create() failed
udev/device creation issue
```

---

## IOCTL returns `ENOTTY`

Possible causes:

```text
Unsupported command
Incorrect IOCTL header
Kernel/user header mismatch
Incorrect magic number
Incorrect command number
```

---

## Driver loads but packets are not filtered

Check:

```text
Network hook registration
Interface name
Packet path
Rule database
Rule matching
Filter enable state
```

---

## Packets are incorrectly dropped

Check:

```text
Rule precedence
IP masks
Protocol matching
Port matching
Default policy
Rule parsing
```

---

# 50. Security Considerations

Kernel drivers operate with high privileges.

The driver must:

* Validate every IOCTL.
* Validate all user input.
* Never trust user pointers.
* Use `copy_from_user()`.
* Use `copy_to_user()`.
* Prevent integer overflow.
* Protect shared structures.
* Limit rule count.
* Avoid memory leaks.
* Avoid use-after-free.
* Restrict access to `/dev/packet_filter`.
* Correctly unregister network hooks during cleanup.

A faulty packet-filter driver can affect the entire networking subsystem.

---

# 51. Performance Considerations

Packet filtering occurs on the packet-processing path, so performance is important.

Avoid:

```text
Large linear rule searches
Unnecessary memory allocations
Excessive printk()
Long-held locks
Sleeping in atomic/network contexts
```

For a large rule set, consider:

```text
Hash tables
Prefix trees
Efficient rule indexing
Per-CPU statistics
RPS/RSS awareness
```

The first implementation should prioritize correctness and safe synchronization before optimization.

---

# 52. Performance Flow

```text
Packet
  │
  ▼
Fast Metadata Extraction
  │
  ▼
Efficient Rule Lookup
  │
  ▼
Decision
  │
  ├── ACCEPT
  └── DROP
```

Monitoring/logging should be designed carefully because excessive logging can significantly reduce packet-processing performance.

---

# 53. JTAG Debugging

For low-level kernel debugging, JTAG can be used when supported by the hardware/debug environment.

Typical debugging layers:

```text
Application
    │
    ▼
IOCTL
    │
    ▼
Kernel Driver
    │
    ▼
Network Stack
    │
    ▼
Hardware
```

Kernel logs are normally the first debugging method.

JTAG is useful when:

* Kernel crashes
* Driver deadlocks
* CPU exceptions occur
* Early boot debugging is required
* Kernel execution must be inspected instruction-by-instruction

---

# 54. Kernel Crash Debugging

For crashes, collect:

```bash
dmesg
```

Look for:

```text
Oops
BUG
kernel panic
Call Trace
NULL pointer
general protection fault
```

Important information:

```text
PC
LR
Call Trace
Register State
Module
Source Line
```

The driver should be built with debugging symbols when deep debugging is required.

---

# 55. Complete Project Flow

```text
                     BeagleBone AI-64
                           │
                       TDA4VM SoC
                           │
                           ▼
                    Linux Kernel
                           │
                           ▼
                  Packet Filter Driver
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
         ▼                 ▼                 ▼
   Character Device   Network Hook      Rule Database
         │                 │                 │
         ▼                 │                 │
 /dev/packet_filter       │                 │
         │                 │                 │
         ▼                 ▼                 ▼
   User Application → Packet Filtering ← Rules
         │                 │
         ▼                 ▼
  Configuration        Statistics
      Files                │
         │                 ▼
         └────────────→ Monitoring
```

---

# 56. End-to-End Boot-to-Packet Flow

```text
Power On
   │
   ▼
TI Boot ROM
   │
   ▼
Boot Firmware
   │
   ▼
U-Boot
   │
   ▼
Linux Kernel
   │
   ▼
Device Tree
   │
   ▼
Kernel Driver Initialization
   │
   ▼
packet_filter.ko
   │
   ▼
/dev/packet_filter
   │
   ▼
User-Space Daemon
   │
   ▼
Load whitelist/blacklist
   │
   ▼
IOCTL
   │
   ▼
Kernel Rule Table
   │
   ▼
Ethernet Packet
   │
   ▼
Network Stack
   │
   ▼
Packet Filter
   │
   ├───────────────┐
   ▼               ▼
  ALLOW           DROP
   │               │
   ▼               ▼
Continue         Discard
```

---

# 57. Development Checklist

## Driver

* [ ] Driver source created
* [ ] Kernel module Makefile created
* [ ] Module initialization implemented
* [ ] Module cleanup implemented
* [ ] Character device registered
* [ ] `/dev/packet_filter` created
* [ ] File operations implemented
* [ ] IOCTL handler implemented
* [ ] Rule database implemented
* [ ] Packet matching implemented
* [ ] Statistics implemented
* [ ] Monitoring implemented
* [ ] Network hook integrated
* [ ] Synchronization added
* [ ] Error handling added

## User Space

* [ ] Shared IOCTL header
* [ ] Configuration parser
* [ ] Whitelist support
* [ ] Blacklist support
* [ ] Monitoring support
* [ ] Status reporting
* [ ] IOCTL test application

## Yocto

* [ ] Kernel module recipe
* [ ] Shared header installation
* [ ] Module added to RootFS
* [ ] Configuration files installed
* [ ] User-space application installed
* [ ] Auto-start service configured

## Testing

* [ ] Module load
* [ ] Module unload
* [ ] Device node
* [ ] IOCTL
* [ ] Rule insertion
* [ ] Rule deletion
* [ ] Packet allow
* [ ] Packet drop
* [ ] Statistics
* [ ] Monitoring
* [ ] Stress testing
* [ ] Reboot testing

---

# 58. Final Driver Architecture

The final architecture is:

```text
                         USER SPACE
                              │
              ┌───────────────┴───────────────┐
              │                               │
              ▼                               ▼
      Configuration Files             Test / Control App
              │                               │
              └───────────────┬───────────────┘
                              │
                           ioctl()
                              │
                              ▼
                    /dev/packet_filter
                              │
                              ▼
                         KERNEL SPACE
                              │
                   +----------+----------+
                   │                     │
                   ▼                     ▼
              IOCTL Handler        Network Hook
                   │                     │
                   ▼                     ▼
              Rule Manager          Packet Parser
                   │                     │
                   └──────────┬──────────┘
                              ▼
                       Rule Matching
                              │
                  ┌───────────┼───────────┐
                  ▼           ▼           ▼
                ALLOW        DROP        LOG
                  │           │           │
                  ▼           ▼           ▼
               Network     Discard     Monitor
               Continue
                  │
                  ▼
             Ethernet Driver
                  │
                  ▼
                TDA4VM
                  │
                  ▼
              Ethernet PHY
```

The kernel driver is therefore the **core enforcement layer** of the packet-filter project. User space defines policy, IOCTL transfers that policy into the kernel, and the kernel driver applies the policy to live network traffic while maintaining statistics and monitoring information.

```
```

