# IOCTL API – BeagleBone AI-64 / TI TDA4VM Packet Filter

## 1. Overview

This document defines the user-space to kernel-space interface for controlling the packet-filter driver through the Linux **`ioctl()` API**.

The IOCTL interface provides a controlled mechanism for the packet-filter application to:

* Add filtering rules
* Remove filtering rules
* Query existing rules
* Enable or disable packet filtering
* Configure monitoring
* Retrieve packet statistics
* Clear statistics
* Query driver status

The communication path is:

```text
User Application
      │
      │ ioctl()
      ▼
/dev/packet_filter
      │
      ▼
Packet Filter Driver
      │
      ▼
Kernel Network Stack
      │
      ▼
Ethernet Hardware
```

---

# 2. Why IOCTL Is Used

The packet-filter driver operates in kernel space, while configuration and control applications operate in user space.

A user-space application cannot directly access kernel data structures.

Therefore, an interface is required:

```text
User Space
    │
    │ System Call
    ▼
Kernel Space
    │
    ▼
Driver
```

For this project, `ioctl()` provides commands for control operations that do not naturally fit simple `read()` or `write()` semantics.

---

# 3. Device Node

The driver exposes a character device:

```text
/dev/packet_filter
```

Expected architecture:

```text
packet_filter.c
      │
      ▼
alloc_chrdev_region()
      │
      ▼
cdev_add()
      │
      ▼
class_create()
      │
      ▼
device_create()
      │
      ▼
/dev/packet_filter
```

Verify on the target:

```bash
ls -l /dev/packet_filter
```

Expected output will contain a character-device entry similar to:

```text
crw------- 1 root root ... /dev/packet_filter
```

The exact major/minor numbers depend on the driver implementation.

---

# 4. IOCTL Architecture

The complete control flow is:

```text
+---------------------------+
| User-space Application    |
|                           |
| ioctl(fd, command, data)  |
+-------------+-------------+
              |
              | System Call
              ▼
+---------------------------+
| Linux Kernel              |
|                           |
| sys_ioctl()               |
+-------------+-------------+
              |
              ▼
+---------------------------+
| Packet Filter Driver      |
|                           |
| unlocked_ioctl()          |
+-------------+-------------+
              |
              ▼
+---------------------------+
| Command Handler           |
|                           |
| ADD / DELETE / QUERY      |
| ENABLE / DISABLE / STATS  |
+---------------------------+
```

---

# 5. Header File

The IOCTL command definitions should be shared between the kernel driver and user-space application.

Recommended location:

```text
include/
└── packet_filter_ioctl.h
```

Project structure:

```text
BeagleBone_AI-64–TI_TDA4VM/
├── include/
│   └── packet_filter_ioctl.h
├── driver/
│   ├── packet_filter.c
│   └── packet_filter.h
└── userspace/
    └── packet_filter_daemon.c
```

The shared header prevents command definitions from becoming inconsistent between user space and kernel space.

---

# 6. IOCTL Command Encoding

Linux provides macros for defining IOCTL commands.

Common macros are:

```c
_IO()
_IOR()
_IOW()
_IOWR()
```

Meaning:

| Macro   | Direction        |
| ------- | ---------------- |
| `_IO`   | No data transfer |
| `_IOR`  | Kernel → User    |
| `_IOW`  | User → Kernel    |
| `_IOWR` | User ↔ Kernel    |

For example:

```c
#define PF_IOC_ENABLE \
    _IO(PF_IOC_MAGIC, 1)
```

A command that sends a rule from user space to the kernel can use:

```c
#define PF_IOC_ADD_RULE \
    _IOW(PF_IOC_MAGIC, 2, struct pf_rule)
```

A command that retrieves statistics can use:

```c
#define PF_IOC_GET_STATS \
    _IOR(PF_IOC_MAGIC, 3, struct pf_stats)
```

---

# 7. IOCTL Magic Number

A unique magic number identifies this driver's IOCTL commands.

Example:

```c
#define PF_IOC_MAGIC 'P'
```

The complete command is then generated from:

```text
Magic Number
      +
Command Number
      +
Data Direction
      +
Data Size
```

---

# 8. Rule Data Structure

A rule represents a filtering policy.

Example structure:

```c
struct pf_rule {
    uint32_t id;
    uint32_t action;
    uint32_t protocol;

    uint32_t src_ip;
    uint32_t src_mask;

    uint32_t dst_ip;
    uint32_t dst_mask;

    uint16_t src_port;
    uint16_t dst_port;
};
```

The actual structure should remain synchronized between the kernel driver and user-space application.

---

# 9. Rule Actions

Define filtering actions:

```c
#define PF_ACTION_ALLOW  1
#define PF_ACTION_DROP   2
#define PF_ACTION_LOG    3
```

Conceptually:

```text
                 Packet
                    │
                    ▼
                Rule Match
                    │
          ┌─────────┼─────────┐
          ▼         ▼         ▼
        ALLOW      DROP      LOG
          │         │         │
          ▼         ▼         ▼
        Accept    Reject    Monitor
```

---

# 10. Protocol Definitions

Example:

```c
#define PF_PROTO_ANY  0
#define PF_PROTO_TCP  6
#define PF_PROTO_UDP  17
#define PF_PROTO_ICMP 1
```

These values correspond to protocol identifiers used by the networking stack.

---

# 11. Statistics Structure

The driver can expose packet statistics through IOCTL.

Example:

```c
struct pf_stats {
    uint64_t packets_received;
    uint64_t packets_allowed;
    uint64_t packets_dropped;
    uint64_t packets_logged;
    uint64_t bytes_received;
    uint64_t bytes_dropped;
};
```

The application can request this information:

```text
User Space
    │
    │ PF_IOC_GET_STATS
    ▼
Kernel Driver
    │
    ▼
Statistics
    │
    ▼
User Space
```

---

# 12. Proposed IOCTL Commands

The packet-filter API can define the following commands:

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

---

# 13. Command Table

| Command                  | Direction     | Purpose                  |
| ------------------------ | ------------- | ------------------------ |
| `PF_IOC_ENABLE`          | None          | Enable packet filtering  |
| `PF_IOC_DISABLE`         | None          | Disable packet filtering |
| `PF_IOC_ADD_RULE`        | User → Kernel | Add filtering rule       |
| `PF_IOC_DELETE_RULE`     | User → Kernel | Delete rule              |
| `PF_IOC_GET_RULE`        | Kernel → User | Retrieve rule            |
| `PF_IOC_CLEAR_RULES`     | None          | Remove all rules         |
| `PF_IOC_GET_STATS`       | Kernel → User | Retrieve statistics      |
| `PF_IOC_CLEAR_STATS`     | None          | Reset statistics         |
| `PF_IOC_ENABLE_MONITOR`  | None          | Enable monitoring        |
| `PF_IOC_DISABLE_MONITOR` | None          | Disable monitoring       |
| `PF_IOC_GET_STATUS`      | Kernel → User | Get driver status        |

---

# 14. IOCTL Definitions

A possible shared header is:

```c
#ifndef PACKET_FILTER_IOCTL_H
#define PACKET_FILTER_IOCTL_H

#include <stdint.h>
#include <linux/ioctl.h>

#define PF_IOC_MAGIC 'P'

#define PF_ACTION_ALLOW  1
#define PF_ACTION_DROP   2
#define PF_ACTION_LOG    3

#define PF_PROTO_ANY     0
#define PF_PROTO_TCP     6
#define PF_PROTO_UDP     17
#define PF_PROTO_ICMP    1

struct pf_rule {
    uint32_t id;
    uint32_t action;
    uint32_t protocol;

    uint32_t src_ip;
    uint32_t src_mask;

    uint32_t dst_ip;
    uint32_t dst_mask;

    uint16_t src_port;
    uint16_t dst_port;
};

struct pf_stats {
    uint64_t packets_received;
    uint64_t packets_allowed;
    uint64_t packets_dropped;
    uint64_t packets_logged;
    uint64_t bytes_received;
    uint64_t bytes_dropped;
};

struct pf_status {
    uint32_t enabled;
    uint32_t monitoring_enabled;
    uint32_t rule_count;
};

#define PF_IOC_ENABLE \
    _IO(PF_IOC_MAGIC, 1)

#define PF_IOC_DISABLE \
    _IO(PF_IOC_MAGIC, 2)

#define PF_IOC_ADD_RULE \
    _IOW(PF_IOC_MAGIC, 3, struct pf_rule)

#define PF_IOC_DELETE_RULE \
    _IOW(PF_IOC_MAGIC, 4, uint32_t)

#define PF_IOC_GET_RULE \
    _IOWR(PF_IOC_MAGIC, 5, struct pf_rule)

#define PF_IOC_CLEAR_RULES \
    _IO(PF_IOC_MAGIC, 6)

#define PF_IOC_GET_STATS \
    _IOR(PF_IOC_MAGIC, 7, struct pf_stats)

#define PF_IOC_CLEAR_STATS \
    _IO(PF_IOC_MAGIC, 8)

#define PF_IOC_ENABLE_MONITOR \
    _IO(PF_IOC_MAGIC, 9)

#define PF_IOC_DISABLE_MONITOR \
    _IO(PF_IOC_MAGIC, 10)

#define PF_IOC_GET_STATUS \
    _IOR(PF_IOC_MAGIC, 11, struct pf_status)

#endif
```

This is a **proposed project API**; the final command set should match the actual driver implementation.

---

# 15. Opening the Device

The user-space application first opens the character device:

```c
int fd;

fd = open("/dev/packet_filter", O_RDWR);

if (fd < 0) {
    perror("open");
    return -1;
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
/dev/packet_filter
    │
    ▼
Driver open()
```

---

# 16. Enable Packet Filtering

User space:

```c
if (ioctl(fd, PF_IOC_ENABLE) < 0) {
    perror("PF_IOC_ENABLE");
}
```

Kernel flow:

```text
ioctl()
  │
  ▼
packet_filter_ioctl()
  │
  ▼
PF_IOC_ENABLE
  │
  ▼
filter_enabled = true
```

---

# 17. Disable Packet Filtering

User space:

```c
if (ioctl(fd, PF_IOC_DISABLE) < 0) {
    perror("PF_IOC_DISABLE");
}
```

Kernel:

```text
PF_IOC_DISABLE
      │
      ▼
Disable filtering
      │
      ▼
Packets bypass filtering logic
```

The exact behavior should be clearly defined by the driver.

---

# 18. Adding a Rule

Create a rule:

```c
struct pf_rule rule = {0};

rule.id = 1;
rule.action = PF_ACTION_DROP;
rule.protocol = PF_PROTO_TCP;
rule.dst_port = 23;
```

Send it to the driver:

```c
if (ioctl(fd, PF_IOC_ADD_RULE, &rule) < 0) {
    perror("PF_IOC_ADD_RULE");
}
```

Flow:

```text
User Application
      │
      │ struct pf_rule
      ▼
ioctl()
      │
      ▼
Kernel
      │
      ▼
copy_from_user()
      │
      ▼
Validate Rule
      │
      ▼
Add Rule
```

---

# 19. Deleting a Rule

Example:

```c
uint32_t rule_id = 1;

if (ioctl(fd, PF_IOC_DELETE_RULE, &rule_id) < 0) {
    perror("PF_IOC_DELETE_RULE");
}
```

Flow:

```text
Rule ID
   │
   ▼
ioctl()
   │
   ▼
Kernel
   │
   ▼
Find Rule
   │
   ▼
Delete Rule
```

---

# 20. Getting a Rule

The application can request an existing rule:

```c
struct pf_rule rule = {0};

rule.id = 1;

if (ioctl(fd, PF_IOC_GET_RULE, &rule) < 0) {
    perror("PF_IOC_GET_RULE");
}
```

The kernel fills the structure and returns it to user space.

```text
Kernel
  │
  ▼
Rule Database
  │
  ▼
Requested Rule
  │
  ▼
copy_to_user()
  │
  ▼
Application
```

---

# 21. Clearing All Rules

```c
if (ioctl(fd, PF_IOC_CLEAR_RULES) < 0) {
    perror("PF_IOC_CLEAR_RULES");
}
```

Flow:

```text
PF_IOC_CLEAR_RULES
        │
        ▼
Kernel Rule Table
        │
        ▼
Remove All Rules
```

This operation should normally require appropriate privileges.

---

# 22. Reading Statistics

```c
struct pf_stats stats;

if (ioctl(fd, PF_IOC_GET_STATS, &stats) < 0) {
    perror("PF_IOC_GET_STATS");
}
```

Display:

```c
printf("Received : %llu\n",
       (unsigned long long)stats.packets_received);

printf("Allowed  : %llu\n",
       (unsigned long long)stats.packets_allowed);

printf("Dropped  : %llu\n",
       (unsigned long long)stats.packets_dropped);
```

---

# 23. Clearing Statistics

```c
if (ioctl(fd, PF_IOC_CLEAR_STATS) < 0) {
    perror("PF_IOC_CLEAR_STATS");
}
```

The driver resets the counters:

```text
packets_received = 0
packets_allowed  = 0
packets_dropped  = 0
packets_logged   = 0
```

---

# 24. Monitoring Control

Enable monitoring:

```c
ioctl(fd, PF_IOC_ENABLE_MONITOR);
```

Disable monitoring:

```c
ioctl(fd, PF_IOC_DISABLE_MONITOR);
```

Architecture:

```text
Packet
  │
  ▼
Rule Evaluation
  │
  ▼
Monitoring Enabled?
  │
  ├── YES → Generate Event
  │
  └── NO  → Continue
```

---

# 25. Getting Driver Status

Example:

```c
struct pf_status status;

if (ioctl(fd, PF_IOC_GET_STATUS, &status) < 0) {
    perror("PF_IOC_GET_STATUS");
}
```

Possible output:

```text
Packet Filter : Enabled
Monitoring     : Enabled
Rules          : 5
```

---

# 26. Kernel IOCTL Handler

The driver can implement an IOCTL handler through `file_operations`.

Conceptually:

```c
static const struct file_operations pf_fops = {
    .owner          = THIS_MODULE,
    .open           = pf_open,
    .release        = pf_release,
    .unlocked_ioctl = pf_ioctl,
};
```

The main handler:

```c
static long pf_ioctl(struct file *file,
                     unsigned int cmd,
                     unsigned long arg)
{
    switch (cmd) {

    case PF_IOC_ENABLE:
        break;

    case PF_IOC_DISABLE:
        break;

    case PF_IOC_ADD_RULE:
        break;

    case PF_IOC_DELETE_RULE:
        break;

    case PF_IOC_GET_STATS:
        break;

    default:
        return -EINVAL;
    }

    return 0;
}
```

---

# 27. Safe User/Kernel Data Transfer

Kernel code must not directly dereference user-space pointers.

Use:

```c
copy_from_user()
```

for:

```text
User → Kernel
```

and:

```c
copy_to_user()
```

for:

```text
Kernel → User
```

Example:

```c
struct pf_rule rule;

if (copy_from_user(&rule,
                   (void __user *)arg,
                   sizeof(rule))) {
    return -EFAULT;
}
```

For returning data:

```c
if (copy_to_user((void __user *)arg,
                 &stats,
                 sizeof(stats))) {
    return -EFAULT;
}
```

---

# 28. IOCTL Validation

Every IOCTL input must be validated.

For a rule:

```text
User Input
    │
    ▼
Check Command
    │
    ▼
Copy From User
    │
    ▼
Validate Structure
    │
    ├── Invalid → -EINVAL
    │
    └── Valid
          │
          ▼
      Apply Rule
```

Validate:

* Rule ID
* Action
* Protocol
* IP address
* IP mask
* Port
* Rule limits
* Reserved fields

---

# 29. Error Codes

Common error codes include:

| Error     | Meaning                   |
| --------- | ------------------------- |
| `-EINVAL` | Invalid argument          |
| `-EFAULT` | Invalid user-space memory |
| `-ENOMEM` | Memory allocation failure |
| `-ENOENT` | Rule not found            |
| `-EEXIST` | Rule already exists       |
| `-EPERM`  | Permission denied         |
| `-ENOTTY` | Unsupported IOCTL         |

Example:

```c
return -EINVAL;
```

for an invalid rule.

---

# 30. Permissions

The device node should normally be restricted because IOCTL commands can modify kernel filtering behavior.

Check:

```bash
ls -l /dev/packet_filter
```

Example:

```text
crw------- 1 root root ... /dev/packet_filter
```

A production deployment can use a dedicated group if non-root administration is required.

---

# 31. IOCTL API and Configuration Files

The project has two configuration mechanisms:

```text
Static/Runtime Configuration
        │
        ▼
configs/rules/
├── whitelist.conf
├── blacklist.conf
└── monitoring.conf

        │
        ▼
User-Space Daemon
        │
        ▼
IOCTL
        │
        ▼
Kernel Driver
```

This allows configuration files to be translated into IOCTL operations.

For example:

```text
blacklist.conf
      │
      ▼
Parser
      │
      ▼
struct pf_rule
      │
      ▼
PF_IOC_ADD_RULE
      │
      ▼
Kernel Rule Table
```

---

# 32. Example User-Space Flow

A packet-filter daemon can perform:

```text
Start
 │
 ▼
open("/dev/packet_filter")
 │
 ▼
Read whitelist.conf
 │
 ▼
Add whitelist rules
 │
 ▼
Read blacklist.conf
 │
 ▼
Add blacklist rules
 │
 ▼
Read monitoring.conf
 │
 ▼
Configure monitoring
 │
 ▼
Enable filtering
 │
 ▼
Monitor status/statistics
 │
 ▼
Shutdown
```

---

# 33. Example Complete Sequence

```c
int fd;
struct pf_rule rule = {0};
struct pf_stats stats;

fd = open("/dev/packet_filter", O_RDWR);

if (fd < 0)
    return -1;

ioctl(fd, PF_IOC_ENABLE);

rule.id = 1;
rule.action = PF_ACTION_DROP;
rule.protocol = PF_PROTO_TCP;
rule.dst_port = 23;

ioctl(fd, PF_IOC_ADD_RULE, &rule);

ioctl(fd, PF_IOC_GET_STATS, &stats);

ioctl(fd, PF_IOC_DISABLE);

close(fd);
```

The actual production application should check the return value of every system call.

---

# 34. Complete IOCTL Data Flow

```text
                  USER SPACE
+-------------------------------------------+
| packet_filter_daemon                      |
|                                           |
| whitelist.conf                            |
| blacklist.conf                            |
| monitoring.conf                           |
+----------------------+--------------------+
                       |
                       | ioctl()
                       ▼
                  KERNEL SPACE
+-------------------------------------------+
| /dev/packet_filter                        |
|                                           |
| IOCTL Handler                             |
|       │                                   |
|       ├── ENABLE                           |
|       ├── DISABLE                          |
|       ├── ADD_RULE                         |
|       ├── DELETE_RULE                      |
|       ├── GET_RULE                         |
|       ├── GET_STATS                        |
|       └── MONITOR                          |
|                                           |
| Rule Table                                |
| Statistics                                |
+----------------------+--------------------+
                       |
                       ▼
+-------------------------------------------+
| Linux Network Stack                       |
+----------------------+--------------------+
                       |
                       ▼
+-------------------------------------------+
| Ethernet Driver / TDA4VM                  |
+-------------------------------------------+
```

---

# 35. Thread Safety

If multiple user-space processes or threads access the device, the driver must protect shared state.

Potential shared resources:

```text
Rule Table
Statistics
Configuration
Filter State
Monitoring State
```

Kernel synchronization mechanisms may include:

```c
mutex
spinlock
rwlock
atomic_t
```

For example:

```text
IOCTL
  │
  ▼
Acquire Lock
  │
  ▼
Modify Rule Table
  │
  ▼
Release Lock
```

The appropriate synchronization primitive depends on the context in which the data is accessed.

---

# 36. IOCTL Testing

The IOCTL interface should be tested independently before integrating it with the complete packet-filter daemon.

Test sequence:

```text
1. Open device
2. Get status
3. Enable filtering
4. Add rule
5. Query rule
6. Generate traffic
7. Get statistics
8. Delete rule
9. Clear rules
10. Disable filtering
11. Close device
```

---

# 37. IOCTL Test Matrix

| Test                 | Expected Result         |
| -------------------- | ----------------------- |
| Open device          | Success                 |
| Invalid command      | `-ENOTTY`               |
| Enable               | Filtering enabled       |
| Disable              | Filtering disabled      |
| Add valid rule       | Rule added              |
| Add duplicate rule   | Error                   |
| Delete existing rule | Rule removed            |
| Delete unknown rule  | `-ENOENT`               |
| Get rule             | Correct rule returned   |
| Clear rules          | Rule table empty        |
| Get stats            | Valid counters returned |
| Clear stats          | Counters reset          |
| Enable monitoring    | Monitoring enabled      |
| Disable monitoring   | Monitoring disabled     |

---

# 38. Debugging IOCTL

Check device:

```bash
ls -l /dev/packet_filter
```

Check driver:

```bash
lsmod | grep packet_filter
```

Check kernel messages:

```bash
dmesg | grep -i packet_filter
```

For detailed IOCTL debugging, add messages around command processing:

```c
pr_debug("IOCTL command: %u\n", cmd);
```

Then inspect:

```bash
dmesg | grep -i ioctl
```

A useful debug flow is:

```text
Application
    │
    ▼
ioctl()
    │
    ▼
Driver IOCTL Handler
    │
    ▼
Command Identified
    │
    ▼
Data Copied
    │
    ▼
Data Validated
    │
    ▼
Operation Executed
    │
    ▼
Return Result
```

---

# 39. Security Considerations

IOCTL interfaces execute privileged kernel operations and therefore require careful validation.

The driver must:

* Validate every command.
* Validate command arguments.
* Use `copy_from_user()`.
* Use `copy_to_user()`.
* Check buffer sizes.
* Avoid trusting user-provided pointers.
* Prevent integer overflow.
* Limit the number of rules.
* Protect shared data.
* Restrict device permissions.
* Reject unsupported commands.

Never allow an arbitrary user-space value to directly become a kernel pointer or unrestricted hardware address.

---

# 40. API Versioning

The IOCTL API should be versioned if the project is expected to evolve.

Example:

```c
#define PF_API_VERSION_MAJOR 1
#define PF_API_VERSION_MINOR 0
```

A future API can then distinguish incompatible changes.

For example:

```text
v1.0
  │
  ├── ADD_RULE
  ├── DELETE_RULE
  └── GET_STATS

v2.0
  │
  ├── ADD_RULE
  ├── DELETE_RULE
  ├── GET_STATS
  └── Advanced Rule Matching
```

---

# 41. Recommended Project Structure

```text
BeagleBone_AI-64–TI_TDA4VM/
│
├── include/
│   └── packet_filter_ioctl.h
│
├── driver/
│   ├── packet_filter.c
│   ├── packet_filter.h
│   └── Makefile
│
├── userspace/
│   ├── packet_filter_daemon.c
│   ├── ioctl_test.c
│   └── Makefile
│
├── configs/
│   └── rules/
│       ├── whitelist.conf
│       ├── blacklist.conf
│       └── monitoring.conf
│
└── docs/
    └── ioctl-api.md
```

---

# 42. End-to-End Architecture

```text
                     USER SPACE
                          │
              +-----------+-----------+
              │                       │
              ▼                       ▼
      packet_filter_daemon       ioctl_test
              │                       │
              └───────────┬───────────┘
                          │
                       ioctl()
                          │
                          ▼
                    /dev/packet_filter
                          │
                          ▼
                     KERNEL SPACE
                          │
                  +-------+-------+
                  │               │
                  ▼               ▼
             IOCTL Handler    Rule Table
                  │               │
                  └───────┬───────┘
                          │
                          ▼
                  Packet Filter Logic
                          │
                          ▼
                    Linux Network
                       Stack
                          │
                          ▼
                  Ethernet Driver
                          │
                          ▼
                       TDA4VM
                          │
                          ▼
                       Ethernet
```

---

# 43. Summary

The IOCTL API provides the control boundary between the packet-filter user-space application and the Linux kernel driver.

The fundamental flow is:

```text
Configuration
     ↓
User-Space Daemon
     ↓
open("/dev/packet_filter")
     ↓
ioctl()
     ↓
Kernel IOCTL Handler
     ↓
Validate Input
     ↓
Update Rule/State
     ↓
Packet Filtering
     ↓
Statistics / Monitoring
```

The key interfaces are:

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

The IOCTL interface should remain a **small, well-defined, validated API** between user space and the kernel. The shared header should be the single source of truth for command numbers and data structures.

