# Debugging Guide – BeagleBone AI-64 / TI TDA4VM

## 1. Overview

This document describes the debugging methodology for the **BeagleBone AI-64 based on the TI TDA4VM** platform.

Debugging is performed layer by layer, starting from hardware power-up and bootloader initialization, followed by Linux kernel startup, Device Tree initialization, driver probing, networking, packet filtering, and user-space applications.

```text
Hardware
   │
   ▼
Boot ROM
   │
   ▼
SPL / TIFS
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
Drivers
   │
   ▼
Network Stack
   │
   ▼
Packet Filter
   │
   ▼
User Space
```

The main debugging interfaces are:

* UART serial console
* U-Boot console
* Linux `dmesg`
* `journalctl`
* Kernel debug messages
* Network utilities
* `tcpdump`
* `ethtool`
* Sysfs
* Procfs
* JTAG
* GDB
* Kernel tracing

---

# 2. Debugging Strategy

The recommended approach is **bottom-up debugging**.

```text
Step 1  → Power / Hardware
Step 2  → UART Console
Step 3  → Boot ROM / SPL / TIFS
Step 4  → U-Boot
Step 5  → Linux Kernel
Step 6  → Device Tree
Step 7  → Driver Probe
Step 8  → Ethernet
Step 9  → Network Stack
Step 10 → Packet Filter
Step 11 → User Space
Step 12 → Application
```

Do not start debugging the application before confirming that the lower layers are working correctly.

---

# 3. Hardware-Level Debugging

Before debugging software, verify the basic hardware.

Check:

```text
Power
  ↓
Board LEDs
  ↓
UART
  ↓
Storage
  ↓
Ethernet
  ↓
Peripheral Connections
```

Important checks:

* Board receives correct power.
* SD/eMMC boot media is correctly prepared.
* USB-UART connection is working.
* Ethernet cable is connected.
* Network link LEDs are active.
* Required peripherals are physically connected.
* No obvious board or connector damage exists.

---

# 4. UART Serial Console

UART is the primary interface for boot debugging.

Typical flow:

```text
TDA4VM
   │
   ▼
UART TX
   │
   ▼
USB-UART Adapter
   │
   ▼
Host PC
   │
   ▼
Serial Terminal
```

On the Linux host, identify the serial device:

```bash
ls /dev/ttyUSB*
```

or:

```bash
ls /dev/ttyACM*
```

A serial terminal can then be opened using a suitable terminal application.

For example:

```bash
sudo picocom -b 115200 /dev/ttyUSB0
```

The exact UART device and baud rate should match the board documentation and boot configuration.

---

# 5. Bootloader Debugging

The first important software debugging point is the bootloader.

Expected flow:

```text
Power ON
   │
   ▼
Boot ROM
   │
   ▼
TIFS / System Firmware
   │
   ▼
SPL
   │
   ▼
U-Boot
   │
   ▼
Linux
```

If there is no UART output at all:

```text
Power
  ↓
UART connection
  ↓
Boot media
  ↓
Boot mode
  ↓
Boot ROM
```

should be checked first.

If U-Boot starts successfully, interrupt the boot process and inspect the environment.

Useful commands include:

```bash
printenv
```

```bash
bdinfo
```

```bash
version
```

```bash
help
```

---

# 6. U-Boot Environment Debugging

Check important boot variables:

```bash
printenv bootcmd
```

```bash
printenv bootargs
```

```bash
printenv fdtfile
```

```bash
printenv boot_targets
```

The kernel boot arguments are particularly important.

For example:

```text
console=<uart-device>,115200
root=<root-device>
```

If the kernel does not start, verify:

1. Kernel image exists.
2. Device Tree blob exists.
3. Root filesystem exists.
4. Boot arguments are correct.
5. Memory addresses do not overlap.
6. Boot media is accessible.

---

# 7. Linux Kernel Debugging

Once Linux starts, the first tool to inspect is:

```bash
dmesg
```

For the latest kernel messages:

```bash
dmesg | tail
```

For a specific subsystem:

```bash
dmesg | grep -i ethernet
```

```bash
dmesg | grep -i network
```

```bash
dmesg | grep -i driver
```

```bash
dmesg | grep -i error
```

```bash
dmesg | grep -i fail
```

---

# 8. Kernel Log Levels

Kernel messages have different severity levels.

Common levels include:

```text
KERN_EMERG
KERN_ALERT
KERN_CRIT
KERN_ERR
KERN_WARNING
KERN_NOTICE
KERN_INFO
KERN_DEBUG
```

Kernel driver messages can be generated using:

```c
pr_info("packet filter initialized\n");
```

```c
pr_err("packet filter initialization failed\n");
```

```c
pr_warn("invalid filtering rule\n");
```

For temporary debugging:

```c
pr_debug("packet received\n");
```

---

# 9. Dynamic Debug

For kernel code that uses `pr_debug()` or dynamic-debug-enabled logging, dynamic debugging can be enabled at runtime.

Example:

```bash
mount -t debugfs none /sys/kernel/debug
```

Then:

```bash
echo 'module packet_filter +p' > /sys/kernel/debug/dynamic_debug/control
```

Check the result:

```bash
cat /sys/kernel/debug/dynamic_debug/control | grep packet_filter
```

This allows detailed debugging without permanently enabling every debug message.

---

# 10. Kernel Configuration Debugging

Verify the running kernel configuration:

```bash
zcat /proc/config.gz
```

Search for packet-filtering or networking options:

```bash
zcat /proc/config.gz | grep NETFILTER
```

```bash
zcat /proc/config.gz | grep NET
```

The required kernel configuration should also exist in:

```text
configs/kernel/packet_filter_defconfig
```

Compare the intended configuration with the running kernel when debugging configuration-related problems.

---

# 11. Device Tree Debugging

Device Tree problems frequently appear as driver-probe failures.

Check the live Device Tree:

```bash
ls /proc/device-tree/
```

Inspect a node:

```bash
find /proc/device-tree/ -maxdepth 2 -type d
```

Check kernel messages:

```bash
dmesg | grep -i of
```

```bash
dmesg | grep -i device
```

```bash
dmesg | grep -i probe
```

Typical Device Tree problems include:

* Incorrect `compatible` string
* Wrong register address
* Incorrect interrupt
* Missing clock
* Incorrect GPIO
* Disabled peripheral
* Incorrect PHY configuration
* Incorrect pinmux

A typical driver flow is:

```text
Device Tree
     │
     ▼
compatible match
     │
     ▼
Driver Match
     │
     ▼
probe()
     │
     ▼
Hardware Initialization
```

If `probe()` is never called, first verify the Device Tree and driver matching.

---

# 12. Driver Debugging

Check whether the driver is loaded:

```bash
lsmod
```

For built-in drivers:

```bash
dmesg | grep -i <driver-name>
```

Check available modules:

```bash
find /lib/modules/$(uname -r) -type f
```

Load a module:

```bash
sudo modprobe <module-name>
```

Remove a module:

```bash
sudo modprobe -r <module-name>
```

Check module information:

```bash
modinfo <module-name>
```

The expected driver lifecycle is:

```text
Module Load
    │
    ▼
Driver Registration
    │
    ▼
Device Matching
    │
    ▼
probe()
    │
    ▼
Resource Initialization
    │
    ▼
Driver Active
```

---

# 13. Debugging `probe()`

For driver development, add clear messages around every initialization stage.

Example:

```c
pr_info("packet_filter: probe started\n");

pr_info("packet_filter: allocating resources\n");

pr_info("packet_filter: registering hooks\n");

pr_info("packet_filter: creating configuration interface\n");

pr_info("packet_filter: probe completed\n");
```

Then inspect:

```bash
dmesg | grep packet_filter
```

This makes it easy to identify the exact initialization stage where the driver fails.

---

# 14. Ethernet Debugging

Check network interfaces:

```bash
ip link
```

Check IP configuration:

```bash
ip addr
```

Bring an interface up:

```bash
sudo ip link set eth0 up
```

Check the interface:

```bash
ip addr show eth0
```

Check routing:

```bash
ip route
```

Test connectivity:

```bash
ping <destination-ip>
```

---

# 15. Ethernet PHY Debugging

Use:

```bash
ethtool eth0
```

Check link status:

```bash
ethtool eth0 | grep -i link
```

Check driver information:

```bash
ethtool -i eth0
```

Check interface statistics:

```bash
ethtool -S eth0
```

Typical issues include:

```text
No carrier
   │
   ├── Cable
   ├── PHY
   ├── Pinmux
   ├── Device Tree
   ├── MAC configuration
   └── Driver
```

---

# 16. Packet Path Debugging

The packet-filtering project requires understanding the Linux packet path.

For incoming traffic:

```text
Ethernet PHY
     │
     ▼
Ethernet MAC
     │
     ▼
Network Driver
     │
     ▼
Linux Network Stack
     │
     ▼
Packet Filtering
     │
     ├── Whitelist → ACCEPT
     ├── Blacklist → DROP
     └── Monitoring → LOG
     │
     ▼
Socket / Application
```

For outgoing traffic:

```text
Application
     │
     ▼
Socket
     │
     ▼
TCP/IP
     │
     ▼
Packet Filtering
     │
     ▼
Network Driver
     │
     ▼
Ethernet MAC
     │
     ▼
PHY
```

---

# 17. Packet Capture with `tcpdump`

`tcpdump` is one of the most useful tools for network debugging.

Capture packets:

```bash
sudo tcpdump -i eth0
```

Capture ICMP:

```bash
sudo tcpdump -i eth0 icmp
```

Capture TCP:

```bash
sudo tcpdump -i eth0 tcp
```

Capture UDP:

```bash
sudo tcpdump -i eth0 udp
```

Write packets to a file:

```bash
sudo tcpdump -i eth0 -w capture.pcap
```

This allows packet behavior to be analyzed independently from the packet-filter implementation.

---

# 18. Whitelist Debugging

When a packet should be allowed:

```text
Packet
  │
  ▼
Whitelist Rule
  │
  ▼
Match?
 ┌───────┴───────┐
 │               │
YES              NO
 │               │
 ▼               ▼
ALLOW       Continue Filtering
```

Check:

```bash
cat configs/rules/whitelist.conf
```

Then verify the actual traffic:

```bash
sudo tcpdump -i eth0
```

And inspect kernel logs:

```bash
dmesg | grep -i whitelist
```

---

# 19. Blacklist Debugging

For a blocked packet:

```text
Packet
  │
  ▼
Blacklist Rule
  │
  ▼
Match?
 │
 └── YES
      │
      ▼
     DROP
```

Check:

```bash
cat configs/rules/blacklist.conf
```

Monitor:

```bash
sudo tcpdump -i eth0
```

Check driver/kernel messages:

```bash
dmesg | grep -i blacklist
```

A useful debugging message is:

```text
DROP: source=<IP> destination=<IP> protocol=<protocol> reason=blacklist
```

---

# 20. Monitoring Debugging

Monitoring rules should provide visibility without unintentionally modifying packet behavior.

Check:

```bash
cat configs/rules/monitoring.conf
```

Then inspect logs:

```bash
dmesg | grep -i monitor
```

If the project uses a dedicated log file:

```bash
tail -f /var/log/packet-filter.log
```

The monitoring path should be:

```text
Packet
   │
   ▼
Rule Evaluation
   │
   ▼
Monitoring Match
   │
   ▼
Event Generation
   │
   ▼
Kernel/User-Space Logging
```

---

# 21. Debugging Configuration Loading

Verify that the configuration files exist:

```bash
ls -l configs/rules/
```

Expected:

```text
blacklist.conf
monitoring.conf
whitelist.conf
```

Check file contents:

```bash
cat configs/rules/whitelist.conf
cat configs/rules/blacklist.conf
cat configs/rules/monitoring.conf
```

Verify permissions:

```bash
ls -l configs/rules/
```

Check whether the user-space service successfully loaded the rules:

```bash
dmesg | grep -i rule
```

---

# 22. Sysfs Debugging

Sysfs provides runtime information about devices and drivers.

Check:

```bash
ls /sys/class/net/
```

Inspect Ethernet:

```bash
ls -l /sys/class/net/eth0/
```

Check the driver:

```bash
readlink /sys/class/net/eth0/device/driver
```

This helps determine which driver is associated with the interface.

---

# 23. Procfs Debugging

Useful runtime information can be obtained from `/proc`.

CPU information:

```bash
cat /proc/cpuinfo
```

Memory:

```bash
cat /proc/meminfo
```

Network interfaces:

```bash
cat /proc/net/dev
```

Kernel command line:

```bash
cat /proc/cmdline
```

Mounted filesystems:

```bash
cat /proc/mounts
```

---

# 24. Kernel Crash Debugging

If the kernel crashes, immediately collect:

```bash
dmesg
```

Look for:

```text
Oops
BUG
WARNING
Call Trace
Kernel panic
Unable to handle
NULL pointer
```

A typical kernel crash flow is:

```text
Kernel Crash
     │
     ▼
Call Trace
     │
     ▼
Faulting Function
     │
     ▼
Source Code
     │
     ▼
Root Cause
```

Important information includes:

* CPU/core
* Program counter
* Stack trace
* Register values
* Faulting address
* Call trace
* Kernel version
* Loaded modules

---

# 25. JTAG Debugging

JTAG provides low-level hardware debugging.

```text
Host Debugger
     │
     ▼
JTAG Probe
     │
     ▼
TDA4VM
     │
     ├── CPU Registers
     ├── Memory
     ├── Program Counter
     └── Hardware State
```

JTAG is useful when:

* Processor does not boot.
* Software hangs before UART output.
* Low-level firmware needs debugging.
* CPU registers need inspection.
* Hardware initialization must be examined.

---

# 26. GDB Debugging

For user-space applications, GDB can be used to inspect execution.

Example:

```bash
gdb ./packet_filter_daemon
```

Useful commands:

```gdb
break main
run
next
step
continue
backtrace
print variable
info threads
```

For a running process:

```bash
gdb -p <pid>
```

This is useful for debugging configuration parsing, monitoring logic, and user-space packet-filter management.

---

# 27. Performance Debugging

Check CPU usage:

```bash
top
```

or:

```bash
htop
```

Check memory:

```bash
free -h
```

Check processes:

```bash
ps aux
```

Check network statistics:

```bash
ip -s link
```

Check Ethernet hardware statistics:

```bash
ethtool -S eth0
```

For packet filtering, monitor:

```text
Packet Rate
CPU Usage
Packet Drop Rate
Packet Processing Time
Memory Usage
Logging Overhead
```

---

# 28. Common Debugging Flow

When packet filtering is not working, use this sequence:

```text
1. Check Ethernet link
        ↓
2. Check IP configuration
        ↓
3. Verify ping/connectivity
        ↓
4. Capture packets with tcpdump
        ↓
5. Check kernel logs
        ↓
6. Check packet-filter driver
        ↓
7. Check configuration files
        ↓
8. Verify whitelist/blacklist rules
        ↓
9. Check rule matching
        ↓
10. Check ACCEPT/DROP decision
        ↓
11. Check monitoring logs
        ↓
12. Validate application behavior
```

---

# 29. Debugging Checklist

### Hardware

* [ ] Board powers on
* [ ] UART connection works
* [ ] Boot media is detected
* [ ] Ethernet cable is connected
* [ ] Ethernet link is active

### Bootloader

* [ ] Boot ROM starts
* [ ] TIFS/system firmware initializes
* [ ] SPL starts
* [ ] U-Boot starts
* [ ] U-Boot environment is valid
* [ ] Kernel image is found
* [ ] Device Tree is found

### Linux

* [ ] Kernel boots
* [ ] Root filesystem mounts
* [ ] `dmesg` has no critical errors
* [ ] Device Tree nodes are present
* [ ] Required drivers probe successfully

### Networking

* [ ] `eth0` exists
* [ ] Interface is UP
* [ ] IP address is configured
* [ ] Route is configured
* [ ] PHY link is active
* [ ] Ping works

### Packet Filter

* [ ] Driver loads
* [ ] Configuration files are available
* [ ] Whitelist rules load
* [ ] Blacklist rules load
* [ ] Monitoring rules load
* [ ] Packets are evaluated
* [ ] ACCEPT decisions work
* [ ] DROP decisions work
* [ ] Monitoring events are logged

### Application

* [ ] User-space daemon starts
* [ ] Configuration is parsed
* [ ] Kernel interface is accessible
* [ ] Runtime errors are handled
* [ ] Logs are generated correctly

---

# 30. Debugging Command Reference

| Area                | Command                |
| ------------------- | ---------------------- |
| Kernel logs         | `dmesg`                |
| Kernel config       | `zcat /proc/config.gz` |
| CPU                 | `cat /proc/cpuinfo`    |
| Memory              | `cat /proc/meminfo`    |
| Interfaces          | `ip link`              |
| IP addresses        | `ip addr`              |
| Routes              | `ip route`             |
| Ethernet            | `ethtool eth0`         |
| Ethernet statistics | `ethtool -S eth0`      |
| Packet capture      | `tcpdump -i eth0`      |
| Processes           | `ps aux`               |
| CPU usage           | `top`                  |
| Memory usage        | `free -h`              |
| Modules             | `lsmod`                |
| Module information  | `modinfo`              |
| Device Tree         | `/proc/device-tree/`   |
| Network statistics  | `cat /proc/net/dev`    |
| Kernel command line | `cat /proc/cmdline`    |
| Sysfs network       | `/sys/class/net/`      |

---

# 31. Final Debugging Model

The complete debugging methodology is:

```text
                    ┌───────────────┐
                    │   Hardware    │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ Bootloader    │
                    │ ROM/SPL/U-Boot│
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ Linux Kernel  │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ Device Tree   │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │    Drivers    │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ Network Stack │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ Packet Filter │
                    └───────┬───────┘
                            │
              ┌─────────────┼─────────────┐
              ▼             ▼             ▼
          Whitelist      Blacklist    Monitoring
              │             │             │
           ACCEPT          DROP           LOG
              │             │             │
              └─────────────┼─────────────┘
                            ▼
                    ┌───────────────┐
                    │ User Space    │
                    └───────────────┘
```

The key principle is:

> **Debug from the bottom layer upward. First prove the hardware and boot chain, then Linux, Device Tree, drivers, networking, packet filtering, and finally user-space behavior.**

