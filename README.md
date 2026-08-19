# BeagleBone AI-64 – TI TDA4VM
# Linux Kernel Packet Filtering System

![Platform](https://img.shields.io/badge/Platform-BeagleBone%20AI--64-blue)
![SoC](https://img.shields.io/badge/SoC-TI%20TDA4VM-orange)
![CPU](https://img.shields.io/badge/CPU-Cortex--A72%20%2B%20Cortex--R5F-green)
![OS](https://img.shields.io/badge/OS-Linux-yellow)
![Kernel](https://img.shields.io/badge/Linux%20Kernel-Custom-red)
![Build](https://img.shields.io/badge/Build-Yocto-purple)
![Language](https://img.shields.io/badge/Language-C-blue)
![Driver](https://img.shields.io/badge/Driver-Linux%20Kernel%20Module-darkgreen)
![Network](https://img.shields.io/badge/Network-Ethernet-lightblue)
![Status](https://img.shields.io/badge/Status-Development-orange)

---

## Complete BeagleBone AI-64 System Flow

The following animation illustrates the complete BeagleBone AI-64 based on the TI TDA4VM execution flow, from power-on and boot stages through the Linux kernel, Device Tree, drivers, middleware, AI acceleration, application processing, and hardware peripherals.

<p align="center">
  <img src="images/beaglebone_ai64_tda4vm_flow_animation.gif"
       alt="BeagleBone AI-64 TI TDA4VM Complete System Flow"
       width="950">
</p>


# Table of Contents

1. [Project Overview](#1-project-overview)
2. [Project Objectives](#2-project-objectives)
3. [Key Features](#3-key-features)
4. [Hardware Platform](#4-hardware-platform)
5. [TDA4VM Architecture](#5-tda4vm-architecture)
6. [Hardware Software Relationship](#6-hardware-software-relationship)
7. [Complete System Architecture](#7-complete-system-architecture)
8. [Boot Flow](#8-boot-flow)
9. [Linux Startup Flow](#9-linux-startup-flow)
10. [Packet Filtering Architecture](#10-packet-filtering-architecture)
11. [Packet Processing Flow](#11-packet-processing-flow)
12. [Packet Parser Logic](#12-packet-parser-logic)
13. [Rule Engine Logic](#13-rule-engine-logic)
14. [Whitelist Logic](#14-whitelist-logic)
15. [Blacklist Logic](#15-blacklist-logic)
16. [Monitoring Logic](#16-monitoring-logic)
17. [Default Policy](#17-default-policy)
18. [Statistics Architecture](#18-statistics-architecture)
19. [Kernel Driver Architecture](#19-kernel-driver-architecture)
20. [IOCTL Architecture](#20-ioctl-architecture)
21. [Userspace Architecture](#21-userspace-architecture)
22. [Userspace Tools](#22-userspace-tools)
23. [Sysfs / Proc / Device Interface](#23-sysfs--proc--device-interface)
24. [Network Packet Path](#24-network-packet-path)
25. [Logging Architecture](#25-logging-architecture)
26. [Performance Architecture](#26-performance-architecture)
27. [Project Directory](#27-project-directory)
28. [Yocto Architecture](#28-yocto-architecture)
29. [Yocto Build Flow](#29-yocto-build-flow)
30. [Kernel Build Flow](#30-kernel-build-flow)
31. [Complete Build Flow](#31-complete-build-flow)
32. [Driver Compilation](#32-driver-compilation)
33. [Userspace Compilation](#33-userspace-compilation)
34. [Installation](#34-installation)
35. [Flashing](#35-flashing)
36. [Driver Loading](#36-driver-loading)
37. [Configuration](#37-configuration)
38. [Testing Strategy](#38-testing-strategy)
39. [Unit Testing](#39-unit-testing)
40. [Integration Testing](#40-integration-testing)
41. [Performance Testing](#41-performance-testing)
42. [Stress Testing](#42-stress-testing)
43. [Debugging](#43-debugging)
44. [Kernel Debugging](#44-kernel-debugging)
45. [Network Debugging](#45-network-debugging)
46. [JTAG Debugging](#46-jtag-debugging)
47. [Logging](#47-logging)
48. [Error Handling](#48-error-handling)
49. [Security Considerations](#49-security-considerations)
50. [Performance Considerations](#50-performance-considerations)
51. [Deployment Flow](#51-deployment-flow)
52. [Troubleshooting](#52-troubleshooting)
53. [Example Packet Flows](#53-example-packet-flows)
54. [Development Workflow](#54-development-workflow)
55. [Future Enhancements](#55-future-enhancements)
56. [Conclusion](#56-conclusion)

---

# 1. Project Overview

The **BeagleBone AI-64 – TI TDA4VM Packet Filtering System** is an embedded Linux networking project that implements configurable packet filtering inside the Linux kernel.

The system provides:

- Packet inspection
- Source/destination IP filtering
- TCP/UDP filtering
- Source/destination port filtering
- Whitelist rules
- Blacklist rules
- Monitoring rules
- Default packet policy
- Packet statistics
- Runtime configuration
- Userspace control
- Kernel logging
- Performance monitoring
- Yocto-based integration
- Automated testing

The project is designed around the Linux networking stack and a custom kernel packet-filter driver.

---

# 2. Project Objectives

The main objectives are:

- Develop a Linux kernel packet-filter driver.
- Integrate the driver with the BeagleBone AI-64 platform.
- Provide userspace configuration tools.
- Implement whitelist and blacklist policies.
- Provide packet monitoring.
- Maintain packet and byte statistics.
- Provide IOCTL-based kernel/userspace communication.
- Integrate the driver into a Yocto Linux image.
- Provide automated unit and integration tests.
- Provide performance and stress testing.
- Provide debugging and logging infrastructure.

---

# 3. Key Features

| Feature | Description |
|---|---|
| Packet Filtering | Inspect and classify network packets |
| Whitelist | Explicitly allow trusted traffic |
| Blacklist | Explicitly reject unwanted traffic |
| Monitoring | Observe traffic without blocking |
| IP Filtering | Source/destination IPv4 filtering |
| Port Filtering | TCP/UDP source/destination ports |
| Protocol Filtering | TCP, UDP, ICMP |
| Default Policy | Allow or drop unmatched packets |
| IOCTL API | Kernel/userspace communication |
| Statistics | Packet and byte counters |
| Logging | Kernel and userspace diagnostic logs |
| Configuration | Runtime rule configuration |
| Yocto | Reproducible embedded Linux build |
| Testing | Unit, integration, performance and stress tests |

---

# 4. Hardware Platform

## BeagleBone AI-64

The project targets the **BeagleBone AI-64**, based on the Texas Instruments TDA4VM/J721E family.

### Major hardware blocks

```text
                    BeagleBone AI-64
                          |
        +-----------------+-----------------+
        |                 |                 |
       CPU              Memory           I/O
        |                 |                 |
   Cortex-A72         LPDDR4/DDR        Ethernet
   Cortex-R5F         Memory            USB
        |                                PCIe
        |                                GPIO
        |                                UART
        |                                I2C
        |                                SPI
        |
      TDA4VM
        |
   +----+----+
   |         |
  GPU       DSP
   |         |
  3D       C7x DSP
   |
  NPU
````

---

# 5. TDA4VM Architecture

The TI TDA4VM is designed for embedded processing, networking, vision and AI workloads.

Important processing elements include:

* ARM Cortex-A72 application processor
* ARM Cortex-R5F real-time processors
* C7x DSP
* GPU
* Vision processing hardware
* AI acceleration
* Ethernet connectivity
* Memory controllers
* DMA
* Peripheral controllers

For this project, the primary Linux execution environment is the **Cortex-A72 application processor**.

---

# 6. Hardware Software Relationship

The packet-filter system follows this relationship:

```text
Hardware
   |
   v
Ethernet MAC
   |
   v
Linux Network Driver
   |
   v
Linux Networking Stack
   |
   v
Packet Filter
   |
   +----> Rule Engine
   |
   +----> Packet Parser
   |
   +----> Statistics
   |
   +----> Logging
   |
   v
Userspace
```

The hardware receives the Ethernet frame.

The Linux Ethernet driver transfers packet information into the Linux networking stack.

The packet filter inspects the packet and applies configured rules.

---

# 7. Complete System Architecture

```text
+------------------------------------------------------------+
|                       USERSPACE                            |
|                                                            |
|  filter_ctl       filter_stats       filter_test           |
|       |                 |                 |                |
|       +-----------------+-----------------+                |
|                         |                                  |
|                     libfilter                               |
|                         |                                  |
|                       IOCTL                                |
+-------------------------|----------------------------------+
                          |
                          v
+------------------------------------------------------------+
|                     LINUX KERNEL                           |
|                                                            |
|                 packet_filter driver                       |
|                                                            |
|     +---------------+---------------+----------------+     |
|     |               |               |                |     |
| Packet Parser   Rule Engine     Statistics       Logging   |
|     |               |               |                |     |
|     +---------------+---------------+----------------+     |
|                         |                                  |
|                    Decision Engine                         |
|                         |                                  |
+-------------------------|----------------------------------+
                          |
                          v
+------------------------------------------------------------+
|                  LINUX NETWORK STACK                       |
+------------------------------------------------------------+
                          |
                          v
+------------------------------------------------------------+
|                    ETHERNET DRIVER                         |
+------------------------------------------------------------+
                          |
                          v
+------------------------------------------------------------+
|                     TDA4VM HW                              |
+------------------------------------------------------------+
                          |
                          v
                       Ethernet
```

---

# 8. Boot Flow

The complete embedded boot flow is:

```text
Power ON
   |
   v
Boot ROM
   |
   v
TI Firmware / TIFS
   |
   v
SPL / Initial Bootloader
   |
   v
U-Boot
   |
   v
Linux Kernel
   |
   v
Device Tree
   |
   v
Root Filesystem
   |
   v
systemd
   |
   v
Packet Filter Service
   |
   v
Kernel Driver
   |
   v
Userspace Services
```

---

# 9. Linux Startup Flow

```text
Kernel starts
     |
     v
Architecture initialization
     |
     v
Memory initialization
     |
     v
Interrupt initialization
     |
     v
Device Tree parsing
     |
     v
Driver subsystem initialization
     |
     v
Network subsystem
     |
     v
Ethernet driver
     |
     v
Network interface
     |
     v
systemd
     |
     v
packet-filter.service
     |
     v
packet_filter.ko
     |
     v
/dev/packet_filter
```

---

# 10. Packet Filtering Architecture

The filtering architecture is divided into five major components:

```text
                    Packet
                      |
                      v
                Packet Parser
                      |
                      v
                Rule Engine
                      |
          +-----------+-----------+
          |           |           |
          v           v           v
       Whitelist   Blacklist   Monitor
          |           |           |
          +-----------+-----------+
                      |
                      v
                  Decision
                      |
            +---------+---------+
            |                   |
          ALLOW                DROP
            |
            v
         Network
```

---

# 11. Packet Processing Flow

A received packet follows this flow:

```text
Ethernet Frame
      |
      v
Linux Network Driver
      |
      v
Network Stack
      |
      v
Packet Filter
      |
      v
Ethernet Header
      |
      v
IPv4 Header
      |
      v
TCP / UDP / ICMP Header
      |
      v
Packet Parser
      |
      v
Rule Engine
      |
      v
Rule Matching
      |
      +------ Match ------+
      |                   |
      v                   v
    Action              No Match
      |                   |
      |                   v
      |              Default Policy
      |                   |
      +---------+---------+
                |
                v
             Decision
                |
        +-------+-------+
        |               |
      ALLOW             DROP
        |               |
        v               v
 Network Stack       Packet Discard
```

---

# 12. Packet Parser Logic

The packet parser extracts:

* Ethernet protocol
* Source MAC
* Destination MAC
* IPv4 source address
* IPv4 destination address
* IP protocol
* TCP source port
* TCP destination port
* UDP source port
* UDP destination port
* Packet length

Example:

```text
Ethernet
    |
    +-- Source MAC
    +-- Destination MAC
    +-- EtherType
             |
             v
           IPv4
             |
             +-- Source IP
             +-- Destination IP
             +-- Protocol
                    |
             +------+------+
             |             |
            TCP           UDP
             |             |
        Source Port    Source Port
        Dest Port      Dest Port
```

---

# 13. Rule Engine Logic

The rule engine compares the parsed packet against configured rules.

Example rule:

```text
ALLOW TCP
SRC = 192.168.10.100
SPORT = 12345
DST = 192.168.1.10
DPORT = 8080
PRIORITY = 10
```

Matching logic:

```text
Packet
  |
  v
Source IP match?
  |
  +-- No --> Next rule
  |
 Yes
  |
  v
Destination IP match?
  |
  +-- No --> Next rule
  |
 Yes
  |
  v
Protocol match?
  |
  +-- No --> Next rule
  |
 Yes
  |
  v
Source Port match?
  |
  +-- No --> Next rule
  |
 Yes
  |
  v
Destination Port match?
  |
  +-- No --> Next rule
  |
 Yes
  |
  v
Rule matched
  |
  v
Apply action
```

---

# 14. Whitelist Logic

Whitelist rules explicitly permit trusted traffic.

Example:

```text
ALLOW TCP
192.168.10.100 -> 192.168.1.10:8080
```

Flow:

```text
Packet
  |
  v
Whitelist lookup
  |
  +---- Match ----> ALLOW
  |
  +---- No Match -> Continue
```

Whitelist rules should normally have high priority.

---

# 15. Blacklist Logic

Blacklist rules explicitly reject unwanted traffic.

Example:

```text
DROP TCP
10.10.10.100 -> ANY
```

Flow:

```text
Packet
  |
  v
Blacklist lookup
  |
  +---- Match ----> DROP
  |
  +---- No Match -> Continue
```

---

# 16. Monitoring Logic

Monitoring observes packets without changing the packet forwarding decision.

```text
Packet
  |
  v
Rule Match
  |
  v
MONITOR
  |
  +---- Update statistics
  |
  +---- Generate log
  |
  v
Continue packet processing
```

Monitoring is useful for:

* Debugging
* Traffic analysis
* Rule validation
* Performance analysis
* Security monitoring

---

# 17. Default Policy

If no rule matches a packet, the default policy is applied.

Possible policies:

```text
DEFAULT ALLOW
```

or:

```text
DEFAULT DROP
```

Recommended security configuration:

```text
Specific ALLOW rules
        |
        v
Specific DROP rules
        |
        v
Monitoring rules
        |
        v
DEFAULT DROP
```

This follows a deny-by-default security model.

---

# 18. Statistics Architecture

Statistics are maintained inside the kernel.

Counters include:

```text
Packets Received
Packets Processed
Packets Allowed
Packets Dropped
Packets Monitored
Bytes Received
Bytes Processed
Bytes Allowed
Bytes Dropped
Rule Matches
Rule Misses
Parser Errors
Driver Errors
```

Architecture:

```text
Packet
  |
  v
Filter
  |
  +---- Update packet counters
  |
  +---- Update byte counters
  |
  +---- Update rule counters
  |
  v
Decision
```

Userspace retrieves statistics using the library:

```text
filter_stats
      |
      v
libfilter
      |
      v
IOCTL
      |
      v
Kernel Statistics
```

---

# 19. Kernel Driver Architecture

Main driver components:

```text
kernel/packet_filter/
│
├── packet_filter.c
│
├── packet_filter.h
│
├── packet_parser.c
├── packet_parser.h
│
├── rule_engine.c
├── rule_engine.h
│
├── statistics.c
├── statistics.h
│
├── logging.c
├── logging.h
│
└── ioctl_defs.h
```

Responsibilities:

| File            | Responsibility       |
| --------------- | -------------------- |
| packet_filter.c | Main driver          |
| packet_filter.h | Driver definitions   |
| packet_parser.c | Packet parsing       |
| rule_engine.c   | Rule matching        |
| statistics.c    | Counters             |
| logging.c       | Kernel logging       |
| ioctl_defs.h    | Kernel/userspace API |

---

# 20. IOCTL Architecture

Userspace communicates with the driver through IOCTL.

```text
Userspace
    |
    v
libfilter.c
    |
    v
ioctl()
    |
    v
/dev/packet_filter
    |
    v
packet_filter_ioctl()
    |
    +---- ADD_RULE
    +---- DELETE_RULE
    +---- GET_RULE
    +---- GET_RULES
    +---- CLEAR_RULES
    +---- GET_STATS
    +---- RESET_STATS
    +---- ENABLE_MONITOR
    +---- DISABLE_MONITOR
    |
    v
Kernel
```

This provides a controlled interface between userspace and kernel space.

---

# 21. Userspace Architecture

```text
+------------------------------+
|        Userspace Tools       |
|                              |
| filter_ctl                   |
| filter_stats                 |
| filter_test                  |
| benchmark                    |
+--------------+---------------+
               |
               v
+------------------------------+
|        libfilter             |
|                              |
| Rule API                     |
| Statistics API               |
| Monitoring API               |
| Driver API                   |
+--------------+---------------+
               |
               v
             ioctl
               |
               v
+------------------------------+
|       Kernel Driver          |
+------------------------------+
```

---

# 22. Userspace Tools

## filter_ctl

Controls the packet-filter configuration.

Examples:

```bash
filter_ctl add
filter_ctl delete
filter_ctl list
filter_ctl enable
filter_ctl disable
```

---

## filter_stats

Displays runtime statistics.

```bash
sudo ./filter_stats show
```

Monitor continuously:

```bash
sudo ./filter_stats monitor
```

---

## filter_test

Runs functional tests.

```bash
sudo ./filter_test
```

Quick test:

```bash
sudo ./filter_test quick
```

---

## benchmark

Measures performance.

```bash
sudo ./benchmark
```

---

# 23. Sysfs / Proc / Device Interface

The primary character-device interface is:

```text
/dev/packet_filter
```

Example:

```bash
ls -l /dev/packet_filter
```

The kernel driver can expose runtime information through sysfs/proc/debugfs where required.

Example conceptual structure:

```text
/sys/
  |
  +-- class/
      |
      +-- packet_filter/
```

---

# 24. Network Packet Path

Actual network packet path:

```text
Ethernet PHY
    |
    v
TDA4VM Ethernet MAC
    |
    v
Ethernet Driver
    |
    v
DMA
    |
    v
Linux Network Stack
    |
    v
Packet Filter
    |
    v
Parser
    |
    v
Rule Engine
    |
    v
Decision
    |
    +---- DROP
    |
    +---- ALLOW
             |
             v
       TCP/IP Stack
             |
             v
        Application
```

---

# 25. Logging Architecture

Logging is divided into:

### Kernel logging

Uses Linux kernel logging:

```c
pr_info()
pr_warn()
pr_err()
pr_debug()
```

View logs:

```bash
dmesg
```

or:

```bash
journalctl -k
```

### Userspace logging

Userspace applications use:

```text
INFO
WARNING
ERROR
DEBUG
```

Logging flow:

```text
Packet
  |
  v
Packet Parser
  |
  +---- Error --> ERROR log
  |
  v
Rule Engine
  |
  +---- Match --> DEBUG/INFO
  |
  v
Decision
  |
  +---- DROP --> INFO/WARNING
```

---

# 26. Performance Architecture

Performance-critical areas:

* Packet parsing
* Rule lookup
* Rule comparison
* Lock contention
* Memory allocation
* IOCTL overhead
* Logging overhead
* Statistics updates

Performance principle:

```text
Packet
  |
  v
Avoid unnecessary allocations
  |
  v
Parse only required headers
  |
  v
Efficient rule lookup
  |
  v
Minimal locking
  |
  v
Fast decision
```

Benchmark metrics:

```text
Packets/sec
Mpps
Bytes/sec
Mbps
Average latency
Rule lookup latency
IOCTL latency
CPU utilization
```

---

# 27. Project Directory

```text
BeagleBone_AI-64–TI_TDA4VM/
│
├── README.md
│
├── configs/
│   └── kernel/
│       └── packet_filter_defconfig
│
├── docs/
│   ├── architecture.md
│   ├── debugging.md
│   ├── deployment.md
│   ├── device-tree.md
│   ├── ioctl-api.md
│   ├── kernel-driver.md
│   ├── packet-flow.md
│   ├── performance.md
│   ├── testing.md
│   ├── userspace.md
│   └── yocto-build.md
│
├── kernel/
│   └── packet_filter/
│       ├── ioctl_defs.h
│       ├── logging.c
│       ├── logging.h
│       ├── packet_filter.c
│       ├── packet_filter.h
│       ├── packet_parser.c
│       ├── packet_parser.h
│       ├── rule_engine.c
│       ├── rule_engine.h
│       ├── statistics.c
│       ├── statistics.h
│       └── Makefile
│
├── scripts/
│   ├── build.sh
│   ├── clean.sh
│   ├── configure.sh
│   ├── install.sh
│   ├── load_driver.sh
│   ├── perf_test.sh
│   ├── stress_test.sh
│   ├── test_suite.sh
│   └── unload_driver.sh
│
├── systemd/
│   └── packet-filter.service
│
├── tests/
│   ├── integration/
│   │   ├── test_filtering.sh
│   │   ├── test_ioctl.sh
│   │   └── test_sysfs.sh
│   │
│   ├── performance/
│   │   ├── benchmark.c
│   │   └── packet_generator.c
│   │
│   └── unit/
│       ├── test_packet_parser.c
│       └── test_rule_engine.c
│
├── userspace/
│   ├── config/
│   │   └── default_rules.conf
│   │
│   ├── include/
│   │   └── libfilter.h
│   │
│   ├── lib/
│   │   └── libfilter.c
│   │
│   └── tools/
│       ├── benchmark.c
│       ├── filter_ctl.c
│       ├── filter_stats.c
│       └── filter_test.c
│
└── yocto/
    └── meta-packet-filter/
        ├── conf/
        │   └── layer.conf
        │
        ├── packet-filter/
        │   ├── packet-filter.service
        │   └── default_rules.conf
        │
        └── recipes-kernel/
            └── packet-filter/
                ├── packet-filter.bb
                └── files/
                    ├── 0001-packet-filter-driver.patch
                    └── packet-filter.service
```

---

# 28. Yocto Architecture

The custom Yocto layer is:

```text
yocto/
   |
   v
meta-packet-filter
   |
   +-- conf/layer.conf
   |
   +-- recipes-kernel
   |
   +-- packet-filter
   |
   +-- systemd service
   |
   v
BitBake
   |
   v
Kernel Module
   |
   v
RootFS
   |
   v
Final Image
```

---

# 29. Yocto Build Flow

```text
Source Code
     |
     v
meta-packet-filter
     |
     v
layer.conf
     |
     v
packet-filter.bb
     |
     v
BitBake
     |
     +---- Kernel
     |
     +---- Module
     |
     +---- Service
     |
     +---- Configuration
     |
     v
RootFS
     |
     v
Image
```

Add layer:

```bash
bitbake-layers add-layer ../yocto/meta-packet-filter
```

Verify:

```bash
bitbake-layers show-layers
```

Build:

```bash
bitbake packet-filter
```

---

# 30. Kernel Build Flow

```text
Kernel Source
     |
     v
packet_filter_defconfig
     |
     v
make menuconfig
     |
     v
Kernel Configuration
     |
     v
Driver Compilation
     |
     v
packet_filter.ko
```

Example:

```bash
make ARCH=arm64 packet_filter_defconfig
```

Build:

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules
```

---

# 31. Complete Build Flow

Complete project flow:

```text
Developer
    |
    v
Modify C Source
    |
    v
Kernel Driver
    |
    v
Compile
    |
    v
packet_filter.ko
    |
    v
Userspace Library
    |
    v
Userspace Tools
    |
    v
Yocto Recipe
    |
    v
BitBake
    |
    v
RootFS
    |
    v
Linux Image
    |
    v
SD/eMMC
    |
    v
BeagleBone AI-64
```

---

# 32. Driver Compilation

From the kernel driver directory:

```bash
cd kernel/packet_filter
```

Build:

```bash
make
```

Expected output:

```text
packet_filter.ko
```

Check:

```bash
file packet_filter.ko
```

---

# 33. Userspace Compilation

Build the library:

```bash
gcc -c userspace/lib/libfilter.c \
    -Iuserspace/include
```

Build tools:

```bash
gcc -Wall -Wextra -O2 \
    -Iuserspace/include \
    userspace/tools/filter_ctl.c \
    userspace/lib/libfilter.c \
    -o filter_ctl
```

Statistics:

```bash
gcc -Wall -Wextra -O2 \
    -Iuserspace/include \
    userspace/tools/filter_stats.c \
    userspace/lib/libfilter.c \
    -o filter_stats
```

Functional test:

```bash
gcc -Wall -Wextra -O2 \
    -Iuserspace/include \
    userspace/tools/filter_test.c \
    userspace/lib/libfilter.c \
    -o filter_test
```

---

# 34. Installation

Install driver:

```bash
sudo ./scripts/install.sh
```

Load:

```bash
sudo ./scripts/load_driver.sh
```

Verify:

```bash
lsmod | grep packet_filter
```

Check device:

```bash
ls -l /dev/packet_filter
```

---

# 35. Flashing

The final Yocto image can be deployed to SD/eMMC.

General flow:

```text
Yocto Image
     |
     v
SD Card / eMMC
     |
     v
BeagleBone AI-64
     |
     v
Bootloader
     |
     v
Linux
     |
     v
RootFS
```

Always verify the target storage device before writing an image.

---

# 36. Driver Loading

Manual:

```bash
sudo modprobe packet_filter
```

Or:

```bash
sudo ./scripts/load_driver.sh
```

Verify:

```bash
lsmod | grep packet_filter
```

Kernel logs:

```bash
dmesg | tail -50
```

Unload:

```bash
sudo ./scripts/unload_driver.sh
```

---

# 37. Configuration

Default rules:

```text
userspace/config/default_rules.conf
```

Example:

```text
ALLOW TCP 192.168.10.100 12345 192.168.1.10 8080
DROP TCP 10.10.10.100 ANY 192.168.1.10 ANY
MONITOR ICMP ANY ANY ANY ANY
```

Configuration flow:

```text
default_rules.conf
       |
       v
filter_ctl
       |
       v
libfilter
       |
       v
IOCTL
       |
       v
Kernel Rule Engine
```

---

# 38. Testing Strategy

Testing is divided into:

```text
                 Testing
                    |
        +-----------+-----------+
        |           |           |
      Unit      Integration  Performance
        |           |           |
        v           v           v
 Parser       Driver/API      Benchmark
 Rule Engine  Filtering       Packet Rate
```

Additional:

```text
Stress Testing
Regression Testing
Hardware Testing
Boot Testing
Network Testing
```

---

# 39. Unit Testing

Unit tests:

```text
tests/unit/
├── test_packet_parser.c
└── test_rule_engine.c
```

Parser tests verify:

* IPv4 parsing
* TCP parsing
* UDP parsing
* Invalid packet handling
* Header length validation
* Protocol identification

Rule engine tests verify:

* Rule matching
* Priority
* Whitelist
* Blacklist
* Default policy
* Multiple rules

---

# 40. Integration Testing

Integration tests:

```text
tests/integration/
├── test_filtering.sh
├── test_ioctl.sh
└── test_sysfs.sh
```

Test flow:

```text
Userspace
    |
    v
IOCTL
    |
    v
Kernel Driver
    |
    v
Rule Engine
    |
    v
Decision
```

---

# 41. Performance Testing

Performance test files:

```text
tests/performance/
├── benchmark.c
└── packet_generator.c
```

Metrics:

```text
Packets/sec
Mpps
Mbps
Latency
CPU usage
Rule lookup time
Statistics overhead
```

Run:

```bash
sudo ./scripts/perf_test.sh
```

---

# 42. Stress Testing

Stress testing checks:

* High packet rate
* Large number of rules
* Repeated rule updates
* Concurrent userspace operations
* Long-duration filtering
* Statistics overflow behavior
* Memory stability

Run:

```bash
sudo ./scripts/stress_test.sh
```

---

# 43. Debugging

Main debugging tools:

```bash
dmesg
journalctl
ip
ethtool
ss
tcpdump
lsmod
modinfo
cat
grep
strace
```

Example:

```bash
dmesg | grep packet
```

Network:

```bash
ip addr
ip link
```

Interface:

```bash
ethtool eth0
```

Traffic:

```bash
tcpdump -i eth0
```

---

# 44. Kernel Debugging

Check module:

```bash
lsmod | grep packet_filter
```

Check module information:

```bash
modinfo packet_filter
```

Check kernel logs:

```bash
dmesg | grep packet_filter
```

Follow logs:

```bash
dmesg -w
```

---

# 45. Network Debugging

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

Capture packets:

```bash
sudo tcpdump -i eth0 -nn
```

Test connectivity:

```bash
ping <target-ip>
```

---

# 46. JTAG Debugging

JTAG can be used for low-level debugging.

Typical flow:

```text
JTAG Probe
    |
    v
TDA4VM
    |
    +---- Cortex-A72
    |
    +---- Cortex-R5F
    |
    +---- DSP
```

Useful for:

* Bootloader debugging
* Kernel bring-up
* CPU halt
* Register inspection
* Memory inspection
* Crash analysis

---

# 47. Logging

Logging levels:

```text
DEBUG
INFO
WARNING
ERROR
CRITICAL
```

Kernel:

```bash
dmesg
```

Systemd:

```bash
journalctl -u packet-filter.service
```

Kernel service:

```bash
journalctl -k
```

Live:

```bash
dmesg -w
```

---

# 48. Error Handling

Errors are handled at multiple levels:

```text
Userspace
    |
    v
libfilter
    |
    v
IOCTL validation
    |
    v
Kernel driver
    |
    v
Parser
    |
    v
Rule Engine
```

Typical errors:

```text
Invalid rule
Invalid IP address
Invalid port
Invalid protocol
Invalid packet
Missing driver
Invalid IOCTL
Permission denied
Rule table full
Device unavailable
```

---

# 49. Security Considerations

Security principles:

### Deny by default

```text
DEFAULT DROP
```

### Validate all userspace data

```text
Userspace
   |
   v
copy_from_user()
   |
   v
Validate
   |
   v
Use data
```

### Avoid unsafe kernel memory access

Never directly trust userspace pointers.

### Rule priority

Security-critical DROP rules should have appropriate priority.

### Logging

Security events should be logged without flooding the kernel log.

---

# 50. Performance Considerations

Performance bottlenecks:

```text
Rule search
    |
    v
O(N) lookup
```

For a small rule set this is acceptable.

For a large rule set, possible improvements include:

```text
Hash tables
Prefix trees
Trie
Connection tracking
Flow cache
Rule indexing
```

Potential optimization:

```text
Current:

Packet
  |
  v
Check Rule 1
  |
  v
Check Rule 2
  |
  v
Check Rule 3
  |
  v
...
  |
  v
Check Rule N
```

Optimized:

```text
Packet
  |
  v
Hash / Prefix Lookup
  |
  v
Candidate Rules
  |
  v
Exact Match
  |
  v
Decision
```

---

# 51. Deployment Flow

Production deployment:

```text
Developer
    |
    v
Git Repository
    |
    v
Yocto Build
    |
    v
Kernel + RootFS
    |
    v
SD/eMMC Image
    |
    v
BeagleBone AI-64
    |
    v
Boot
    |
    v
systemd
    |
    v
packet-filter.service
    |
    v
packet_filter.ko
    |
    v
Load Rules
    |
    v
Packet Filtering Active
```

---

# 52. Troubleshooting

## Driver does not load

```bash
sudo modprobe packet_filter
```

Check:

```bash
dmesg | tail -50
```

---

## Device node missing

Check:

```bash
ls -l /dev/packet_filter
```

Then:

```bash
dmesg | grep packet
```

---

## Rules cannot be added

Check:

```bash
filter_ctl list
```

Check driver:

```bash
lsmod | grep packet_filter
```

---

## Packets are unexpectedly dropped

Check:

```text
Rule priority
Source IP
Destination IP
Protocol
Source port
Destination port
Default policy
```

---

## No packet statistics

Check:

```bash
filter_stats show
```

Then:

```bash
dmesg | grep packet
```

---

## Yocto build failure

Check layers:

```bash
bitbake-layers show-layers
```

Check recipe:

```bash
bitbake -e packet-filter
```

Clean:

```bash
bitbake -c clean packet-filter
```

Rebuild:

```bash
bitbake packet-filter
```

---

# 53. Example Packet Flows

## Example 1 – Whitelisted packet

Rule:

```text
ALLOW TCP
192.168.10.100:12345
      ->
192.168.1.10:8080
```

Packet:

```text
192.168.10.100:12345
          |
          v
       TCP 8080
          |
          v
      Rule Match
          |
          v
        ALLOW
```

---

## Example 2 – Blacklisted packet

Rule:

```text
DROP TCP
10.10.10.100 -> ANY
```

Packet:

```text
10.10.10.100
      |
      v
 Rule Match
      |
      v
    DROP
```

---

## Example 3 – Monitoring

```text
ICMP Packet
     |
     v
MONITOR Rule
     |
     +---- Statistics
     |
     +---- Logging
     |
     v
Continue Processing
```

---

## Example 4 – No rule match

```text
Packet
   |
   v
No matching rule
   |
   v
Default policy
   |
   +---- DEFAULT ALLOW
   |
   +---- DEFAULT DROP
```

---

# 54. Development Workflow

Recommended development workflow:

```text
1. Modify source
       |
       v
2. Compile
       |
       v
3. Unit test
       |
       v
4. Load driver
       |
       v
5. Configure rules
       |
       v
6. Test packets
       |
       v
7. Check logs
       |
       v
8. Check statistics
       |
       v
9. Performance test
       |
       v
10. Stress test
       |
       v
11. Yocto integration
       |
       v
12. Build final image
       |
       v
13. Flash hardware
       |
       v
14. Hardware validation
```

---

# 55. Future Enhancements

Possible future improvements:

* IPv6 filtering
* VLAN filtering
* MAC filtering
* Ethernet frame filtering
* Stateful firewall
* Connection tracking
* NAT
* Rule persistence
* JSON configuration
* REST API
* Web dashboard
* eBPF integration
* XDP-based filtering
* Hardware acceleration
* TI networking acceleration
* Rule hash tables
* Multi-core packet processing
* CPU affinity
* Lock-free statistics
* Per-interface policies
* Per-process policies
* Rate limiting
* Traffic shaping
* DoS protection

---

# 56. Conclusion

The BeagleBone AI-64 – TI TDA4VM Packet Filtering System provides a complete embedded Linux packet-filtering architecture.

The project combines:

```text
Hardware
   +
Bootloader
   +
Linux Kernel
   +
Device Tree
   +
Ethernet Driver
   +
Packet Filter Driver
   +
Packet Parser
   +
Rule Engine
   +
Statistics
   +
Logging
   +
IOCTL
   +
Userspace Library
   +
Userspace Tools
   +
Yocto
   +
Testing
   +
Deployment
```

The complete operational flow is:

```text
                    POWER ON
                       |
                       v
                  Boot ROM
                       |
                       v
                     SPL
                       |
                       v
                    U-Boot
                       |
                       v
                  Linux Kernel
                       |
                       v
                  Device Tree
                       |
                       v
                    RootFS
                       |
                       v
                    systemd
                       |
                       v
             packet-filter.service
                       |
                       v
              packet_filter.ko
                       |
                       v
              /dev/packet_filter
                       |
                       v
                 libfilter
                       |
                       v
                 Rule Config
                       |
                       v
                 Ethernet Packet
                       |
                       v
                Packet Parser
                       |
                       v
                  Rule Engine
                       |
             +---------+---------+
             |         |         |
             v         v         v
          ALLOW      DROP     MONITOR
             |         |         |
             |         |         +----> Statistics
             |         |         +----> Logging
             |         |
             v         v
             Network Decision
                       |
                       v
                  Statistics
                       |
                       v
                filter_stats
```

The project therefore provides a complete embedded Linux networking solution from **hardware packet reception to kernel filtering, userspace configuration, monitoring, testing, Yocto integration and deployment**.

---

Platform:

**BeagleBone AI-64**

SoC:

**Texas Instruments TDA4VM**

Operating System:

**Embedded Linux**

Build System:

**Yocto / BitBake**

Kernel Component:

**Custom Linux Packet Filter Driver**

Userspace:

**C-based control, statistics and testing tools**

Primary Interfaces:

**Ethernet / IOCTL / systemd / Device Node**

---

````

