# Device Tree Guide – BeagleBone AI-64 / TI TDA4VM

## 1. Overview

The **Device Tree** is a hardware description mechanism used by Linux to describe the hardware available on the BeagleBone AI-64 based on the **TI TDA4VM** SoC.

The Device Tree allows the Linux kernel to understand hardware resources without hard-coding board-specific information directly into drivers.

It describes:

* CPUs
* Memory
* Interrupts
* GPIOs
* Clocks
* Pin multiplexing
* Ethernet
* I2C
* SPI
* UART
* USB
* PCIe
* Storage
* Regulators
* PHYs
* DMA
* Peripheral enable/disable status

The general relationship is:

```text
Hardware
    │
    ▼
Device Tree Source (.dts/.dtsi)
    │
    ▼
Device Tree Compiler (dtc)
    │
    ▼
Device Tree Blob (.dtb)
    │
    ▼
U-Boot
    │
    ▼
Linux Kernel
    │
    ▼
Device Tree Nodes
    │
    ▼
Driver Matching
    │
    ▼
Driver probe()
```

---

# 2. Why Device Tree Is Required

Linux drivers need information about the physical hardware.

For example, a driver may need to know:

```text
Register Address
Interrupt Number
Clock
GPIO
DMA Channel
PHY
Pin Configuration
```

Instead of embedding these values inside the driver, Device Tree provides them.

```text
              Device Tree
                   │
       ┌───────────┼────────────┐
       │           │            │
   Registers   Interrupts     GPIO
       │           │            │
       └───────────┼────────────┘
                   ▼
                 Driver
                   │
                   ▼
                Hardware
```

This makes the same Linux driver reusable across multiple boards.

---

# 3. Device Tree Source Files

Device Tree commonly uses two file types:

```text
.dts
.dtsi
```

### `.dts`

The board-specific Device Tree source.

Example:

```text
board.dts
```

### `.dtsi`

Common or reusable hardware description.

Example:

```text
soc.dtsi
```

Typical relationship:

```text
Common SoC Description
        │
        ▼
     .dtsi
        │
        ▼
Board Description
        │
        ▼
     .dts
        │
        ▼
      .dtb
```

---

# 4. Device Tree Structure

A simplified Device Tree looks like:

```dts
/dts-v1/;

/ {
    model = "BeagleBone AI-64";

    compatible = "ti,j721e";

    memory@80000000 {
        device_type = "memory";
        reg = <0x0 0x80000000 0x0 0x80000000>;
    };

    chosen {
        stdout-path = &main_uart0;
    };
};
```

The Device Tree is hierarchical.

```text
/
├── chosen
├── memory
├── aliases
├── reserved-memory
├── cpus
├── bus
│   ├── ethernet
│   ├── i2c
│   ├── spi
│   └── uart
└── peripherals
```

---

# 5. Root Node

The root node is represented by:

```dts
/ {
};
```

Important root properties include:

```dts
model = "BeagleBone AI-64";

compatible = "ti,j721e";
```

The `model` property identifies the board.

The `compatible` property is used to identify the hardware to the kernel and relevant drivers.

---

# 6. `compatible` Property

The `compatible` property is one of the most important Device Tree properties.

Example:

```dts
compatible = "vendor,device";
```

It connects a Device Tree node with a driver.

Conceptually:

```text
Device Tree
    │
    │ compatible
    ▼
Driver Match Table
    │
    ▼
probe()
```

Example driver:

```c
static const struct of_device_id packet_filter_of_match[] = {
    {
        .compatible = "example,packet-filter",
    },
    { }
};
```

Device Tree:

```dts
packet-filter {
    compatible = "example,packet-filter";
};
```

The kernel can then match the node with the driver.

---

# 7. `status` Property

The `status` property controls whether a hardware node is enabled.

Example:

```dts
&some_peripheral {
    status = "okay";
};
```

Disabled:

```dts
&some_peripheral {
    status = "disabled";
};
```

Typical values:

```text
okay
disabled
reserved
fail
```

For normal enabled peripherals:

```dts
status = "okay";
```

---

# 8. Registers

Hardware registers are described using the `reg` property.

Example:

```dts
device@10000000 {
    reg = <0x0 0x10000000 0x0 0x1000>;
};
```

Conceptually:

```text
reg
 │
 ├── Base Address
 │
 └── Size
```

The driver can retrieve the resource using kernel APIs instead of hard-coding the physical address.

---

# 9. Interrupts

Hardware interrupts are described using the `interrupts` property.

Conceptual example:

```dts
device@10000000 {
    interrupts = <0 100 4>;
};
```

The exact interrupt-controller format depends on the platform.

The relationship is:

```text
Hardware Event
      │
      ▼
Interrupt Controller
      │
      ▼
Linux IRQ
      │
      ▼
Driver Interrupt Handler
```

---

# 10. GPIO

GPIOs can be described using GPIO controller references.

Example:

```dts
reset-gpios = <&gpio0 20 GPIO_ACTIVE_LOW>;
```

The driver can request the GPIO using the kernel GPIO framework.

Flow:

```text
Device Tree
    │
    ▼
GPIO Number
    │
    ▼
GPIO Framework
    │
    ▼
Driver
    │
    ▼
Hardware Pin
```

---

# 11. Pin Multiplexing

TDA4VM pins can support different peripheral functions.

For example, one physical pin may be configured for:

```text
GPIO
UART
I2C
SPI
PWM
```

The pinmux configuration determines which function is active.

Conceptually:

```text
Physical Pin
     │
     ▼
Pin Multiplexer
     │
 ┌───┼────┬────┐
 ▼   ▼    ▼    ▼
GPIO UART I2C SPI
```

Incorrect pinmux configuration can cause an otherwise correct driver to fail.

---

# 12. Clocks

Many peripherals require clocks.

Device Tree can describe clock relationships:

```text
Peripheral
    │
    ▼
Clock Provider
    │
    ▼
Clock Configuration
    │
    ▼
Peripheral Enabled
```

If a required clock is unavailable, the driver may fail during `probe()`.

Typical kernel error:

```text
failed to get clock
```

---

# 13. DMA

High-performance peripherals may use DMA to transfer data without requiring the CPU to copy every byte.

Conceptually:

```text
Peripheral
    │
    ▼
DMA Controller
    │
    ▼
Memory
```

Device Tree can describe DMA channels and related resources.

---

# 14. Ethernet Device Tree Architecture

The Ethernet configuration is especially important for this project.

Simplified architecture:

```text
                 TDA4VM
                   │
              Ethernet MAC
                   │
                   ▼
                 MDIO
                   │
                   ▼
                  PHY
                   │
                   ▼
               Connector
```

The Device Tree describes the relationship between the MAC and PHY.

Conceptually:

```dts
&ethernet_controller {
    status = "okay";

    phy-handle = <&phy0>;
};
```

The exact node names and properties depend on the TDA4VM board Device Tree.

---

# 15. PHY Configuration

The PHY provides the physical Ethernet interface.

```text
Linux
  │
  ▼
Ethernet Driver
  │
  ▼
MAC
  │
  ▼
MDIO
  │
  ▼
PHY
  │
  ▼
RJ45 / Ethernet
```

When Ethernet fails, check:

```bash
dmesg | grep -i phy
```

```bash
dmesg | grep -i ethernet
```

And:

```bash
ethtool eth0
```

---

# 16. UART Device Tree

UART nodes contain information required by the serial driver.

Conceptually:

```dts
&main_uart0 {
    status = "okay";
};
```

The boot console may reference a UART through:

```dts
chosen {
    stdout-path = &main_uart0;
};
```

Flow:

```text
Bootloader
    │
    ▼
UART
    │
    ▼
Linux Console
    │
    ▼
Serial Terminal
```

This is particularly important for board bring-up and debugging.

---

# 17. I2C Device Tree

I2C controllers can have child devices.

Conceptually:

```dts
&i2c0 {
    status = "okay";

    sensor@48 {
        compatible = "vendor,sensor";
        reg = <0x48>;
    };
};
```

Architecture:

```text
I2C Controller
      │
      ├── Device 0
      │
      ├── Device 1
      │
      └── Device 2
```

The `reg` property normally represents the I2C slave address for the child device.

---

# 18. SPI Device Tree

SPI devices are normally represented as children of an SPI controller.

Conceptually:

```dts
&spi0 {
    status = "okay";

    device@0 {
        compatible = "vendor,device";
        reg = <0>;
        spi-max-frequency = <10000000>;
    };
};
```

Architecture:

```text
SPI Controller
     │
     ├── CS0 → Device
     ├── CS1 → Device
     └── CS2 → Device
```

---

# 19. Device Tree for the Packet Filter

The packet-filter component can use Device Tree if it requires board-specific hardware resources.

For example:

```dts
packet_filter {
    compatible = "example,packet-filter";
    status = "okay";
};
```

If the driver requires resources:

```dts
packet_filter {
    compatible = "example,packet-filter";
    status = "okay";

    interrupts = <...>;
    clocks = <...>;
    memory-region = <...>;
};
```

The actual properties must match the driver implementation.

---

# 20. Driver and Device Tree Matching

The complete matching process is:

```text
Device Tree Node
       │
       ▼
compatible property
       │
       ▼
Kernel Device Model
       │
       ▼
Driver Match Table
       │
       ▼
Match Found
       │
       ▼
probe()
       │
       ▼
Resource Initialization
       │
       ▼
Driver Ready
```

If the `compatible` strings do not match:

```text
Device Tree
    │
    ▼
No Driver Match
    │
    ▼
probe() not called
```

---

# 21. Device Tree Compilation

The Device Tree Compiler (`dtc`) converts source into a binary DTB.

```text
board.dts
    │
    ▼
dtc
    │
    ▼
board.dtb
```

Example:

```bash
dtc -I dts -O dtb -o board.dtb board.dts
```

In a Linux kernel build, Device Tree compilation is normally handled automatically by the kernel build system.

---

# 22. Device Tree Build Flow

Typical kernel build:

```bash
make ARCH=arm64 <defconfig>
```

Then:

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
```

The build generates the kernel and required Device Tree blobs according to the selected configuration.

Check for DTBs:

```bash
find arch/arm64/boot/dts/ -name "*.dtb"
```

The exact directory layout can vary with the kernel source tree and vendor BSP.

---

# 23. Device Tree Deployment

The DTB is normally placed in the boot filesystem.

Typical structure:

```text
/boot/
├── Image
├── *.dtb
└── overlays/
```

U-Boot loads:

```text
Kernel Image
     +
Device Tree Blob
     │
     ▼
Linux Kernel
```

The kernel then uses the Device Tree during hardware initialization.

---

# 24. Inspecting the Running Device Tree

After Linux boots:

```bash
ls /proc/device-tree/
```

Inspect compatible properties:

```bash
find /proc/device-tree/ -name compatible -print
```

A binary property can be examined using:

```bash
xxd /proc/device-tree/<node>/compatible
```

The live Device Tree is also exposed through:

```text
/sys/firmware/devicetree/base/
```

For example:

```bash
ls /sys/firmware/devicetree/base/
```

---

# 25. Device Tree Debugging

When a driver does not probe:

```text
Driver Failure
      │
      ▼
Check dmesg
      │
      ▼
Check compatible
      │
      ▼
Check status
      │
      ▼
Check registers
      │
      ▼
Check interrupts
      │
      ▼
Check clocks
      │
      ▼
Check GPIO
      │
      ▼
Check pinmux
      │
      ▼
Check dependencies
```

Useful commands:

```bash
dmesg | grep -i probe
```

```bash
dmesg | grep -i device
```

```bash
dmesg | grep -i clock
```

```bash
dmesg | grep -i gpio
```

```bash
dmesg | grep -i interrupt
```

---

# 26. Common Device Tree Problems

## 26.1 Incorrect `compatible`

```text
Device Tree compatible
        ≠
Driver compatible
```

Result:

```text
Driver does not probe
```

---

## 26.2 Node Disabled

```dts
status = "disabled";
```

Result:

```text
Hardware node is not initialized
```

Change to:

```dts
status = "okay";
```

when appropriate for the board configuration.

---

## 26.3 Incorrect Pinmux

```text
Peripheral
    │
    ▼
Wrong Pin Configuration
    │
    ▼
Hardware Does Not Work
```

Typical symptoms:

* UART has no output.
* I2C device cannot be detected.
* SPI communication fails.
* GPIO does not change state.
* Ethernet PHY cannot establish a link.

---

## 26.4 Incorrect Interrupt

Symptoms can include:

* Device initializes but does not generate events.
* Interrupt handler never executes.
* Timeout errors.
* Hardware appears stuck.

Check:

```bash
cat /proc/interrupts
```

---

## 26.5 Missing Clock

Typical symptom:

```text
failed to get clock
```

Check:

```bash
dmesg | grep -i clock
```

---

## 26.6 Incorrect Register Address

If the `reg` property is incorrect, the driver may access the wrong hardware region.

Symptoms can include:

```text
Probe failure
Timeout
Bus error
Hardware malfunction
```

---

# 27. Device Tree Overlay Concept

A Device Tree overlay can modify or add hardware configuration without replacing the complete base Device Tree.

Conceptually:

```text
Base DTB
   │
   +
Overlay
   │
   ▼
Modified Device Tree
```

This is useful when enabling optional hardware or peripherals.

However, whether a particular overlay mechanism is available and supported depends on the board's bootloader/kernel configuration.

---

# 28. Device Tree and Yocto

In a Yocto BSP, Device Tree sources are generally integrated into the kernel recipe or a board-specific kernel layer.

Typical relationship:

```text
Yocto
  │
  ▼
Kernel Recipe
  │
  ├── Kernel Configuration
  │
  ├── Device Tree Sources
  │
  └── Kernel Patches
  │
  ▼
BitBake
  │
  ▼
Kernel + DTB
```

A project-specific Yocto layer can provide Device Tree modifications using kernel recipes, patches, configuration fragments, or board-specific files depending on the BSP structure.

---

# 29. Device Tree Modification Workflow

The recommended development workflow is:

```text
1. Identify hardware
       ↓
2. Find existing Device Tree node
       ↓
3. Understand controller/PHY relationships
       ↓
4. Modify DTS/DTSI
       ↓
5. Compile DTB
       ↓
6. Build kernel/image
       ↓
7. Deploy DTB
       ↓
8. Boot board
       ↓
9. Check dmesg
       ↓
10. Verify driver probe
       ↓
11. Test hardware
```

---

# 30. Device Tree and Driver Development

Device Tree and drivers must be designed together.

```text
             Device Tree
                  │
                  │ Resources
                  ▼
               Driver
                  │
                  │ Initialization
                  ▼
              Hardware
```

For example:

```text
Device Tree
    │
    ├── compatible
    ├── reg
    ├── interrupts
    ├── clocks
    ├── GPIOs
    └── DMA
         │
         ▼
       probe()
         │
         ▼
   Resource Request
         │
         ▼
 Hardware Initialization
```

---

# 31. Device Tree Debugging Checklist

### Board

* [ ] Correct board Device Tree selected
* [ ] Correct SoC Device Tree included
* [ ] Correct `.dts` file selected
* [ ] Required `.dtsi` files included

### Nodes

* [ ] Node exists
* [ ] `compatible` is correct
* [ ] `status = "okay"`
* [ ] `reg` is correct
* [ ] Interrupts are correct
* [ ] Clocks are available
* [ ] GPIOs are correct
* [ ] Pinmux is correct

### Driver

* [ ] Driver is built
* [ ] Driver is loaded
* [ ] `compatible` matches
* [ ] `probe()` executes
* [ ] Resources are successfully requested

### Runtime

* [ ] DTB deployed
* [ ] Running DT contains expected node
* [ ] `dmesg` shows successful initialization
* [ ] Hardware responds correctly

---

# 32. Complete Device Tree Flow

```text
                   TDA4VM Hardware
                          │
                          ▼
                  Hardware Resources
                          │
                          ▼
                    Device Tree
                          │
              ┌───────────┼────────────┐
              │           │            │
             CPU        Memory     Peripherals
                                      │
                         ┌────────────┼────────────┐
                         │            │            │
                      Ethernet       I2C          SPI
                         │
                         ▼
                        PHY
                         │
                         ▼
                    Linux Driver
                         │
                         ▼
                       probe()
                         │
                         ▼
                Hardware Initialized
```

---

# 33. Final Device Tree Model

The Device Tree is the connection between the **physical TDA4VM hardware** and the **Linux software stack**.

```text
Hardware
   │
   ▼
Device Tree
   │
   ├── CPU
   ├── Memory
   ├── Ethernet
   ├── PHY
   ├── GPIO
   ├── I2C
   ├── SPI
   ├── UART
   ├── Clocks
   ├── Interrupts
   └── DMA
   │
   ▼
Linux Device Model
   │
   ▼
Driver Matching
   │
   ▼
probe()
   │
   ▼
Driver Initialization
   │
   ▼
Working Hardware
```

The key debugging principle is:

> **When a hardware peripheral does not work, verify the Device Tree description before modifying the driver. The Device Tree must correctly describe the hardware resources and must match the driver's expected `compatible` entry.**

