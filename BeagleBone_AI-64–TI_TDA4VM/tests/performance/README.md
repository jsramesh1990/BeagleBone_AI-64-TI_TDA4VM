Compile

From the project root:

gcc -Wall -Wextra -O2 \
    -o tests/performance/benchmark \
    tests/performance/benchmark.c

If your ioctl_defs.h contains the actual IOCTL definitions, I recommend changing the benchmark to include that project header rather than maintaining duplicate definitions:

#include "../../kernel/packet_filter/ioctl_defs.h"

Then remove the fallback #ifndef PACKET_FILTER_IOCTL_MAGIC block from the benchmark.

Run
sudo ./tests/performance/benchmark

For a shorter test:

sudo ./tests/performance/benchmark --iterations 10000

For a larger performance run:

sudo ./tests/performance/benchmark --iterations 1000000

Specify the device:

sudo ./tests/performance/benchmark \
    --device /dev/packet_filter \
    --iterations 100000
What this benchmark measures
                   benchmark.c
                       │
                       ├───────────────┐
                       │               │
                       ▼               ▼
              /dev/packet_filter   CPU/Memory
                       │
                       ▼
                  ioctl()
                       │
             ┌─────────┴─────────┐
             ▼                   ▼
        GET_STATUS           GET_STATS
             │                   │
             ▼                   ▼
        Driver latency       Statistics
             │                   │
             └─────────┬─────────┘
                       ▼
                  Performance
                    Results

It reports:

IOCTL GET_STATUS
    ├── Operations/sec
    ├── Average latency
    ├── Minimum latency
    ├── Maximum latency
    └── CPU time


IOCTL GET_STATS
    ├── Operations/sec
    ├── Average latency
    ├── Minimum latency
    └── Maximum latency


Packet Buffer
    ├── Operations/sec
    └── Mpps


System
    ├── CPU information
    ├── Memory information
    ├── Context switches
    ├── Page faults
    └── CPU usage

Important: because your ioctl_defs.h is the real ABI for this project, the IOCTL numbers/structures in this benchmark should ultimately be taken directly from that file. If your current ioctl_defs.h has different names or structures, those definitions need to be used here rather than the fallback definitions above.






Compile
cd BeagleBone_AI-64–TI_TDA4VM


gcc -Wall -Wextra -O2 \
    -o tests/performance/packet_generator \
    tests/performance/packet_generator.c
First test — don't transmit

Always start with a dry run:

./tests/performance/packet_generator \
    --dry-run \
    --dump \
    --count 1
Generate UDP packets
sudo ./tests/performance/packet_generator \
    --interface eth0 \
    --protocol udp \
    --size 512 \
    --count 10000
Generate TCP packets
sudo ./tests/performance/packet_generator \
    --interface eth0 \
    --protocol tcp \
    --size 512 \
    --count 10000
Generate ICMP packets
sudo ./tests/performance/packet_generator \
    --interface eth0 \
    --protocol icmp \
    --size 128 \
    --count 1000
Controlled packet rate

For 10,000 packets/sec:

sudo ./tests/performance/packet_generator \
    --interface eth0 \
    --protocol udp \
    --size 512 \
    --rate 10000 \
    --count 100000
Continuous stress test
sudo ./tests/performance/packet_generator \
    --interface eth0 \
    --protocol udp \
    --size 1024 \
    --rate 50000 \
    --continuous

Stop with:

Ctrl+C
Randomized traffic

This is useful for testing your packet parser + rule engine:

sudo ./tests/performance/packet_generator \
    --interface eth0 \
    --protocol udp \
    --size 512 \
    --random \
    --count 10000
Explicit packet addresses
sudo ./tests/performance/packet_generator \
    --interface eth0 \
    --src-mac 02:00:00:00:00:01 \
    --dst-mac 02:00:00:00:00:02 \
    --src-ip 192.168.1.100 \
    --dst-ip 192.168.1.1 \
    --src-port 12345 \
    --dst-port 8080 \
    --protocol udp \
    --size 512 \
    --count 1000
How this fits your project
tests/performance/packet_generator.c
              │
              │ creates
              ▼
       Ethernet Frame
              │
              ▼
          IPv4 Packet
              │
       ┌──────┼──────┐
       ▼      ▼      ▼
      UDP    TCP    ICMP
       │      │      │
       └──────┼──────┘
              │
              ▼
        Ethernet NIC
              │
              ▼
      BeagleBone AI-64
              │
              ▼
       Linux Network Stack
              │
              ▼
      Packet Filter Driver
              │
       ┌──────┼──────────┐
       ▼      ▼          ▼
   Whitelist Blacklist Monitoring
       │      │          │
       └──────┼──────────┘
              ▼
        Rule Engine
              │
       ┌──────┴──────┐
       ▼             ▼
    ACCEPT          DROP
       │             │
       ▼             ▼
   Statistics      Statistics

This makes packet_generator.c the traffic source for your performance and integration testing, while benchmark.c measures the driver/API performance.
