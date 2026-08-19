# BeagleBone AI-64 – TI TDA4VM Architecture

## 1. Overview

The **BeagleBone AI-64** is based on the **Texas Instruments TDA4VM** processor and is designed for embedded Linux, edge AI, computer vision, networking, and real-time processing applications.

This project focuses on building a complete embedded Linux software stack around the TDA4VM, including:

* Bootloader
* Linux kernel
* Device Tree
* Yocto Linux distribution
* Packet filtering
* Ethernet networking
* Kernel driver development
* User-space monitoring
* Testing and debugging

The overall software architecture is divided into multiple layers, starting from the processor boot ROM and ending with user-space applications and network services.

---

## 2. Hardware Architecture

### 2.1 Main Processor

The BeagleBone AI-64 uses the **TI TDA4VM** SoC.

The processor contains multiple processing domains designed for different workloads:

```text
                    TDA4VM SoC
                        │
        ┌───────────────┼────────────────┐
        │               │                │
     Cortex-A72      Cortex-R5F       C7x DSP
     Application     Real-Time        AI/DSP
      Processing     Processing       Processing
        │               │                │
        └───────────────┼────────────────┘
                        │
                 Hardware Accelerators
                        │
        ┌───────────────┼────────────────┐
        │               │                │
      Vision           GPU              ISP
   Accelerators     Graphics        Image Processing
```

The Cortex-A72 subsystem primarily runs the Linux operating system and application software.

---

## 3. High-Level Software Architecture

```text
+-----------------------------------------------------------+
|                    User Applications                      |
|                                                           |
| Network Applications | Monitoring | Testing | Utilities  |
+-----------------------------------------------------------+
|                    User-Space Services                   |
|                                                           |
| Packet Filter Daemon | Logging | Configuration Manager   |
+-----------------------------------------------------------+
|                     Linux Kernel                         |
|                                                           |
| Network Stack | Packet Filter Driver | Ethernet Driver   |
| Device Drivers | TCP/IP | Netfilter | Kernel Services   |
+-----------------------------------------------------------+
|                     Device Tree                          |
|                                                           |
| CPU | Memory | Ethernet | GPIO | I2C | SPI | UART       |
+-----------------------------------------------------------+
|                  Linux BSP / Yocto                        |
|                                                           |
| Kernel | Bootloader | RootFS | Applications | Packages   |
+-----------------------------------------------------------+
|                     Bootloader                           |
|                                                           |
| ROM → TIFS → SPL → U-Boot → Linux                        |
+-----------------------------------------------------------+
|                  TDA4VM Hardware                         |
|                                                           |
| Cortex-A72 | Cortex-R5F | C7x | Memory | Peripherals     |
+-----------------------------------------------------------+
```

---

## 4. Boot Architecture

The boot process begins when power is applied to the board.

```text
Power ON
   │
   ▼
TI Boot ROM
   │
   ▼
System Firmware / TIFS
   │
   ▼
SPL
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
Root Filesystem
   │
   ▼
System Services
   │
   ▼
User Applications
```

Each stage initializes additional hardware and software components required by the next stage.

Detailed boot behavior is documented in:

```text
docs/boot-flow.md
```

---

## 5. Linux Kernel Architecture

The Linux kernel provides the main operating-system functionality.

```text
                     Linux Kernel
                          │
       ┌──────────────────┼──────────────────┐
       │                  │                  │
   Process Mgmt       Memory Mgmt       Scheduler
       │                  │                  │
       └──────────────────┼──────────────────┘
                          │
                  Network Subsystem
                          │
              ┌───────────┼───────────┐
              │           │           │
           TCP/IP      Netfilter    Ethernet
              │           │           │
              │      Packet Filter    │
              │           │           │
              └───────────┼───────────┘
                          │
                    Network Driver
                          │
                    Ethernet MAC
                          │
                    TDA4VM Hardware
```

The packet-filtering component operates inside the Linux networking path and evaluates packets according to configured rules.

---

## 6. Packet Filtering Architecture

The packet-filtering subsystem uses three major rule categories:

```text
                    Incoming Packet
                           │
                           ▼
                    Network Stack
                           │
                           ▼
                    Packet Filter
                           │
              ┌────────────┼────────────┐
              │            │            │
              ▼            ▼            ▼
          Whitelist     Blacklist    Monitoring
              │            │            │
              ▼            ▼            ▼
            ALLOW        DROP/REJECT     LOG
              │            │            │
              └────────────┼────────────┘
                           │
                           ▼
                    Network/Application
```

### Whitelist

Defines traffic that is explicitly permitted.

```text
Packet
  │
  ▼
Whitelist Match
  │
  └── Match → ALLOW
```

### Blacklist

Defines traffic that must be blocked.

```text
Packet
  │
  ▼
Blacklist Match
  │
  └── Match → DROP / REJECT
```

### Monitoring

Records selected network events for analysis and debugging.

```text
Packet
  │
  ▼
Monitoring Rules
  │
  └── Match → LOG / MONITOR
```

The configuration files are stored under:

```text
configs/rules/
├── whitelist.conf
├── blacklist.conf
└── monitoring.conf
```

---

## 7. Configuration Architecture

The project separates build-time configuration from runtime packet-filter rules.

```text
configs/
│
├── kernel/
│   └── packet_filter_defconfig
│
└── rules/
    ├── whitelist.conf
    ├── blacklist.conf
    └── monitoring.conf
```

### Kernel Configuration

`packet_filter_defconfig` contains the Linux kernel configuration required for the packet-filtering functionality.

### Runtime Rules

The rule files define the actual network filtering policy.

This separation allows the kernel functionality to remain stable while filtering policies can be modified independently.

---

## 8. Yocto Architecture

Yocto is used to construct the embedded Linux system.

```text
                Yocto Project
                     │
          ┌──────────┼──────────┐
          │          │          │
       Recipes     Layers    Configuration
          │          │          │
          └──────────┼──────────┘
                     │
                 BitBake
                     │
          ┌──────────┼──────────┐
          │          │          │
        Kernel     U-Boot      RootFS
          │          │          │
          └──────────┼──────────┘
                     │
                     ▼
             Linux Image / SD
                     │
                     ▼
              BeagleBone AI-64
```

The Yocto build integrates:

* Linux kernel
* Device Tree
* U-Boot
* Root filesystem
* Packet-filter driver
* Configuration files
* User-space utilities
* Test applications

---

## 9. Device Tree Architecture

The Device Tree describes the hardware configuration to Linux.

```text
                     Device Tree
                          │
       ┌──────────────────┼──────────────────┐
       │                  │                  │
      CPU               Memory           Peripherals
                                            │
                         ┌──────────────────┼───────────────┐
                         │          │       │       │       │
                       Ethernet    I2C     SPI    UART    GPIO
                         │
                         ▼
                  Ethernet Driver
```

The Device Tree provides information such as:

* Hardware addresses
* Interrupts
* Clock configuration
* GPIO assignments
* Peripheral enablement
* Ethernet configuration
* PHY configuration

---

## 10. Networking Architecture

The networking path is approximately:

```text
Ethernet PHY
     │
     ▼
Ethernet MAC
     │
     ▼
Linux Ethernet Driver
     │
     ▼
Linux Network Stack
     │
     ▼
Netfilter / Packet Filtering
     │
     ▼
TCP/IP
     │
     ▼
Socket Layer
     │
     ▼
User Application
```

For outgoing packets, the path is reversed:

```text
User Application
      │
      ▼
Socket API
      │
      ▼
TCP/IP Stack
      │
      ▼
Packet Filtering
      │
      ▼
Ethernet Driver
      │
      ▼
Ethernet MAC
      │
      ▼
PHY
```

---

## 11. Driver Architecture

The driver layer provides communication between Linux kernel subsystems and the TDA4VM hardware.

```text
User Space
     │
     │ System Calls / Netlink / Configuration
     ▼
Kernel Space
     │
     ├── Packet Filter
     │
     ├── Network Stack
     │
     ├── Ethernet Driver
     │
     └── Device Drivers
             │
             ▼
        TDA4VM Hardware
```

The packet-filter driver is responsible for implementing the required filtering functionality and integrating it with the Linux networking subsystem.

---

## 12. User-Space Architecture

User-space components provide configuration, monitoring, logging, and testing.

```text
+--------------------------------------+
|          User Applications           |
+--------------------------------------+
|       Packet Filter Daemon           |
+--------------------------------------+
| Configuration | Logging | Monitoring |
+--------------------------------------+
|              Linux API              |
+--------------------------------------+
|              Kernel                  |
+--------------------------------------+
```

The user-space software can load filtering rules, monitor packet activity, and report filtering events.

---

## 13. Debugging Architecture

Debugging is performed at multiple layers.

```text
Application
     │
     ▼
User-Space Service
     │
     ▼
Kernel
     │
     ├── Driver
     ├── Network Stack
     └── Packet Filter
     │
     ▼
Device Tree
     │
     ▼
Hardware
```

Typical debugging mechanisms include:

```text
Serial Console
     │
     ├── U-Boot Logs
     ├── Kernel Logs
     └── Application Logs

Linux Tools
     │
     ├── dmesg
     ├── ip
     ├── ethtool
     ├── tcpdump
     └── journalctl

Hardware Debugging
     │
     └── JTAG
```

More detailed debugging procedures are documented in:

```text
docs/debugging.md
```

---

## 14. Testing Architecture

Testing is performed at multiple levels.

```text
                 Testing
                    │
        ┌───────────┼───────────┐
        │           │           │
       Unit      Integration   Hardware
        │           │           │
        ▼           ▼           ▼
      Driver     Network      Board
      Tests       Tests       Tests
        │           │           │
        └───────────┼───────────┘
                    ▼
              System Validation
```

The test environment validates:

* Kernel configuration
* Driver loading
* Ethernet connectivity
* Whitelist rules
* Blacklist rules
* Monitoring rules
* Packet filtering
* Logging
* System stability

---

## 15. Complete System Flow

The complete project flow can be summarized as:

```text
                         Power ON
                            │
                            ▼
                       Boot ROM
                            │
                            ▼
                       TIFS/SPL
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
                      Root Filesystem
                            │
                            ▼
                   Network Initialization
                            │
                            ▼
                    Ethernet Driver
                            │
                            ▼
                     Network Stack
                            │
                            ▼
                    Packet Filtering
                            │
              ┌─────────────┼─────────────┐
              │             │             │
              ▼             ▼             ▼
          Whitelist      Blacklist     Monitoring
              │             │             │
              ▼             ▼             ▼
            ALLOW          DROP          LOG
              │             │             │
              └─────────────┼─────────────┘
                            │
                            ▼
                     User Application
```

---

## 16. Project Directory Relationship

The architecture maps directly to the repository structure:

```text
BeagleBone_AI-64–TI_TDA4VM/
│
├── configs/
│   ├── kernel/
│   │   └── packet_filter_defconfig
│   └── rules/
│       ├── whitelist.conf
│       ├── blacklist.conf
│       └── monitoring.conf
│
├── driver/
│   ├── packet_filter.c
│   └── packet_filter.h
│
├── userspace/
│   └── packet_filter_daemon.c
│
├── scripts/
│   ├── build.sh
│   ├── clean.sh
│   ├── deploy.sh
│   ├── flash_sd.sh
│   ├── run.sh
│   └── test.sh
│
├── tests/
│
├── yocto/
│   └── meta-beaglebone-ai64/
│
├── docs/
│   ├── architecture.md
│   ├── debugging.md
│   ├── boot-flow.md
│   ├── build-flow.md
│   ├── kernel.md
│   ├── device-tree.md
│   ├── driver-development.md
│   ├── packet-filter.md
│   ├── networking.md
│   ├── yocto.md
│   ├── flashing.md
│   ├── testing.md
│   └── troubleshooting.md
│
├── Makefile
├── CMakeLists.txt
└── README.md
```

---

## 17. Summary

The project follows a layered embedded Linux architecture:

```text
Hardware
   ↓
Boot ROM / Firmware
   ↓
SPL / U-Boot
   ↓
Linux Kernel
   ↓
Device Tree
   ↓
Drivers
   ↓
Network Stack
   ↓
Packet Filter
   ↓
User-Space Services
   ↓
Applications
```

This architecture provides a clear separation between hardware initialization, operating-system functionality, driver development, packet filtering, configuration management, and user-space applications.

The remaining documentation files describe each layer in greater detail.

