# `docs/yocto-build.md`

````markdown
# Yocto Build – BeagleBone AI-64 / TI TDA4VM

## 1. Overview

This document describes the complete Yocto build process for the
BeagleBone AI-64 platform based on the TI TDA4VM SoC.

The Yocto build generates the complete embedded Linux software stack:

```text
Yocto Project
     │
     ├── Bootloader
     │     ├── TIFS
     │     ├── SPL
     │     └── U-Boot
     │
     ├── Linux Kernel
     │
     ├── Device Tree
     │
     ├── Root Filesystem
     │
     ├── Kernel Modules
     │
     ├── Packet Filter Driver
     │
     ├── User-Space Application
     │
     └── Configuration Files
              │
              ▼
       Bootable Image
              │
              ▼
      BeagleBone AI-64
````

---

# 2. Build Architecture

The complete build flow is:

```text
Source Code
     │
     ▼
Yocto Layers
     │
     ▼
BitBake Recipes
     │
     ▼
Configuration
     │
     ├── local.conf
     └── bblayers.conf
     │
     ▼
BitBake
     │
     ├── Fetch
     ├── Unpack
     ├── Patch
     ├── Configure
     ├── Compile
     ├── Install
     ├── Package
     └── RootFS Generation
     │
     ▼
Bootloader + Kernel + DTB + RootFS
     │
     ▼
Deploy Directory
     │
     ▼
SD / eMMC
     │
     ▼
BeagleBone AI-64
```

---

# 3. Required Host Environment

Recommended host:

```text
Ubuntu Linux
Git
Python
Build tools
Yocto dependencies
Cross compiler
```

The host should have sufficient:

```text
CPU
RAM
Disk space
Network connectivity
```

A Yocto build can consume significant disk space, particularly when
downloads and shared state are retained.

---

# 4. Recommended Host Packages

Install the required development packages before initializing the build
environment.

Example:

```bash
sudo apt update
```

Then install the packages required by the selected Yocto release.

Typical packages include:

```bash
sudo apt install \
    gawk \
    wget \
    git \
    diffstat \
    unzip \
    texinfo \
    gcc \
    build-essential \
    chrpath \
    socat \
    cpio \
    python3 \
    python3-pip \
    python3-pexpect \
    xz-utils \
    debianutils \
    iputils-ping \
    file \
    locales \
    zstd \
    lz4 \
    libacl1 \
    liblz4-tool
```

The exact dependency list should follow the Yocto release documentation used
by the project.

---

# 5. Locale Configuration

Yocto requires a valid UTF-8 locale.

Check:

```bash
locale
```

Example:

```text
LANG=en_US.UTF-8
```

If the locale is unavailable:

```bash
sudo locale-gen en_US.UTF-8
sudo update-locale LANG=en_US.UTF-8
```

Open a new terminal and verify:

```bash
locale
```

---

# 6. Project Directory

Recommended workspace:

```text
~/BeagleBone_AI-64–TI_TDA4VM/
```

Example:

```bash
mkdir -p ~/BeagleBone_AI-64–TI_TDA4VM
cd ~/BeagleBone_AI-64–TI_TDA4VM
```

Recommended repository structure:

```text
BeagleBone_AI-64–TI_TDA4VM/
│
├── configs/
│   ├── kernel/
│   └── rules/
│
├── docs/
│   ├── architecture.md
│   ├── boot-flow.md
│   ├── build-flow.md
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
├── scripts/
│
├── userspace/
│
├── yocto/
│
└── README.md
```

---

# 7. Yocto Directory Structure

A typical Yocto workspace contains:

```text
yocto/
├── poky/
├── meta-openembedded/
├── meta-ti/
├── meta-arm/
├── meta-custom/
├── build/
└── downloads/
```

The exact layers depend on the selected TI/Yocto release.

---

# 8. Yocto Layers

The build can contain:

```text
+----------------------------+
|        Custom Layer        |
|                            |
| Packet Filter Driver       |
| User Application           |
| Configuration              |
+-------------+--------------+
              |
+-------------▼--------------+
|          meta-ti           |
|                            |
| TI SoC / Board Support     |
+-------------+--------------+
              |
+-------------▼--------------+
|      meta-openembedded     |
|                            |
| Additional packages        |
+-------------+--------------+
              |
+-------------▼--------------+
|            poky            |
|                            |
| oe-core + BitBake          |
+-----------------------------+
```

---

# 9. Clone Yocto Source

Clone the appropriate Yocto/OE-Core distribution used by the project.

Example:

```bash
cd ~/BeagleBone_AI-64–TI_TDA4VM/yocto

git clone <yocto-repository>
```

Then select the required release:

```bash
cd <yocto-directory>

git checkout <yocto-release>
```

Do not mix arbitrary branches from unrelated Yocto releases.

---

# 10. TI Yocto Layers

For a TI TDA4VM platform, the TI BSP layer is important.

Example:

```bash
git clone <meta-ti-repository>
```

Then checkout the branch compatible with the chosen Yocto release:

```bash
cd meta-ti
git checkout <compatible-branch>
```

The exact branch must match the Yocto release.

---

# 11. Additional Layers

Depending on project requirements:

```text
meta-openembedded
meta-ti
meta-arm
custom project layer
```

Example:

```bash
git clone <meta-openembedded-repository>
git clone <meta-arm-repository>
```

Always use release-compatible branches.

---

# 12. Custom Project Layer

Create a project layer for the packet-filter software.

Example:

```text
meta-packet-filter/
├── conf/
│   └── layer.conf
├── recipes-kernel/
│   └── packet-filter/
│       ├── packet-filter.bb
│       └── files/
├── recipes-apps/
│   └── packet-filter-userspace/
│       ├── packet-filter-userspace.bb
│       └── files/
└── recipes-core/
    └── images/
        └── packet-filter-image.bb
```

This keeps project-specific changes separate from upstream layers.

---

# 13. Layer Configuration

Example:

```text
meta-packet-filter/
└── conf/
    └── layer.conf
```

The layer configuration defines:

```text
BBPATH
BBFILES
BBFILE_COLLECTIONS
BBFILE_PATTERN
BBFILE_PRIORITY
```

Example conceptual configuration:

```bitbake
BBFILE_COLLECTIONS += "packet-filter"

BBFILE_PATTERN_packet-filter := "^${LAYERDIR}/"

BBFILE_PRIORITY_packet-filter = "10"

BBFILES += "${LAYERDIR}/recipes-*/*/*.bb"
BBFILES += "${LAYERDIR}/recipes-*/*/*.bbappend"
```

---

# 14. Initialize Build Environment

From the Yocto source directory:

```bash
source oe-init-build-env build
```

After this:

```text
build/
├── conf/
│   ├── local.conf
│   └── bblayers.conf
```

The shell environment is now configured for BitBake.

---

# 15. Build Configuration

The two main configuration files are:

```text
build/conf/local.conf
build/conf/bblayers.conf
```

---

# 16. local.conf

`local.conf` controls build-specific settings.

Typical configuration includes:

```text
MACHINE
DISTRO
PACKAGE_CLASSES
IMAGE_FSTYPES
DL_DIR
SSTATE_DIR
EXTRA_IMAGE_FEATURES
```

Example:

```bitbake
MACHINE ?= "<board-machine>"
```

The exact machine name must be taken from the BSP layer being used.

Do not blindly assume the machine name.

---

# 17. bblayers.conf

`bblayers.conf` defines the layers used by BitBake.

Conceptually:

```text
BBLAYERS
├── poky/meta
├── poky/meta-poky
├── meta-openembedded/meta-oe
├── meta-openembedded/meta-networking
├── meta-ti
├── meta-arm
└── meta-packet-filter
```

Verify:

```bash
bitbake-layers show-layers
```

---

# 18. Verify Layers

Run:

```bash
bitbake-layers show-layers
```

Expected output should show all required layers.

Example:

```text
layer                 priority
--------------------------------
core                  5
poky                  5
meta-oe               6
meta-networking       6
meta-ti               10
meta-packet-filter    10
```

The actual priorities depend on the selected layers.

---

# 19. Verify Machine

Check available machines:

```bash
bitbake-layers show-machines
```

Search for the required TI board:

```bash
bitbake-layers show-machines | grep -i ti
```

Select the machine corresponding to the actual BeagleBone AI-64 BSP.

---

# 20. Machine Configuration

Set:

```bitbake
MACHINE = "<correct-beaglebone-ai-64-machine>"
```

Verify:

```bash
bitbake -e | grep '^MACHINE='
```

Expected:

```text
MACHINE="<selected-machine>"
```

---

# 21. Image Selection

The project can start from a standard image or a custom image.

Typical image categories:

```text
core-image-minimal
core-image-base
core-image-full-cmdline
custom packet-filter image
```

For development:

```bash
bitbake <development-image>
```

For the final project, use a custom image.

---

# 22. Custom Packet-Filter Image

Recommended:

```text
meta-packet-filter/
└── recipes-core/
    └── images/
        └── packet-filter-image.bb
```

Example:

```bitbake
require recipes-core/images/core-image-base.bb

IMAGE_INSTALL:append = " \
    packet-filter \
    packet-filter-userspace \
"
```

Additional networking/debugging packages can be added when required.

---

# 23. Packet Filter Kernel Driver Recipe

Recommended structure:

```text
recipes-kernel/
└── packet-filter/
    ├── packet-filter.bb
    └── files/
        ├── packet_filter.c
        ├── packet_filter.h
        └── Makefile
```

The recipe is responsible for:

```text
Fetch source
Patch source
Configure
Compile
Install
Package
```

---

# 24. Kernel Module Recipe

For an external kernel module, the recipe should inherit the appropriate
kernel-module support provided by the Yocto environment.

Conceptually:

```bitbake
SUMMARY = "Packet Filter Kernel Module"

inherit module

SRC_URI = "file://packet_filter.c \
           file://Makefile"

S = "${WORKDIR}"

RPROVIDES:${PN} += "kernel-module-packet-filter"
```

The exact recipe depends on how the driver is integrated.

---

# 25. In-Tree Kernel Driver

If the packet-filter driver is integrated into the Linux kernel source tree:

```text
linux/
└── drivers/
    └── net/
        └── packet-filter/
```

The Yocto kernel configuration must enable it.

Example:

```text
CONFIG_PACKET_FILTER=y
```

or:

```text
CONFIG_PACKET_FILTER=m
```

depending on the intended design.

---

# 26. Kernel Configuration

Project configuration:

```text
configs/kernel/
└── packet_filter_defconfig
```

Example:

```text
CONFIG_NET=y
CONFIG_NETFILTER=y
CONFIG_PACKET_FILTER=m
```

The exact options depend on the driver's architecture.

Do not enable unrelated kernel features without a reason.

---

# 27. Kernel Configuration Fragment

A kernel configuration fragment can be used:

```text
recipes-kernel/
└── linux/
    └── linux-*.bbappend
```

Example:

```text
files/
└── packet-filter.cfg
```

Then:

```bitbake
SRC_URI += "file://packet-filter.cfg"
```

This keeps project-specific kernel configuration isolated.

---

# 28. Device Tree Integration

If the packet-filter hardware requires Device Tree configuration:

```text
recipes-kernel/
└── linux/
    └── linux-*.bbappend
```

Example:

```text
files/
└── packet-filter.dtsi
```

The Device Tree should define only the hardware resources actually required
by the driver.

Refer to:

```text
docs/device-tree.md
```

for the detailed Device Tree design.

---

# 29. User-Space Recipe

Recommended:

```text
recipes-apps/
└── packet-filter-userspace/
    ├── packet-filter-userspace.bb
    └── files/
        ├── Makefile
        ├── src/
        └── include/
```

The recipe installs:

```text
/usr/bin/packet-filter
```

---

# 30. Configuration Recipe

Project configuration can be installed through a separate recipe or the
user-space recipe.

Target layout:

```text
/etc/packet-filter/
├── whitelist.conf
├── blacklist.conf
└── monitoring.conf
```

---

# 31. Installing Configuration

Conceptual recipe:

```bitbake
do_install() {
    install -d ${D}${sysconfdir}/packet-filter

    install -m 0644 ${WORKDIR}/whitelist.conf \
        ${D}${sysconfdir}/packet-filter/

    install -m 0644 ${WORKDIR}/blacklist.conf \
        ${D}${sysconfdir}/packet-filter/

    install -m 0644 ${WORKDIR}/monitoring.conf \
        ${D}${sysconfdir}/packet-filter/
}
```

The actual file names should match the repository.

---

# 32. Include Package in Image

Add:

```bitbake
IMAGE_INSTALL:append = " packet-filter-userspace"
```

and:

```bitbake
IMAGE_INSTALL:append = " packet-filter"
```

Then rebuild the image.

---

# 33. Dependency Relationship

The final image should contain:

```text
packet-filter-image
       │
       ├── Linux Kernel
       │
       ├── Packet Filter Driver
       │
       ├── User-Space Application
       │
       ├── Configuration Files
       │
       └── Network Utilities
```

---

# 34. Fetch Phase

BitBake first downloads source files:

```text
do_fetch
```

Typical sources:

```text
Git repositories
Tarballs
Local files
Patches
Configuration fragments
```

Downloaded files are stored in:

```text
DL_DIR
```

---

# 35. Unpack Phase

BitBake extracts source:

```text
do_unpack
```

into the recipe work directory.

---

# 36. Patch Phase

Project patches are applied:

```text
do_patch
```

Typical changes:

```text
Kernel fixes
Driver modifications
Device Tree changes
Build fixes
Configuration changes
```

---

# 37. Configure Phase

BitBake executes:

```text
do_configure
```

This prepares the source for compilation.

For kernel builds this includes:

```text
Kernel configuration
Configuration fragments
Defconfig
```

---

# 38. Compile Phase

BitBake executes:

```text
do_compile
```

Major components:

```text
U-Boot
Kernel
Device Tree
Kernel modules
User-space applications
Libraries
```

---

# 39. Install Phase

BitBake executes:

```text
do_install
```

This places files into the package staging area.

Example:

```text
${D}/usr/bin/
${D}/etc/
${D}/lib/modules/
```

---

# 40. Package Phase

BitBake packages the installed files.

Depending on configuration:

```text
RPM
DEB
IPK
```

Package files are generated before the root filesystem is assembled.

---

# 41. RootFS Generation

The selected image recipe collects packages:

```text
Packages
   │
   ▼
RootFS construction
   │
   ├── /bin
   ├── /sbin
   ├── /etc
   ├── /usr
   ├── /lib
   ├── /dev
   ├── /proc
   ├── /sys
   └── /var
```

---

# 42. Image Generation

The final image generation creates:

```text
Kernel
DTB
RootFS
Boot files
```

Output is normally available under:

```text
tmp/deploy/images/<machine>/
```

---

# 43. Deploy Directory

Check:

```bash
ls tmp/deploy/images/<machine>/
```

Typical artifacts can include:

```text
Image
*.dtb
*.wic
*.wic.xz
*.tar.bz2
modules-*.tgz
bootloader files
```

The exact file names depend on the BSP and image configuration.

---

# 44. Build Command

After configuring the environment:

```bash
bitbake <image-name>
```

Example:

```bash
bitbake packet-filter-image
```

This performs the required dependency build automatically.

---

# 45. Build a Specific Recipe

To build only the user-space application:

```bash
bitbake packet-filter-userspace
```

To build the kernel module:

```bash
bitbake packet-filter
```

To build the Linux kernel:

```bash
bitbake virtual/kernel
```

---

# 46. Force Rebuild

Normally BitBake uses task signatures and sstate to determine what needs
rebuilding.

For development:

```bash
bitbake -c compile -f packet-filter
```

Then:

```bash
bitbake packet-filter
```

Use force rebuilds carefully because they can significantly increase build
time.

---

# 47. Clean Recipe

Clean work files:

```bash
bitbake -c clean packet-filter
```

For a more aggressive clean:

```bash
bitbake -c cleansstate packet-filter
```

Use `cleansstate` only when necessary because it removes reusable shared
state for that recipe.

---

# 48. Clean Image

```bash
bitbake -c clean <image-name>
```

Then rebuild:

```bash
bitbake <image-name>
```

Avoid deleting the entire build directory unless a complete rebuild is
actually required.

---

# 49. Dependency Inspection

Show dependencies:

```bash
bitbake -g <image-name>
```

This generates dependency information that can be analyzed to understand
the build graph.

---

# 50. Environment Inspection

Inspect a variable:

```bash
bitbake -e <recipe> | grep '^MACHINE='
```

Inspect image packages:

```bash
bitbake -e <image-name> | grep '^IMAGE_INSTALL='
```

Inspect kernel provider:

```bash
bitbake -e virtual/kernel | grep '^PREFERRED_PROVIDER'
```

---

# 51. Recipe Information

Find a recipe:

```bash
bitbake-layers show-recipes | grep packet
```

Show recipe providers:

```bash
bitbake-layers show-recipes virtual/kernel
```

This helps identify which layer provides a recipe.

---

# 52. Task Listing

List available tasks:

```bash
bitbake -c listtasks <recipe>
```

Important tasks:

```text
do_fetch
do_unpack
do_patch
do_configure
do_compile
do_install
do_package
do_rootfs
do_image
do_deploy
```

---

# 53. Build Logs

Recipe work directories are located below:

```text
tmp/work/
```

Logs can be found under the relevant recipe work directory.

For example:

```text
tmp/work/
└── <architecture>/
    └── packet-filter/
        └── <version>/
```

Important files include task logs generated by BitBake.

---

# 54. Finding a Failed Task

When BitBake reports:

```text
ERROR: Task (... do_compile) failed
```

identify the recipe and task.

Then inspect the corresponding log:

```text
log.do_compile
```

Also inspect:

```text
run.do_compile
```

when command-level debugging is required.

---

# 55. Build Debugging Flow

```text
BitBake Failure
      │
      ▼
Identify Recipe
      │
      ▼
Identify Failed Task
      │
      ▼
Open log.do_<task>
      │
      ▼
Find First Real Error
      │
      ▼
Inspect Source / Dependency
      │
      ▼
Fix
      │
      ▼
Rebuild Recipe
      │
      ▼
Rebuild Image
```

Do not focus only on the final error line; the first meaningful error is
usually more useful.

---

# 56. SSTATE

Yocto uses Shared State Cache:

```text
sstate-cache/
```

Sstate allows previously completed tasks to be reused.

Flow:

```text
Task
 │
 ├── Matching sstate
 │       │
 │       ▼
 │     Reuse
 │
 └── No match
         │
         ▼
       Build
```

This significantly reduces incremental build time.

---

# 57. Downloads Directory

Downloaded source archives are stored in:

```text
downloads/
```

It is recommended to keep this directory outside disposable build
directories when maintaining multiple builds.

Example:

```text
yocto/
├── downloads/
├── sstate-cache/
├── build-debug/
└── build-release/
```

---

# 58. Multiple Build Configurations

The same source layers can support:

```text
build-debug
build-release
build-production
```

Example:

```bash
source oe-init-build-env build-debug
```

and:

```bash
source oe-init-build-env build-release
```

Each build directory maintains its own:

```text
conf/
tmp/
cache/
```

---

# 59. Debug Build

A development image can include:

```text
gdb
strace
tcpdump
ethtool
iproute2
debug symbols
kernel debugging options
```

These packages should generally be limited to development images rather than
production images.

---

# 60. Production Build

A production image should contain only required components:

```text
Bootloader
Kernel
DTB
RootFS
Packet Filter Driver
Packet Filter Application
Required Configuration
Required Network Utilities
```

Avoid unnecessary development tools.

---

# 61. Build Reproducibility

For reproducible builds, record:

```text
Yocto release
Poky commit
meta-ti commit
Other layer commits
Machine
DISTRO
Kernel version
U-Boot version
Configuration
Recipe versions
```

Generate a manifest when appropriate.

---

# 62. Git Revision Tracking

Every production image should be traceable to a Git revision.

Recommended metadata:

```text
Project Git commit
Yocto layers commits
Kernel commit
Driver commit
User-space commit
Build timestamp
```

Example:

```text
Build ID:
TDA4VM-PF-2026-08-19-001

Project commit:
<git-commit>

Kernel:
<kernel-commit>

meta-ti:
<meta-ti-commit>
```

---

# 63. Development Build Flow

```text
Developer Changes Source
        │
        ▼
Git Commit
        │
        ▼
Yocto Recipe
        │
        ▼
BitBake
        │
        ▼
Compile
        │
        ▼
Package
        │
        ▼
RootFS
        │
        ▼
Image
        │
        ▼
Flash
        │
        ▼
Boot
        │
        ▼
Test
```

---

# 64. Kernel + Driver + User-Space Integration

The complete project build is:

```text
                    Yocto
                      │
          ┌───────────┼───────────┐
          │           │           │
          ▼           ▼           ▼
      U-Boot       Kernel      User Space
          │           │           │
          │           ├── DTB     ├── CLI
          │           ├── Driver  └── Config
          │           └── Modules
          │
          └───────────┬───────────┘
                      │
                      ▼
                  RootFS/Image
                      │
                      ▼
                BeagleBone AI-64
```

---

# 65. Complete Build Flow

```text
1. Install host dependencies
        │
        ▼
2. Create Yocto workspace
        │
        ▼
3. Clone Yocto source
        │
        ▼
4. Clone TI BSP layers
        │
        ▼
5. Clone additional layers
        │
        ▼
6. Create custom layer
        │
        ▼
7. Initialize build environment
        │
        ▼
8. Configure bblayers.conf
        │
        ▼
9. Configure local.conf
        │
        ▼
10. Select MACHINE
        │
        ▼
11. Add kernel configuration
        │
        ▼
12. Add Device Tree changes
        │
        ▼
13. Add packet-filter driver
        │
        ▼
14. Add user-space application
        │
        ▼
15. Add configuration files
        │
        ▼
16. Build image
        │
        ▼
17. Check deploy artifacts
        │
        ▼
18. Flash image
        │
        ▼
19. Boot board
        │
        ▼
20. Test packet filtering
```

---

# 66. Recommended Build Commands

```bash
# Enter Yocto environment
cd ~/BeagleBone_AI-64–TI_TDA4VM/yocto
source oe-init-build-env build

# Verify layers
bitbake-layers show-layers

# Verify machine
bitbake -e | grep '^MACHINE='

# Find packet-filter recipes
bitbake-layers show-recipes | grep packet

# Build user-space application
bitbake packet-filter-userspace

# Build driver
bitbake packet-filter

# Build complete image
bitbake packet-filter-image

# Check generated images
ls tmp/deploy/images/<machine>/
```

Replace placeholders with the actual machine and recipe names used by the
repository.

---

# 67. Build Validation

After a successful build verify:

```text
[ ] BitBake completed successfully
[ ] Kernel image generated
[ ] Device Tree generated
[ ] Bootloader artifacts generated
[ ] RootFS generated
[ ] Packet filter driver packaged
[ ] User-space application packaged
[ ] Configuration files packaged
[ ] Final image generated
```

---

# 68. Target Validation

After flashing and booting:

```bash
uname -a
```

Check driver:

```bash
lsmod | grep packet
```

Check device:

```bash
ls -l /dev/packet_filter
```

Check application:

```bash
packet-filter --version
```

Check configuration:

```bash
ls -l /etc/packet-filter/
```

Check networking:

```bash
ip link
ip addr
```

---

# 69. Build-to-Test Flow

```text
                 YOCTO BUILD
                      │
                      ▼
             Bootable Image
                      │
                      ▼
                   Flash
                      │
                      ▼
             BeagleBone AI-64
                      │
                      ▼
                    Boot
                      │
                      ▼
              Kernel Validation
                      │
                      ▼
              Driver Validation
                      │
                      ▼
             User-Space Validation
                      │
                      ▼
              Rule Configuration
                      │
                      ▼
               Traffic Generation
                      │
                      ▼
              Packet Verification
                      │
                      ▼
              Performance Testing
                      │
                      ▼
                 PASS / FAIL
```

---

# 70. Common Build Problems

## 70.1 Missing Host Dependency

Error:

```text
Required program not found
```

Solution:

```text
Install the missing Yocto host dependency.
```

Then restart the build environment.

---

## 70.2 Locale Error

Error:

```text
Locale not available
```

Check:

```bash
locale
```

Generate:

```bash
sudo locale-gen en_US.UTF-8
```

---

## 70.3 Layer Not Found

Error:

```text
Layer 'meta-xxx' not found
```

Check:

```bash
bitbake-layers show-layers
```

Verify:

```text
Path
Git checkout
Branch compatibility
bblayers.conf
```

---

## 70.4 Recipe Not Found

Error:

```text
Nothing PROVIDES 'packet-filter'
```

Check:

```bash
bitbake-layers show-recipes | grep packet
```

Possible causes:

```text
Recipe missing
Layer not added
Recipe name incorrect
Layer compatibility problem
```

---

## 70.5 Machine Not Found

Error:

```text
MACHINE=<machine> not found
```

Check:

```bash
bitbake-layers show-machines
```

Use the machine definition supplied by the selected BSP.

---

## 70.6 Kernel Configuration Failure

Check:

```bash
bitbake virtual/kernel -c menuconfig
```

or inspect the kernel configuration fragments and task logs.

Verify that the driver configuration symbol exists.

---

## 70.7 Driver Build Failure

Inspect:

```text
log.do_compile
```

Verify:

```text
Kernel headers
Kernel version
KBUILD configuration
Driver source
Makefile
Symbol compatibility
```

---

## 70.8 RootFS Package Missing

If the application is not present:

```bash
which packet-filter
```

If missing, verify:

```text
IMAGE_INSTALL
Recipe
Package name
do_install()
FILES:${PN}
```

---

# 71. Clean Recovery Flow

When a recipe has an inconsistent work directory:

```bash
bitbake -c clean <recipe>
bitbake <recipe>
```

If shared state is causing an issue:

```bash
bitbake -c cleansstate <recipe>
bitbake <recipe>
```

Avoid deleting the complete `tmp/` and `sstate-cache/` directories unless
necessary.

---

# 72. CI/CD Integration

The Yocto build can later be integrated into CI.

```text
Git Push
   │
   ▼
CI Server
   │
   ▼
Checkout
   │
   ▼
Initialize Yocto
   │
   ▼
Build
   │
   ▼
Run Tests
   │
   ▼
Generate Artifacts
   │
   ▼
Archive Image
```

Recommended CI validation:

```text
Layer parsing
Recipe parsing
Kernel compilation
Driver compilation
User-space compilation
Image generation
Static checks
Unit tests
```

---

# 73. Release Artifacts

A production build should archive:

```text
Bootloader
Kernel
DTB
RootFS
SD/eMMC image
Kernel modules
Package manifest
Build manifest
Configuration
Git revision information
Release notes
```

Example:

```text
release/
├── bootloader/
├── kernel/
├── dtb/
├── rootfs/
├── images/
├── modules/
├── manifests/
└── configs/
```

---

# 74. Final Build Checklist

```text
[ ] Host dependencies installed
[ ] UTF-8 locale configured
[ ] Correct Yocto release selected
[ ] Correct meta-ti branch selected
[ ] Required layers added
[ ] Custom layer added
[ ] MACHINE configured
[ ] DISTRO configured
[ ] Kernel configuration added
[ ] Device Tree changes added
[ ] Driver recipe added
[ ] User-space recipe added
[ ] Configuration recipe/files added
[ ] Image recipe configured
[ ] BitBake parsing successful
[ ] Kernel builds successfully
[ ] Driver builds successfully
[ ] User-space builds successfully
[ ] RootFS contains required files
[ ] Final image generated
[ ] Deploy artifacts verified
[ ] Image flashed
[ ] Board boots
[ ] Driver loads
[ ] Application runs
[ ] Packet filtering tested
```

---

# 75. Final Build Principle

The Yocto build should be treated as a reproducible pipeline:

```text
Source
  │
  ▼
Layer
  │
  ▼
Recipe
  │
  ▼
BitBake
  │
  ├── Fetch
  ├── Unpack
  ├── Patch
  ├── Configure
  ├── Compile
  ├── Install
  ├── Package
  └── Image
  │
  ▼
Bootable Image
  │
  ▼
BeagleBone AI-64
  │
  ▼
Linux
  │
  ▼
Packet Filter Driver
  │
  ▼
User-Space Application
  │
  ▼
Network Packet Filtering
```

The key objective is that **one controlled Yocto build must produce the
complete software stack required by the BeagleBone AI-64 packet-filter
project**, rather than manually compiling and copying individual components
onto the target.

```
```

