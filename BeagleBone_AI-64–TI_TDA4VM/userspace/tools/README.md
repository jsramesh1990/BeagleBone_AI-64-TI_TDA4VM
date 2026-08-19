File relationship

Your userspace side is now:

userspace/
├── config/
│   └── default_rules.conf
│
├── include/
│   └── libfilter.h
│
├── lib/
│   └── libfilter.c
│
└── tools/
    ├── benchmark.c
    └── filter_ctl.c

The main control flow is:

filter_ctl
    |
    | command-line
    v
libfilter.c
    |
    | ioctl()
    v
/dev/packet_filter
    |
    v
packet_filter.c
    |
    +--> packet_parser.c
    |
    +--> rule_engine.c
    |
    +--> statistics.c
    |
    +--> logging.c

Example commands after the driver is loaded:

sudo ./filter_ctl status
sudo ./filter_ctl version
sudo ./filter_ctl list
sudo ./filter_ctl stats
sudo ./filter_ctl load userspace/config/default_rules.conf
sudo ./filter_ctl monitor on
sudo ./filter_ctl interface eth0
sudo ./filter_ctl default-action drop

Add a rule:

sudo ./filter_ctl add \
    --src-ip 192.168.1.100 \
    --dst-ip 192.168.1.1 \
    --dst-port 22 \
    --protocol tcp \
    --action allow

Delete it:

sudo ./filter_ctl delete 1

The next important file to make consistent is userspace/tools/benchmark.c, because it should use this same libfilter.h API rather than directly duplicating ioctl handling.




What this benchmark measures
benchmark.c
    |
    +-- Rule ADD
    |      |
    |      +-- ioctl()
    |
    +-- Rule LOOKUP
    |      |
    |      +-- ioctl()
    |
    +-- Packet PROCESS
    |      |
    |      +-- ioctl()
    |
    +-- Statistics READ
    |      |
    |      +-- ioctl()
    |
    +-- Rule LIST
    |      |
    |      +-- ioctl()
    |
    +-- Rule DELETE
           |
           +-- ioctl()

Run it like:

cd BeagleBone_AI-64–TI_TDA4VM


gcc -Wall -Wextra -O2 \
    -Iuserspace/include \
    -Ikernel/packet_filter \
    userspace/tools/benchmark.c \
    userspace/lib/libfilter.c \
    -o benchmark

Then:

sudo ./benchmark

Or a larger test:

sudo ./benchmark -n 10000 -r 100 -p 100000

For your project, keep the distinction clear:

userspace/tools/benchmark.c
        |
        | measures API/ioctl overhead
        v
kernel driver




tests/performance/benchmark.c
        |
        | should measure actual filtering
        | / packet-path performance
        v
Ethernet packet path

So the two benchmark.c files have different purposes and should not be duplicates.


Their responsibilities are now separated:

filter_ctl.c
    |
    +-- add/delete/list rules
    +-- enable/disable rules
    +-- load/reload configuration
    +-- interface configuration
    +-- default action
    +-- monitoring control




filter_stats.c
    |
    +-- show statistics
    +-- reset statistics
    +-- rule statistics
    +-- packet/byte counters
    +-- packet rate
    +-- Mbps
    +-- continuous monitoring




benchmark.c
    |
    +-- API latency
    +-- ioctl performance
    +-- rule operations
    +-- packet-processing benchmark

Example:

sudo ./filter_stats show

Reset:

sudo ./filter_stats reset

Continuous monitoring:

sudo ./filter_stats monitor

Every 5 seconds:

sudo ./filter_stats monitor 5

One important point: this file assumes the filter_statistics_t and filter_rule_t definitions in your existing userspace/include/libfilter.h contain the fields used above, especially packets_received, packets_processed, packets_allowed, packets_dropped, bytes_processed, rule_matches, parser_errors, and match_count. If your current libfilter.h uses different field names, that header should be made authoritative before compiling.




Your userspace tools are now:

userspace/
├── config/
│   └── default_rules.conf
│
├── include/
│   └── libfilter.h
│
├── lib/
│   └── libfilter.c
│
└── tools/
    ├── benchmark.c
    ├── filter_ctl.c
    ├── filter_stats.c
    └── filter_test.c

The purpose is cleanly separated:

filter_ctl.c
    → Configuration/control CLI


filter_stats.c
    → Runtime statistics


benchmark.c
    → Performance measurement


filter_test.c
    → Functional validation

Run:

sudo ./filter_test

Quick validation:

sudo ./filter_test quick

Full validation:

sudo ./filter_test full

The full test flow is:

Initialize driver
      ↓
Check API/driver version
      ↓
Check network interface
      ↓
Clear old rules
      ↓
Add test rule
      ↓
Read rule back
      ↓
Verify rule count
      ↓
Disable rule
      ↓
Re-enable rule
      ↓
Generate test packet
      ↓
Verify ALLOW decision
      ↓
Read statistics
      ↓
Enable monitoring
      ↓
Disable monitoring
      ↓
Test default action
      ↓
Delete test rule
      ↓
PASS / FAIL summary

One important implementation point: this filter_test.c is intentionally using the same libfilter.h API as filter_ctl.c, filter_stats.c, and benchmark.c; it should not contain direct ioctl() calls. That keeps the architecture:

filter_test.c
filter_ctl.c
filter_stats.c
benchmark.c
        │
        ▼
   libfilter.c
        │
        ▼
   ioctl_defs.h
        │
        ▼
 packet_filter.c



