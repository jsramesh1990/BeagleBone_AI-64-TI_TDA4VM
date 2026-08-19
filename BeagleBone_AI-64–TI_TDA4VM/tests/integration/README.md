Make it executable:

chmod +x tests/integration/test_filtering.sh

Run on the BeagleBone AI-64:

sudo ./tests/integration/test_filtering.sh

Or specify the Ethernet interface:

sudo INTERFACE=eth0 ./tests/integration/test_filtering.sh

The integration test validates this complete path:

                TEST FILTERING
                     │
                     ▼
             Network Interface
                     │
                     ▼
              packet_filter.ko
                     │
                     ▼
              Packet Reception
                     │
                     ▼
              packet_parser.c
                     │
                     ▼
               rule_engine.c
                /     |      \
               /      |       \
              ▼       ▼        ▼
        WHITELIST  BLACKLIST  MONITORING
              \       |        /
               \      |       /
                ▼     ▼       ▼
                  Decision
                     │
          ┌──────────┴──────────┐
          ▼                     ▼
       ACCEPT                  DROP
          │                     │
          └──────────┬──────────┘
                     ▼
              statistics.c
                     │
                     ▼
                logging.c
                     │
                     ▼
             Integration Report

Results are stored under:

build/test-results/integration/
└── YYYYMMDD_HHMMSS/
    ├── integration.log
    ├── summary.csv
    ├── report.txt
    ├── dmesg.txt
    ├── lsmod.txt
    ├── modinfo.txt
    ├── interface_addr.txt
    ├── interface_statistics.txt
    └── routes.txt

This gives you the integration-level test, while your existing scripts/test_suite.sh remains the higher-level complete project test runner.




Make it executable:

chmod +x tests/integration/test_ioctl.sh

Run it:

sudo ./tests/integration/test_ioctl.sh

Check IOCTL status only:

sudo ./tests/integration/test_ioctl.sh status

The test validates this path:

userspace
    │
    │ ioctl()
    ▼
/dev/packet_filter
    │
    ▼
packet_filter.c
    │
    ▼
ioctl_defs.h
    │
    ├── GET_STATUS
    ├── GET_STATS
    ├── LIST_RULES
    ├── ADD_RULE
    ├── DELETE_RULE
    └── CLEAR_RULES
            │
            ▼
       rule_engine.c
            │
            ▼
       statistics.c
            │
            ▼
        logging.c

Note: this script deliberately discovers the actual IOCTL userspace utility and device node instead of inventing a fixed ABI. Your kernel/packet_filter/ioctl_defs.h remains the source of truth for the actual IOCTL command numbers and structures.





Make it executable:

chmod +x tests/integration/test_sysfs.sh

Run:

sudo ./tests/integration/test_sysfs.sh

Check only SYSFS status:

sudo ./tests/integration/test_sysfs.sh status

The test follows your driver architecture:

                 packet_filter.ko
                        │
                        ▼
              Kernel Driver
                        │
                        ▼
              /sys/class/packet_filter/
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
       status        enabled       statistics
          │             │             │
          │             │       ┌─────┴─────┐
          │             │       ▼           ▼
          │             │   accepted      dropped
          │             │
          └─────────────┼──────────────┐
                        ▼              ▼
                    rules          monitoring
                        │              │
                        └──────┬───────┘
                               ▼
                         packet_filter

The results will be saved under:

build/test-results/sysfs/
└── 20260819_HHMMSS/
    ├── sysfs_test.log
    ├── summary.csv
    ├── report.txt
    ├── sysfs_files.txt
    ├── lsmod.txt
    ├── modinfo.txt
    ├── dmesg.txt
    └── values/

Important: the exact SYSFS attribute names must match what your packet_filter.c actually creates with device_create_file(), sysfs_create_group(), or a similar API. The script treats the listed attributes as expected/common attributes and skips optional ones that your driver does not implement.



