# `docs/userspace.md`

````markdown
# User-Space Application – BeagleBone AI-64 / TI TDA4VM

## 1. Overview

This document describes the user-space architecture of the packet-filtering
system running on the BeagleBone AI-64 based on the TI TDA4VM SoC.

The user-space application provides the interface between the administrator
and the kernel packet-filter driver.

The main responsibilities are:

- Configure packet-filter rules
- Add whitelist rules
- Add blacklist rules
- Configure monitoring rules
- Delete rules
- Clear rules
- Enable/disable packet filtering
- Read packet-filter statistics
- Monitor packet activity
- Validate driver status
- Provide command-line control
- Handle configuration errors

The overall architecture is:

```text
+------------------------------------------------------+
|                  USER SPACE                          |
|                                                      |
|  +------------------+       +---------------------+  |
|  | CLI Application  |       | Configuration Files |  |
|  +--------+---------+       +----------+----------+  |
|           |                            |             |
|           +-------------+--------------+             |
|                         |                            |
|                         ▼                            |
|              +-----------------------+              |
|              | User-Space Controller |              |
|              +-----------+-----------+              |
+--------------------------|---------------------------+
                           |
                           | IOCTL
                           ▼
+------------------------------------------------------+
|                  KERNEL SPACE                        |
|                                                      |
|             Packet Filter Driver                     |
|                    │                                 |
|                    ▼                                 |
|             Rule Database                            |
|                    │                                 |
|                    ▼                                 |
|             Packet Processing                        |
+------------------------------------------------------+
                           |
                           ▼
                      Ethernet
````

---

# 2. User-Space Responsibilities

The user-space application should not directly manipulate packet data in the
kernel networking path.

Instead, it controls the packet-filter configuration through the driver's
interface.

```text
User Space
    │
    ├── Rule configuration
    ├── Driver control
    ├── Statistics
    └── Monitoring
          │
          ▼
       IOCTL
          │
          ▼
Kernel Driver
```

---

# 3. Directory Structure

Recommended project structure:

```text
BeagleBone_AI-64–TI_TDA4VM/
│
├── userspace/
│   ├── Makefile
│   ├── README.md
│   │
│   ├── include/
│   │   ├── packet_filter.h
│   │   ├── ioctl.h
│   │   └── config.h
│   │
│   ├── src/
│   │   ├── main.c
│   │   ├── ioctl.c
│   │   ├── rules.c
│   │   ├── config.c
│   │   ├── monitor.c
│   │   └── statistics.c
│   │
│   └── tests/
│       ├── test_ioctl.c
│       ├── test_rules.c
│       └── test_config.c
│
├── configs/
│   └── rules/
│       ├── whitelist.conf
│       ├── blacklist.conf
│       └── monitoring.conf
│
└── docs/
    └── userspace.md
```

---

# 4. User-Space Components

The application can be divided into:

```text
+-------------------------+
|         main.c          |
+------------+------------+
             |
             ▼
+-------------------------+
|       Command Parser    |
+------------+------------+
             |
             ▼
+-------------------------+
|     Configuration       |
+------------+------------+
             |
             ▼
+-------------------------+
|      Rule Manager       |
+------------+------------+
             |
             ▼
+-------------------------+
|      IOCTL Manager      |
+------------+------------+
             |
             ▼
+-------------------------+
|      Kernel Driver      |
+-------------------------+
```

---

# 5. Main Application

`main.c` is the entry point.

Responsibilities:

```text
main()
 │
 ├── Parse command line
 │
 ├── Validate arguments
 │
 ├── Open driver
 │
 ├── Execute requested operation
 │
 ├── Display result
 │
 └── Close driver
```

Example:

```c
int main(int argc, char *argv[])
{
    int fd;

    fd = open("/dev/packet_filter", O_RDWR);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Process command */

    close(fd);

    return 0;
}
```

---

# 6. Device File

The user-space application communicates with the driver through a device node.

Example:

```text
/dev/packet_filter
```

Check:

```bash
ls -l /dev/packet_filter
```

Open:

```c
int fd;

fd = open("/dev/packet_filter", O_RDWR);
```

If the open operation fails:

```c
perror("open");
```

The application must terminate gracefully.

---

# 7. Device Access Flow

```text
Application
     │
     ▼
open("/dev/packet_filter")
     │
     ▼
VFS
     │
     ▼
Character Device
     │
     ▼
Packet Filter Driver
```

After opening the device:

```text
fd
 │
 ├── ioctl()
 ├── read()       if supported
 ├── write()      if supported
 └── close()
```

---

# 8. IOCTL Architecture

IOCTL provides control operations between user space and kernel space.

```text
User Application
      │
      │ ioctl(fd, command, argument)
      ▼
VFS
      │
      ▼
Driver ioctl handler
      │
      ▼
Packet Filter
```

Typical commands:

```text
ADD_RULE
DELETE_RULE
GET_RULE
CLEAR_RULES
GET_STATS
ENABLE
DISABLE
GET_STATUS
```

The exact command names and numbers must match the kernel driver's
implementation.

---

# 9. Shared IOCTL Structures

Kernel and user space must use compatible structures.

Example:

```c
struct pf_rule {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;
    uint8_t  action;
};
```

Important:

```text
User-space structure
        │
        ▼
Must match
        │
        ▼
Kernel structure
```

Avoid independently changing one side.

---

# 10. User-to-Kernel Data Flow

Example: add rule.

```text
User
 │
 │ struct pf_rule
 ▼
ioctl()
 │
 ▼
Kernel
 │
 ├── Validate input
 ├── copy_from_user()
 ├── Validate rule
 └── Add rule
```

The kernel must never blindly trust user-space input.

---

# 11. Kernel-to-User Data Flow

Example: statistics.

```text
User
 │
 │ ioctl(GET_STATS)
 ▼
Kernel
 │
 ├── Collect statistics
 ├── Prepare structure
 └── copy_to_user()
 │
 ▼
User
 │
 ▼
Display statistics
```

---

# 12. User-Space Command Interface

Recommended command structure:

```bash
packet-filter <command> [options]
```

Examples:

```bash
packet-filter status
packet-filter enable
packet-filter disable
packet-filter add-rule ...
packet-filter delete-rule ...
packet-filter list-rules
packet-filter clear-rules
packet-filter stats
packet-filter load-config ...
```

---

# 13. Status Command

Example:

```bash
packet-filter status
```

Expected output:

```text
Packet Filter Status
--------------------
Driver        : Loaded
Filter        : Enabled
Rules         : 25
Packets RX    : 100000
Allowed       : 85000
Dropped       : 15000
```

---

# 14. Enable Command

```bash
packet-filter enable
```

Flow:

```text
CLI
 │
 ▼
open()
 │
 ▼
ioctl(ENABLE)
 │
 ▼
Kernel Driver
 │
 ▼
Filter Enabled
```

Expected:

```text
Packet filter enabled successfully
```

---

# 15. Disable Command

```bash
packet-filter disable
```

Flow:

```text
CLI
 │
 ▼
ioctl(DISABLE)
 │
 ▼
Kernel Driver
 │
 ▼
Filter Disabled
```

The driver should continue to operate safely while filtering is disabled.

---

# 16. Add Rule

Example conceptual command:

```bash
packet-filter add-rule \
    --src 192.168.1.100 \
    --dst 192.168.1.50 \
    --protocol tcp \
    --dport 443 \
    --action allow
```

Flow:

```text
Command
   │
   ▼
Argument Parser
   │
   ▼
Rule Structure
   │
   ▼
Validation
   │
   ▼
IOCTL
   │
   ▼
Kernel Rule Database
```

---

# 17. Delete Rule

Example:

```bash
packet-filter delete-rule --id 10
```

Flow:

```text
Rule ID
  │
  ▼
Argument validation
  │
  ▼
ioctl(DELETE_RULE)
  │
  ▼
Kernel
  │
  ▼
Rule removed
```

---

# 18. List Rules

Example:

```bash
packet-filter list-rules
```

Possible output:

```text
ID   TYPE       PROTOCOL   SRC              DST       ACTION
------------------------------------------------------------
1    WHITELIST  TCP        192.168.1.10     ANY       ALLOW
2    BLACKLIST  TCP        10.0.0.10        ANY       DROP
3    MONITOR    UDP        ANY              ANY       LOG
```

The actual output should match the implemented rule model.

---

# 19. Clear Rules

```bash
packet-filter clear-rules
```

Flow:

```text
CLI
 │
 ▼
ioctl(CLEAR_RULES)
 │
 ▼
Kernel
 │
 ▼
Rule database cleared
```

The application should request confirmation for destructive operations if
appropriate.

---

# 20. Statistics Command

```bash
packet-filter stats
```

Possible output:

```text
Packet Filter Statistics
------------------------

Packets Received : 1000000
Packets Allowed  : 850000
Packets Dropped  : 150000
Rules Matched    : 950000
Monitor Events   : 12000
```

---

# 21. Statistics Flow

```text
User Space
     │
     ▼
GET_STATS
     │
     ▼
Kernel Driver
     │
     ▼
Per-CPU / Global Counters
     │
     ▼
Aggregate
     │
     ▼
copy_to_user()
     │
     ▼
CLI
```

---

# 22. Configuration Files

The project contains:

```text
configs/rules/
├── whitelist.conf
├── blacklist.conf
└── monitoring.conf
```

These files define the initial rule configuration.

Recommended flow:

```text
Configuration File
       │
       ▼
User-Space Parser
       │
       ▼
Validation
       │
       ▼
Rule Structure
       │
       ▼
IOCTL
       │
       ▼
Kernel Rule Database
```

---

# 23. Whitelist Configuration

Example:

```text
# Allow HTTPS
tcp,any,192.168.1.50,443,allow

# Allow SSH from management host
tcp,192.168.1.10,any,22,allow
```

The exact syntax must remain consistent with the parser implementation.

---

# 24. Blacklist Configuration

Example:

```text
# Block specific source
tcp,192.168.1.100,any,any,drop

# Block destination port
tcp,any,any,23,drop
```

---

# 25. Monitoring Configuration

Example:

```text
# Monitor SSH traffic
tcp,any,any,22,monitor

# Monitor UDP traffic
udp,any,any,any,monitor
```

Monitoring behavior must be clearly separated from the actual packet action.

---

# 26. Configuration Parser

The configuration parser performs:

```text
Read file
   │
   ▼
Read line
   │
   ▼
Remove comments
   │
   ▼
Tokenize
   │
   ▼
Validate fields
   │
   ▼
Convert values
   │
   ▼
Build rule structure
   │
   ▼
Send to kernel
```

---

# 27. Configuration Validation

Validate:

```text
IP address
Subnet
Port
Protocol
Action
Rule type
Rule ID
Field count
```

Invalid:

```text
999.999.999.999
```

must be rejected before sending the rule to the kernel.

---

# 28. Error Handling

The user-space application should handle:

```text
open() failure
ioctl() failure
invalid command
invalid arguments
invalid configuration
driver unavailable
permission denied
rule not found
duplicate rule
maximum rule count
```

Example:

```c
if (ioctl(fd, PF_IOCTL_ADD_RULE, &rule) < 0) {
    perror("PF_IOCTL_ADD_RULE");
    return -1;
}
```

---

# 29. errno Handling

Typical errors:

```text
EINVAL
ENOENT
EFAULT
ENOMEM
EBUSY
EPERM
ENODEV
```

Example:

```c
if (ret < 0) {
    fprintf(stderr,
            "IOCTL failed: %s\n",
            strerror(errno));
}
```

Do not expose meaningless numeric errors to the user without context.

---

# 30. Permission Handling

The device node may require elevated privileges.

Example:

```bash
sudo packet-filter status
```

If permission is denied:

```text
Error: Permission denied accessing /dev/packet_filter
```

The application should not attempt unsafe privilege escalation itself.

---

# 31. Root vs Non-Root Access

Recommended model:

```text
Normal user
    │
    ├── Read status
    └── Read statistics

Administrator
    │
    ├── Add rules
    ├── Delete rules
    ├── Enable
    ├── Disable
    └── Clear rules
```

The exact permission model should be enforced by the device node and driver.

---

# 32. User-Space Monitoring

Monitoring can run as:

```bash
packet-filter monitor
```

Flow:

```text
Kernel
  │
  ▼
Monitoring Event
  │
  ▼
Driver Interface
  │
  ▼
User-Space Monitor
  │
  ▼
Console / Log
```

Example output:

```text
[10:32:15] TCP 192.168.1.100:443 -> 192.168.1.50:443 ALLOW
[10:32:16] TCP 10.0.0.10:22 -> 192.168.1.50:22 DROP
```

---

# 33. Monitoring Performance

The monitoring path must not unnecessarily slow packet processing.

Preferred architecture:

```text
Packet Path
     │
     ▼
Filter
     │
     ├────────► Decision
     │
     └────────► Event Queue
                    │
                    ▼
               User Space
```

Avoid blocking packet processing on console output.

---

# 34. Logging

User-space logs should distinguish:

```text
INFO
WARNING
ERROR
DEBUG
```

Example:

```text
INFO: Packet filter enabled
INFO: Rule 10 added
WARNING: Rule already exists
ERROR: Failed to communicate with driver
DEBUG: Received statistics
```

---

# 35. Log File

Optional log location:

```text
/var/log/packet-filter.log
```

Example:

```text
2026-08-19 10:00:01 INFO Driver opened
2026-08-19 10:00:02 INFO Filter enabled
2026-08-19 10:00:05 INFO Rule 1 added
```

Log rotation should be considered for long-running monitoring applications.

---

# 36. Application Startup

A startup sequence can be:

```text
Application Start
      │
      ▼
Parse Arguments
      │
      ▼
Open /dev/packet_filter
      │
      ▼
Check Driver Status
      │
      ▼
Load Configuration
      │
      ▼
Validate Rules
      │
      ▼
Apply Rules
      │
      ▼
Enable Filter
      │
      ▼
Ready
```

---

# 37. Configuration Reload

Configuration can be reloaded without rebooting.

Example:

```bash
packet-filter reload
```

Flow:

```text
New Configuration
       │
       ▼
Parse
       │
       ▼
Validate
       │
       ▼
Create New Rule Set
       │
       ▼
Send to Kernel
       │
       ▼
Activate
```

A failed configuration should not leave the driver in a partially updated
state.

---

# 38. Atomic Configuration Update

Preferred conceptual model:

```text
Current Rules
     │
     │
     ▼
Build New Rules
     │
     ▼
Validate Completely
     │
     ▼
Activate New Rules
     │
     ▼
Replace Current Rules
```

Avoid:

```text
Delete Rule 1
Add Rule 1
Delete Rule 2
...
```

when a partially applied configuration could create an unsafe intermediate
state.

The actual atomicity mechanism belongs to the kernel implementation.

---

# 39. CLI Help

The application should provide:

```bash
packet-filter --help
```

Example:

```text
Usage:
  packet-filter <command> [options]

Commands:
  status
  enable
  disable
  add-rule
  delete-rule
  list-rules
  clear-rules
  load-config
  reload
  stats
  monitor
```

---

# 40. CLI Version

Provide:

```bash
packet-filter --version
```

Example:

```text
packet-filter version 1.0.0
Driver API version 1
```

The application and kernel driver API versions should be compatible.

---

# 41. API Versioning

Define a version:

```c
#define PF_API_VERSION 1
```

The application can query the driver:

```text
GET_API_VERSION
```

Flow:

```text
User Application
      │
      ▼
GET_API_VERSION
      │
      ▼
Kernel Driver
      │
      ▼
API Version
```

This helps prevent incompatible user/kernel binaries.

---

# 42. ABI Compatibility

Structures passed through IOCTL form part of the user/kernel ABI.

Consider:

```text
Data type sizes
Structure alignment
Structure padding
32-bit vs 64-bit
Endianness
```

Use fixed-width types:

```c
uint32_t
uint16_t
uint8_t
```

rather than relying on implementation-dependent types.

---

# 43. 32-bit / 64-bit Considerations

The BeagleBone AI-64 runs a 64-bit ARM architecture.

Avoid passing raw user-space pointers inside persistent kernel structures.

If pointers must be passed through an IOCTL interface, they must be handled
carefully using the appropriate kernel/user ABI mechanisms.

Prefer fixed-size data structures.

---

# 44. Security Considerations

User-space input is untrusted.

The kernel must validate:

```text
Pointer
Size
Rule fields
Protocol
Port
IP address
Action
Command
```

Never assume:

```text
User input == valid input
```

---

# 45. Input Validation

Recommended validation order:

```text
Command
   │
   ▼
Argument count
   │
   ▼
Argument format
   │
   ▼
Value range
   │
   ▼
Rule semantics
   │
   ▼
IOCTL
```

Example:

```text
Port:
0        → depends on specification
1-65535  → valid
65536    → invalid
```

---

# 46. Resource Management

Every successful:

```c
open()
```

must eventually have:

```c
close()
```

Every allocated user-space resource must have a cleanup path.

Example:

```text
Application
   │
   ├── open()
   ├── malloc()
   ├── config file
   └── socket/event
         │
         ▼
       Cleanup
```

Use a single cleanup path where practical.

---

# 47. Application Exit

Normal exit:

```text
Receive command
      │
      ▼
Complete operation
      │
      ▼
Close device
      │
      ▼
Free resources
      │
      ▼
Exit
```

Signal handling may be required for long-running monitoring mode.

---

# 48. Signal Handling

Monitoring applications should handle:

```text
SIGINT
SIGTERM
```

Example:

```text
Ctrl+C
  │
  ▼
SIGINT
  │
  ▼
Stop monitoring
  │
  ▼
Close device
  │
  ▼
Cleanup
  │
  ▼
Exit
```

---

# 49. Threading

A simple CLI application may not require multiple threads.

Monitoring applications may use:

```text
Main Thread
     │
     ├── Command/control
     │
     └── Monitoring Thread
```

If multiple threads access the same rule/configuration data, synchronization
must be used.

---

# 50. User-Space Testing

User-space tests should validate:

```text
Command parser
Configuration parser
IOCTL handling
Error handling
Rule conversion
Statistics formatting
Monitoring
```

Example:

```text
tests/
├── test_parser.c
├── test_config.c
├── test_ioctl.c
├── test_rules.c
└── test_monitor.c
```

---

# 51. User-Space Unit Test

Example:

```text
Input:
tcp,192.168.1.10,192.168.1.50,443,allow

Expected:
Protocol = TCP
Source    = 192.168.1.10
Destination = 192.168.1.50
Port      = 443
Action    = ALLOW
```

No hardware should be required for pure parser tests.

---

# 52. User-Space Integration Test

Integration testing requires the target driver.

```text
CLI
 │
 ▼
Parser
 │
 ▼
IOCTL
 │
 ▼
Kernel Driver
 │
 ▼
Rule Database
```

Test:

```text
Add Rule
   ↓
Get Rule
   ↓
List Rule
   ↓
Generate Packet
   ↓
Verify Decision
```

---

# 53. User-Space Error Test

Test:

```bash
packet-filter unknown-command
```

Expected:

```text
Error: Unknown command
```

Test:

```bash
packet-filter delete-rule --id 99999
```

Expected:

```text
Error: Rule not found
```

Test:

```bash
packet-filter add-rule --dport abc
```

Expected:

```text
Error: Invalid port
```

---

# 54. Build

For a simple C application:

```bash
make
```

Example:

```text
userspace/
├── Makefile
├── src/
├── include/
└── packet-filter
```

Build result:

```text
packet-filter
```

---

# 55. Cross Compilation

For Yocto-based development, the application should preferably be built using
the Yocto SDK or as a Yocto recipe.

Conceptually:

```text
Host PC
   │
   ▼
Yocto SDK
   │
   ▼
Cross Compiler
   │
   ▼
ARM64 Binary
   │
   ▼
BeagleBone AI-64
```

---

# 56. Native Target Build

If the required compiler is available on the board:

```bash
gcc main.c -o packet-filter
```

However, production builds should normally use the controlled Yocto build
environment.

---

# 57. Yocto Integration

Recommended structure:

```text
meta-box-storage/
└── recipes-box/
    └── packet-filter/
        ├── packet-filter.bb
        └── files/
            ├── Makefile
            ├── src/
            └── include/
```

The recipe installs:

```text
/usr/bin/packet-filter
```

and configuration:

```text
/etc/packet-filter/
```

---

# 58. Installation Layout

Recommended target filesystem:

```text
/usr/bin/packet-filter
/etc/packet-filter/
├── whitelist.conf
├── blacklist.conf
└── monitoring.conf

/var/log/
└── packet-filter.log

/dev/
└── packet_filter
```

The device node is normally created by the kernel device infrastructure,
udev, or the project's configured device-management mechanism.

---

# 59. Complete User-Space Flow

```text
                    USER
                     │
                     ▼
             packet-filter CLI
                     │
                     ▼
              Argument Parser
                     │
                     ▼
             Configuration Parser
                     │
                     ▼
               Rule Manager
                     │
                     ▼
                 IOCTL
                     │
                     ▼
          /dev/packet_filter
                     │
                     ▼
             Kernel Driver
                     │
          ┌──────────┴──────────┐
          ▼                     ▼
     Rule Database         Packet Filter
                                │
                                ▼
                          ALLOW / DROP
                                │
                                ▼
                           Statistics
                                │
                                ▼
                         User-Space Monitor
```

---

# 60. Example Complete Workflow

### Step 1 – Check driver

```bash
packet-filter status
```

### Step 2 – Clear old rules

```bash
packet-filter clear-rules
```

### Step 3 – Load configuration

```bash
packet-filter load-config /etc/packet-filter/whitelist.conf
```

### Step 4 – Add blacklist rules

```bash
packet-filter load-config /etc/packet-filter/blacklist.conf
```

### Step 5 – Verify rules

```bash
packet-filter list-rules
```

### Step 6 – Enable filter

```bash
packet-filter enable
```

### Step 7 – Generate network traffic

```bash
ping <peer-ip>
```

or:

```bash
iperf3 -c <server-ip>
```

### Step 8 – Read statistics

```bash
packet-filter stats
```

### Step 9 – Monitor

```bash
packet-filter monitor
```

### Step 10 – Disable

```bash
packet-filter disable
```

---

# 61. Failure Handling Flow

```text
User Command
     │
     ▼
Validate
     │
     ├── FAIL ──► Display Error
     │
     ▼
Open Driver
     │
     ├── FAIL ──► Driver Error
     │
     ▼
IOCTL
     │
     ├── FAIL ──► Decode errno
     │
     ▼
Display Result
```

---

# 62. User-Space Security Boundary

The important security boundary is:

```text
+-----------------------------+
|        USER SPACE           |
|                             |
|  CLI                        |
|  Configuration              |
|  User Input                 |
+-------------+---------------+
              |
              | Untrusted Data
              ▼
+-----------------------------+
|        KERNEL SPACE         |
|                             |
|  Validate                   |
|  copy_from_user()           |
|  Rule Database              |
|  Packet Filter              |
+-----------------------------+
```

The kernel must independently validate every user-space request.

---

# 63. Recommended Coding Rules

User-space code should follow:

```text
1. Check every system-call return value
2. Validate all command-line input
3. Validate configuration input
4. Check ioctl() return values
5. Handle errno correctly
6. Close file descriptors
7. Free allocated memory
8. Avoid buffer overflows
9. Use fixed-width integer types for ABI structures
10. Keep kernel/user structures synchronized
11. Avoid unnecessary global state
12. Provide useful error messages
13. Keep monitoring asynchronous
14. Never assume driver availability
15. Keep CLI behavior deterministic
```

---

# 64. User-Space Checklist

```text
[ ] CLI application implemented
[ ] Device node access implemented
[ ] IOCTL interface implemented
[ ] Rule structure defined
[ ] Configuration parser implemented
[ ] Whitelist support implemented
[ ] Blacklist support implemented
[ ] Monitoring support implemented
[ ] Rule add implemented
[ ] Rule delete implemented
[ ] Rule list implemented
[ ] Rule clear implemented
[ ] Statistics implemented
[ ] Enable/disable implemented
[ ] Error handling implemented
[ ] Permission handling implemented
[ ] Logging implemented
[ ] Configuration reload implemented
[ ] API version check implemented
[ ] Unit tests implemented
[ ] Integration tests implemented
[ ] Yocto recipe implemented
```

---

# 65. Relationship With Kernel Driver

The complete system is:

```text
                USER SPACE
                     │
                     ▼
             packet-filter CLI
                     │
                     ▼
              Configuration
                     │
                     ▼
                  IOCTL
                     │
=====================│=====================
                     │
                 KERNEL SPACE
                     │
                     ▼
             Packet Filter Driver
                     │
             ┌───────┴────────┐
             ▼                ▼
        Rule Database      Packet Hook
                              │
                              ▼
                         Rule Lookup
                              │
                              ▼
                         Decision
                         /      \
                        /        \
                    ALLOW        DROP
```

The key separation is:

```text
User Space
    = Configuration + Control + Monitoring

Kernel Space
    = Packet Processing + Rule Enforcement
```

---

# 66. Final Summary

The user-space application is the control plane of the packet-filtering
system.

Its primary flow is:

```text
Configuration
     │
     ▼
User-Space Parser
     │
     ▼
Validation
     │
     ▼
IOCTL
     │
     ▼
Kernel Driver
     │
     ▼
Rule Database
     │
     ▼
Packet Processing
     │
     ▼
ALLOW / DROP
     │
     ▼
Statistics / Monitoring
     │
     ▼
User Space
```

The most important design principle is:

```text
                    CONTROL PLANE
                         │
                         ▼
                    USER SPACE
                         │
                       IOCTL
                         │
                         ▼
                     KERNEL
                         │
                         ▼
                    DATA PLANE
                         │
                         ▼
                  PACKET PROCESSING
```

User space should configure and monitor the packet filter, while the kernel
driver should perform the actual packet filtering in the networking data
path.

```
```

