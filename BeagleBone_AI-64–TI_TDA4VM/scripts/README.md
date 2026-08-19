Then make it executable:

chmod +x scripts/build.sh

Run:

./scripts/build.sh

Or for the TDA4VM ARM64 cross-build:

ARCH=arm64 \
CROSS_COMPILE=aarch64-linux-gnu- \
./scripts/build.sh
Build flow represented by this script
scripts/build.sh
       │
       ▼
Check tools
       │
       ▼
Check project structure
       │
       ▼
Check packet-filter sources
       │
       ▼
Check whitelist / blacklist / monitoring
       │
       ▼
Kernel configuration
       │
       ▼
Linux Kernel
       │
       ▼
Device Tree
       │
       ▼
packet_filter.ko
       │
       ▼
Validate module
       │
       ▼
Yocto image
       │
       ▼
Build artifacts

One thing to fix before using this script: your project currently has configs/kernel/packet_filter_defconfig from the earlier structure, so the script expects exactly:

BeagleBone_AI-64–TI_TDA4VM/
├── configs/
│   ├── kernel/
│   │   └── packet_filter_defconfig
│   └── rules/
│       ├── blacklist.conf
│       ├── monitoring.conf
│       └── whitelist.conf
├── kernel/
│   └── packet_filter/
│       ├── Makefile
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
│       └── statistics.h
└── scripts/
    ├── build.sh
    └── clean.sh

This gives you a proper top-level build entry point; 
clean.sh should be the next script to complete.




[200~Make it executable:

chmod +x scripts/clean.sh
Usage

Normal clean:

./scripts/clean.sh

Driver only:

./scripts/clean.sh driver

Kernel only:

./scripts/clean.sh kernel

Deep clean:

./scripts/clean.sh deep
Clean flow
                    clean.sh
                       │
             ┌─────────┼─────────┐
             │         │         │
             ▼         ▼         ▼
          Driver     Build     Temp
             │         │         │
             ▼         ▼         ▼
          *.o/.ko   build/    *.tmp
          *.mod     logs/     *.swp
             │
             └─────────┬─────────┘
                       │
                 deep clean
                       │
             ┌─────────┴─────────┐
             ▼                   ▼
        Linux Kernel          Device Tree
        build artifacts      *.dtb/*.dtbo
             │                   │
             └─────────┬─────────┘
                       ▼
                  kernel/.config

Your scripts/ directory is now:

scripts/
├── build.sh
└── clean.sh

The next useful scripts for the complete project are deploy.sh, flash_sd.sh, run.sh, and test.sh







Make it executable:

chmod +x scripts/configure.sh
Usage

Full configuration:

./scripts/configure.sh

Verify only:

./scripts/configure.sh verify

Show configuration:

./scripts/configure.sh show

The resulting flow is:

configure.sh
     │
     ├── Check Kernel
     │
     ├── Check packet_filter_defconfig
     │
     ├── Check whitelist.conf
     ├── Check blacklist.conf
     ├── Check monitoring.conf
     │
     ├── Check Driver Sources
     │
     ├── Backup existing .config
     │
     ├── Apply packet_filter_defconfig
     │
     ├── make olddefconfig
     │
     ├── Verify CONFIG_NET
     ├── Verify CONFIG_NETFILTER
     ├── Verify CONFIG_MODULES
     │
     └── configuration-summary.txt

Your scripts/ directory becomes:

scripts/
├── build.sh
├── clean.sh
└── configure.sh

And importantly, this script does not modify your source-controlled:

configs/kernel/packet_filter_defconfig
configs/rules/whitelist.conf
configs/rules/blacklist.conf
configs/rules/monitoring.conf

It uses them as the project's configuration inputs and generates the working kernel .config.




Make it executable:

chmod +x scripts/install.sh
Usage on the BeagleBone AI-64 itself

After copying the project/build artifacts to the board:

sudo ./scripts/install.sh

Install only the kernel module:

sudo ./scripts/install.sh module

Install only the rule files:

sudo ./scripts/install.sh rules

Verify:

sudo ./scripts/install.sh verify
Installing into a mounted RootFS

For example:

sudo TARGET_ROOTFS=/mnt/beaglebone-rootfs \
    KERNEL_VERSION=6.x.x \
    ./scripts/install.sh

The resulting target filesystem will contain:

/
├── etc/
│   ├── modules-load.d/
│   │   └── packet_filter.conf
│   │
│   ├── packet_filter/
│   │   ├── packet_filter.conf
│   │   └── rules/
│   │       ├── whitelist.conf
│   │       ├── blacklist.conf
│   │       └── monitoring.conf
│   │
│   └── systemd/
│       └── system/
│           └── packet-filter.service
│
├── lib/
│   └── modules/
│       └── <kernel-version>/
│           └── extra/
│               └── packet_filter.ko
│
├── usr/
│   └── sbin/
│       └── packet_filter_ctl
│
├── var/
│   └── log/
│       └── packet_filter/
│
└── run/
    └── packet_filter/

So your scripts directory is now:

scripts/
├── build.sh
├── clean.sh
├── configure.sh
└── install.sh

One important point: the packet_filter_ctl userspace utility is optional in this script. If you have not created userspace/packet_filter_ctl yet, installation will continue and simply report that it was skipped.





Then:

chmod +x scripts/load_driver.sh
Usage on BeagleBone AI-64

After installation:

sudo ./scripts/load_driver.sh load

Check:

sudo ./scripts/load_driver.sh status

You should see:

DRIVER STATUS


Module:
  packet_filter


Status:
  [LOADED]

Check kernel messages:

sudo ./scripts/load_driver.sh logs

Unload:

sudo ./scripts/load_driver.sh unload

Reload:

sudo ./scripts/load_driver.sh reload

Test:

sudo ./scripts/load_driver.sh test
Driver lifecycle
                 packet_filter.ko
                        │
                        ▼
             load_driver.sh load
                        │
                        ▼
                     insmod
                  / modprobe
                        │
                        ▼
               Linux Kernel Module
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
       Parser       Rule Engine    Statistics
          │             │             │
          └─────────────┼─────────────┘
                        ▼
                  Packet Filter
                        │
                        ▼
                  Network Stack

Your scripts/ directory is now:

scripts/
├── build.sh
├── clean.sh
├── configure.sh
├── install.sh
└── load_driver.sh

This keeps building, installation, and runtime driver loading as separate operations, which is the cleaner structure for your BeagleBone AI-64 BSP project.




Make it executable:

chmod +x scripts/perf_test.sh
Basic performance test
sudo ./scripts/perf_test.sh

Default test duration is 30 seconds.

Test a specific Ethernet interface
sudo INTERFACE=eth0 \
    ./scripts/perf_test.sh
60-second network performance test
sudo INTERFACE=eth0 \
    DURATION=60 \
    ./scripts/perf_test.sh network
With iperf3

If another machine is running:

iperf3 -s

then on the BeagleBone AI-64:

sudo INTERFACE=eth0 \
    IPERF_SERVER=192.168.1.100 \
    DURATION=60 \
    ./scripts/perf_test.sh network
Quick test
sudo ./scripts/perf_test.sh quick
Driver-only performance information
sudo ./scripts/perf_test.sh driver

The results are stored separately for every run:

build/
└── performance/
    └── 20260819_103000/
        └── performance.txt

The script measures the important areas for your packet-filter project:

                    perf_test.sh
                         │
          ┌──────────────┼──────────────┐
          │              │              │
          ▼              ▼              ▼
        CPU            Memory        Network
          │              │              │
          ▼              ▼              ▼
      CPU usage       RAM usage      RX/TX bytes
      CPU freq        Slab           RX/TX packets
      Load avg        /proc/meminfo  Dropped packets
          │                             │
          └──────────────┬──────────────┘
                         ▼
                 Packet Filter Driver
                         │
              ┌──────────┼──────────┐
              ▼          ▼          ▼
            Module     IRQs       SoftIRQs
              │          │          │
              └──────────┼──────────┘
                         ▼
                   iperf3 / UDP
                         │
                         ▼
                  Performance Report

So your scripts/ directory is now:

scripts/
├── build.sh
├── clean.sh
├── configure.sh
├── install.sh
├── load_driver.sh
└── perf_test.sh

This gives the project a clean separation between build → configure → install → load → performance test.





Make it executable:

chmod +x scripts/stress_test.sh
Basic 60-second stress test
sudo ./scripts/stress_test.sh
5-minute full stress test
sudo DURATION=300 \
     INTERFACE=eth0 \
     ./scripts/stress_test.sh
Network + packet-filter stress

Run an iperf3 server on another machine:

iperf3 -s

Then on the BeagleBone AI-64:

sudo DURATION=300 \
     INTERFACE=eth0 \
     IPERF_SERVER=192.168.1.100 \
     UDP_BANDWIDTH=500M \
     PACKET_SIZE=1024 \
     ./scripts/stress_test.sh driver
What this script stresses
                    stress_test.sh
                          │
          ┌───────────────┼────────────────┐
          │               │                │
          ▼               ▼                ▼
      CPU Stress      Memory Stress    Network Stress
          │               │                │
          │               │                ▼
          │               │             iperf3/UDP
          │               │                │
          └───────────────┼────────────────┘
                          ▼
                  Packet Filter Driver
                          │
              ┌───────────┼───────────┐
              ▼           ▼           ▼
          Packet       Rule Engine  Statistics
          Parser           │
              │            │
              └────────────┘
                     │
                     ▼
                Linux Network
                     │
          ┌──────────┼──────────┐
          ▼          ▼          ▼
        IRQs      SoftIRQs    CPU Load
                     │
                     ▼
               Stress Monitor
                     │
                     ▼
             Stress Test Report

Results are stored separately for each run:

build/
└── stress/
    └── 20260819_103000/
        ├── stress_test.txt
        ├── monitor.csv
        ├── cpu.log
        ├── memory.log
        ├── network.log
        ├── interrupts.log
        ├── traffic.log
        ├── dmesg_before.txt
        └── dmesg_after.txt

Your scripts directory is now:

scripts/
├── build.sh
├── clean.sh
├── configure.sh
├── install.sh
├── load_driver.sh
├── perf_test.sh
└── stress_test.sh

This gives you a separate performance test and stress test: perf_test.sh measures normal performance, while stress_test.sh deliberately creates sustained CPU/memory/network load and checks whether the packet-filter driver remains stable.




Make it executable:

chmod +x scripts/test_suite.sh
Run the complete suite
sudo ./scripts/test_suite.sh
Functional tests only
sudo INTERFACE=eth0 ./scripts/test_suite.sh functional
Driver-only tests
sudo ./scripts/test_suite.sh driver
Network tests
sudo INTERFACE=eth0 ./scripts/test_suite.sh network
With stress/iperf testing
sudo INTERFACE=eth0 \
     IPERF_SERVER=192.168.1.100 \
     DURATION=30 \
     STRESS_DURATION=60 \
     ./scripts/test_suite.sh

The results will be organized as:

build/
└── test-results/
    └── 20260819_103000/
        ├── test_suite.txt
        ├── test_suite.log
        ├── summary.csv
        ├── driver_load.log
        ├── performance.log
        └── stress.log

Your scripts/ directory is now:

scripts/
├── build.sh
├── clean.sh
├── configure.sh
├── install.sh
├── load_driver.sh
├── perf_test.sh
├── stress_test.sh
└── test_suite.sh

The intended flow is:

build.sh
   │
   ▼
configure.sh
   │
   ▼
install.sh
   │
   ▼
load_driver.sh
   │
   ▼
test_suite.sh
   │
   ├── Functional Tests
   ├── Driver Tests
   ├── Network Tests
   ├── Performance Tests
   └── Stress Tests
           │
           ▼
      Test Report




Then:

chmod +x scripts/unload_driver.sh

Your driver lifecycle is now:

                 build.sh
                    │
                    ▼
             packet_filter.ko
                    │
                    ▼
             load_driver.sh
                    │
                    ▼
        ┌───────────────────────┐
        │ packet_filter loaded  │
        └───────────┬───────────┘
                    │
              test_suite.sh
                    │
        ┌───────────┼────────────┐
        ▼           ▼            ▼
    functional   performance   stress
        │           │            │
        └───────────┼────────────┘
                    ▼
             unload_driver.sh
                    │
                    ▼
        ┌───────────────────────┐
        │ packet_filter removed │
        └───────────────────────┘
Normal unload
sudo ./scripts/unload_driver.sh
Check status
sudo ./scripts/unload_driver.sh status
Force unload
sudo ./scripts/unload_driver.sh force

Important: normal modprobe -r/rmmod should be preferred. The force option is mainly for development/debugging and can be unsafe if the driver has active references or has an incomplete cleanup path.



