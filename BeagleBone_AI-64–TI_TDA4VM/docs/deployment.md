# Deployment Guide – BeagleBone AI-64 / TI TDA4VM

## 1. Overview

This document describes the deployment process for the **BeagleBone AI-64 based on the Texas Instruments TDA4VM**.

Deployment covers the process of taking the generated software components from the development host and installing them onto the target board.

The deployment flow includes:

* Building the Linux BSP
* Building the kernel
* Building Device Tree
* Building the packet-filter driver
* Building user-space applications
* Creating the root filesystem
* Preparing the boot media
* Copying software to the target
* Booting the board
* Loading and validating the packet-filter functionality

---

## 2. Deployment Architecture

```text
                    Development Host
                           │
                           ▼
                    Source Repository
                           │
                           ▼
                    Build System
                           │
             ┌─────────────┼─────────────┐
             │             │             │
             ▼             ▼             ▼
          U-Boot         Kernel        RootFS
             │             │             │
             │             ├── DTB      │
             │             └── Driver   │
             │                         │
             └─────────────┬───────────┘
                           ▼
                    Deployment Image
                           │
                           ▼
                    SD / eMMC / Target
                           │
                           ▼
                   BeagleBone AI-64
                           │
                           ▼
                       Linux Boot
                           │
                           ▼
                  Packet Filter System
```

---

# 3. Deployment Components

The final deployment normally contains:

```text
Deployment
├── Bootloader
├── Linux Kernel
├── Device Tree Blob
├── Root Filesystem
├── Kernel Modules
├── Packet Filter Driver
├── User-Space Application
├── Configuration Files
└── Test Utilities
```

The important project files are:

```text
configs/
├── kernel/
│   └── packet_filter_defconfig
└── rules/
    ├── whitelist.conf
    ├── blacklist.conf
    └── monitoring.conf
```

---

# 4. Host Preparation

The development host should contain the required build tools and cross-compilation environment.

Verify the compiler:

```bash
aarch64-linux-gnu-gcc --version
```

Verify Git:

```bash
git --version
```

Verify Make:

```bash
make --version
```

Verify Python:

```bash
python3 --version
```

For Yocto-based builds, ensure the required host dependencies are installed before starting the build.

---

# 5. Get the Source Code

Clone the project:

```bash
git clone <repository-url>
```

Enter the project:

```bash
cd BeagleBone_AI-64–TI_TDA4VM
```

Check the repository:

```bash
git status
```

Expected project structure:

```text
BeagleBone_AI-64–TI_TDA4VM/
├── configs/
├── driver/
├── userspace/
├── scripts/
├── tests/
├── yocto/
├── docs/
├── Makefile
├── CMakeLists.txt
└── README.md
```

---

# 6. Build the Project

The preferred build entry point is:

```bash
./scripts/build.sh
```

If the script is not executable:

```bash
chmod +x scripts/*.sh
```

Then:

```bash
./scripts/build.sh
```

The build process should generate the required deployment artifacts.

Typical output:

```text
build/
├── Image
├── *.dtb
├── modules/
├── packet_filter.ko
└── packet_filter_daemon
```

The exact filenames depend on the selected Yocto/kernel configuration.

---

# 7. Kernel Deployment

The Linux kernel image must be copied to the boot partition or included in the generated boot image.

Typical kernel artifact:

```text
Image
```

Check the generated image:

```bash
ls -lh build/Image
```

The kernel is loaded by U-Boot during the boot process.

```text
U-Boot
   │
   ▼
Load Image
   │
   ▼
Load Device Tree
   │
   ▼
Boot Linux
```

---

# 8. Device Tree Deployment

The Device Tree Blob is generated from the Device Tree source.

Typical artifact:

```text
*.dtb
```

Check:

```bash
find build/ -name "*.dtb"
```

The DTB must correspond to the target hardware configuration.

Deployment flow:

```text
DTS
 │
 ▼
Device Tree Compiler
 │
 ▼
DTB
 │
 ▼
Boot Media
 │
 ▼
U-Boot
 │
 ▼
Linux Kernel
```

---

# 9. Root Filesystem Deployment

The RootFS contains the user-space environment.

It includes:

```text
/bin
/sbin
/etc
/lib
/usr
/var
/home
```

Project-specific components can be installed into the RootFS:

```text
/usr/bin/packet_filter_daemon
/etc/packet-filter/
    ├── whitelist.conf
    ├── blacklist.conf
    └── monitoring.conf
```

The exact installation paths should remain consistent between the build system, deployment scripts, and runtime application.

---

# 10. Packet Filter Driver Deployment

If the packet-filter driver is built as a kernel module:

```text
packet_filter.ko
```

Copy it to the target:

```bash
scp build/packet_filter.ko root@<target-ip>:/tmp/
```

On the target:

```bash
sudo cp /tmp/packet_filter.ko /lib/modules/$(uname -r)/
```

Update module dependencies:

```bash
sudo depmod -a
```

Load the module:

```bash
sudo modprobe packet_filter
```

Verify:

```bash
lsmod | grep packet_filter
```

Check kernel logs:

```bash
dmesg | grep -i packet_filter
```

---

# 11. User-Space Application Deployment

Deploy the packet-filter daemon:

```bash
scp build/packet_filter_daemon root@<target-ip>:/usr/bin/
```

Make it executable:

```bash
sudo chmod +x /usr/bin/packet_filter_daemon
```

Verify:

```bash
ls -l /usr/bin/packet_filter_daemon
```

Run manually for initial testing:

```bash
sudo /usr/bin/packet_filter_daemon
```

---

# 12. Rule Configuration Deployment

Create the configuration directory:

```bash
sudo mkdir -p /etc/packet-filter
```

Copy the rule files:

```bash
scp configs/rules/whitelist.conf root@<target-ip>:/tmp/
scp configs/rules/blacklist.conf root@<target-ip>:/tmp/
scp configs/rules/monitoring.conf root@<target-ip>:/tmp/
```

On the target:

```bash
sudo cp /tmp/whitelist.conf /etc/packet-filter/
sudo cp /tmp/blacklist.conf /etc/packet-filter/
sudo cp /tmp/monitoring.conf /etc/packet-filter/
```

Verify:

```bash
ls -l /etc/packet-filter/
```

Expected:

```text
blacklist.conf
monitoring.conf
whitelist.conf
```

---

# 13. SD Card Deployment

For SD-card based deployment, first identify the SD device carefully.

On the host:

```bash
lsblk
```

Example:

```text
sdb
├── sdb1
└── sdb2
```

**Do not assume the device name. Verify it before writing an image.**

If the project provides an image-generation script:

```bash
./scripts/flash_sd.sh
```

The script should perform the necessary image preparation and copying.

A typical deployment flow is:

```text
Build
  │
  ▼
Generate Image
  │
  ▼
Prepare SD Card
  │
  ▼
Copy Boot Files
  │
  ▼
Copy RootFS
  │
  ▼
Sync
  │
  ▼
Insert SD Card
  │
  ▼
Boot Board
```

---

# 14. Deployment Using the Project Script

The recommended project-level deployment command is:

```bash
./scripts/deploy.sh
```

A typical deployment script performs:

```text
1. Validate build artifacts
2. Validate target
3. Copy kernel
4. Copy DTB
5. Copy driver
6. Copy user-space application
7. Copy configuration files
8. Set permissions
9. Update modules
10. Restart required services
11. Run basic validation
```

Example:

```bash
./scripts/deploy.sh
```

---

# 15. Network Deployment

For a board already running Linux and connected through Ethernet, deployment can be performed over SSH.

Check connectivity:

```bash
ping <target-ip>
```

Connect:

```bash
ssh root@<target-ip>
```

Copy files:

```bash
scp build/packet_filter.ko root@<target-ip>:/tmp/
```

```bash
scp build/packet_filter_daemon root@<target-ip>:/tmp/
```

This is useful during development because it avoids repeatedly rebuilding and rewriting the complete SD card.

---

# 16. Remote Deployment Flow

```text
Development PC
      │
      │ SSH / SCP
      ▼
Ethernet
      │
      ▼
BeagleBone AI-64
      │
      ├── Kernel Module
      ├── Application
      ├── Configuration
      └── Test Utilities
      │
      ▼
Restart / Reload
      │
      ▼
Validation
```

---

# 17. Driver Installation

After copying the driver:

```bash
sudo depmod -a
```

Load:

```bash
sudo modprobe packet_filter
```

Check:

```bash
lsmod | grep packet_filter
```

Check logs:

```bash
dmesg | tail -50
```

Expected initialization should indicate that the driver successfully registered and initialized.

---

# 18. Start the Packet Filter

Start the user-space service or daemon:

```bash
sudo /usr/bin/packet_filter_daemon
```

If a systemd service is provided:

```bash
sudo systemctl start packet-filter
```

Check:

```bash
sudo systemctl status packet-filter
```

Enable it during boot:

```bash
sudo systemctl enable packet-filter
```

---

# 19. Verify Configuration

Check the deployed configuration:

```bash
cat /etc/packet-filter/whitelist.conf
```

```bash
cat /etc/packet-filter/blacklist.conf
```

```bash
cat /etc/packet-filter/monitoring.conf
```

Confirm that the running system is using the expected configuration.

---

# 20. Deployment Validation

After deployment, first verify Linux:

```bash
uname -a
```

Then verify the board:

```bash
cat /proc/cpuinfo
```

Verify networking:

```bash
ip link
```

```bash
ip addr
```

Verify Ethernet:

```bash
ethtool eth0
```

Verify the driver:

```bash
lsmod | grep packet_filter
```

Verify the application:

```bash
ps aux | grep packet_filter
```

Verify logs:

```bash
dmesg | grep -i packet_filter
```

---

# 21. Packet Filter Validation

Test an allowed packet:

```text
Packet
  │
  ▼
Whitelist
  │
  ▼
ALLOW
```

Test a blocked packet:

```text
Packet
  │
  ▼
Blacklist
  │
  ▼
DROP
```

Test monitoring:

```text
Packet
  │
  ▼
Monitoring Rule
  │
  ▼
LOG
```

Capture network traffic:

```bash
sudo tcpdump -i eth0
```

Check packet-filter logs:

```bash
dmesg | grep -i filter
```

---

# 22. Post-Deployment Verification

Run the project test script:

```bash
./scripts/test.sh
```

A complete validation should verify:

```text
Boot
 │
 ├── Kernel
 ├── Device Tree
 └── RootFS
       │
       ▼
Networking
 │
 ├── Interface
 ├── IP
 └── Ethernet Link
       │
       ▼
Packet Filter
 │
 ├── Driver
 ├── Whitelist
 ├── Blacklist
 └── Monitoring
       │
       ▼
Application
```

---

# 23. Rollback

If the newly deployed software causes problems, retain the previous working image.

Recommended structure:

```text
Deployment Images
├── release/
│   └── known-good-image
└── development/
    └── latest-image
```

For a driver rollback:

```bash
sudo modprobe -r packet_filter
```

Restore the previous module:

```bash
sudo cp <previous-driver>.ko /lib/modules/$(uname -r)/
```

Then:

```bash
sudo depmod -a
sudo modprobe packet_filter
```

For a complete system rollback, boot the previously validated SD/eMMC image.

---

# 24. Deployment Troubleshooting

### Kernel does not boot

Check:

```bash
printenv bootcmd
printenv bootargs
```

Verify:

* Kernel image
* DTB
* Boot media
* Boot arguments
* Serial console

---

### Driver does not load

Check:

```bash
dmesg | grep -i packet_filter
```

```bash
modinfo packet_filter
```

Verify:

* Kernel version
* Module compatibility
* Kernel configuration
* Device Tree
* Required symbols

---

### Ethernet does not work

Check:

```bash
ip link
```

```bash
ethtool eth0
```

```bash
dmesg | grep -i ethernet
```

Verify:

* PHY
* Device Tree
* Pinmux
* MAC configuration
* Network cable

---

### Packet filter does not block traffic

Check:

```bash
cat /etc/packet-filter/blacklist.conf
```

Then:

```bash
sudo tcpdump -i eth0
```

and:

```bash
dmesg | grep -i filter
```

Verify:

* Configuration was loaded.
* Rule syntax is valid.
* Driver is active.
* Packet hook is registered.
* Rule matching is working.

---

# 25. Complete Deployment Flow

```text
                         Source Code
                              │
                              ▼
                         Git Repository
                              │
                              ▼
                         Yocto / Build
                              │
             ┌────────────────┼────────────────┐
             │                │                │
             ▼                ▼                ▼
           U-Boot           Kernel           RootFS
                              │
                              ├── DTB
                              └── Driver
                                               │
                                               ▼
                                      Packet Filter App
                                               │
                                               ▼
                                      Configuration Files
                                               │
                                               ▼
                                      Deployment Image
                                               │
                         ┌─────────────────────┴──────────────────┐
                         │                                        │
                         ▼                                        ▼
                       SD Card                                Network/SSH
                         │                                        │
                         └─────────────────────┬──────────────────┘
                                               ▼
                                      BeagleBone AI-64
                                               │
                                               ▼
                                           Boot Linux
                                               │
                                               ▼
                                      Load Driver / App
                                               │
                                               ▼
                                      Load Filtering Rules
                                               │
                                               ▼
                                      Network Validation
                                               │
                                               ▼
                                      Packet Filter Testing
```

---

# 26. Deployment Checklist

## Build

* [ ] Source code is synchronized
* [ ] Kernel builds successfully
* [ ] Device Tree builds successfully
* [ ] U-Boot builds successfully
* [ ] RootFS builds successfully
* [ ] Packet-filter driver builds successfully
* [ ] User-space application builds successfully

## Deployment

* [ ] Boot artifacts are available
* [ ] Kernel image is available
* [ ] DTB is available
* [ ] RootFS is available
* [ ] Driver is available
* [ ] Application is available
* [ ] Configuration files are available
* [ ] SD/network deployment completed

## Target

* [ ] Board boots
* [ ] Linux starts
* [ ] RootFS mounts
* [ ] Ethernet works
* [ ] Driver loads
* [ ] Application starts
* [ ] Configuration loads
* [ ] Whitelist works
* [ ] Blacklist works
* [ ] Monitoring works
* [ ] Tests pass

---

# 27. Deployment Best Practices

1. Always keep a known-good boot image.
2. Validate the kernel and DTB before deployment.
3. Deploy the driver only after confirming kernel compatibility.
4. Test the Ethernet interface before testing packet filtering.
5. Validate configuration files before loading them.
6. Use SSH/SCP for fast development iterations.
7. Use a complete SD/eMMC image for release deployment.
8. Keep deployment scripts reproducible.
9. Record the deployed software version.
10. Always run the project test suite after deployment.

---

# 28. Version Tracking

Every deployment should identify the software version.

Example:

```text
Project:
BeagleBone_AI-64–TI_TDA4VM

Release:
v1.0.0

Kernel:
<kernel-version>

U-Boot:
<u-boot-version>

Yocto:
<yocto-version>

Packet Filter:
<packet-filter-version>

Build Date:
<date>
```

This makes it possible to reproduce and debug deployed releases.

---

# 29. Final Deployment Model

The deployment process follows:

```text
SOURCE
  ↓
BUILD
  ↓
PACKAGE
  ↓
FLASH / COPY
  ↓
BOOT
  ↓
INITIALIZE
  ↓
LOAD DRIVER
  ↓
LOAD CONFIGURATION
  ↓
START APPLICATION
  ↓
TEST
  ↓
VALIDATE
  ↓
RELEASE
```

The objective is to make deployment **repeatable, testable, and recoverable**, ensuring that the same source tree can consistently produce a working BeagleBone AI-64 software image.

