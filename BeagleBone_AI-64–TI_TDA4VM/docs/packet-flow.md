# `docs/packet-flow.md`

````markdown
# Packet Flow – BeagleBone AI-64 / TI TDA4VM

## 1. Overview

This document describes the complete network packet flow through the
packet-filtering system on the BeagleBone AI-64 platform based on the
TI TDA4VM SoC.

The packet flow covers:

- Ethernet PHY
- Ethernet MAC
- Linux Ethernet driver
- Linux networking stack
- Packet-filter hook
- Packet parsing
- Whitelist processing
- Blacklist processing
- Monitoring
- Packet statistics
- ACCEPT/DROP decision
- TCP/IP processing
- User-space applications

The high-level flow is:

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
Packet Filter Hook
     │
     ▼
Packet Parser
     │
     ▼
Rule Matching
     │
 ┌───┼───────────────┐
 │   │               │
 ▼   ▼               ▼
ALLOW DROP          MONITOR
 │   │               │
 ▼   ▼               ▼
Continue Discard    Log/Event
````

---

# 2. Complete Packet Architecture

```text
                         NETWORK
                            │
                            │ Ethernet Frame
                            ▼
                    +---------------+
                    | Ethernet PHY  |
                    +-------+-------+
                            │
                            ▼
                    +---------------+
                    | Ethernet MAC  |
                    +-------+-------+
                            │
                            ▼
                    +---------------+
                    | Ethernet      |
                    | Driver        |
                    +-------+-------+
                            │
                            ▼
                    +---------------+
                    | Linux Network |
                    | Stack         |
                    +-------+-------+
                            │
                            ▼
                    +---------------+
                    | Packet Filter |
                    | Hook          |
                    +-------+-------+
                            │
                            ▼
                    +---------------+
                    | Packet Parser |
                    +-------+-------+
                            │
                            ▼
                    +---------------+
                    | Rule Matching |
                    +-------+-------+
                       │    │    │
                ALLOW  │    │    │ DROP
                       │    │    │
                       ▼    ▼    ▼
                    Continue Log Discard
                       │
                       ▼
                 Protocol Stack
                       │
                       ▼
                 Application
```

---

# 3. Packet Direction

There are two major packet paths.

## RX Path

Incoming packet:

```text
Network
   │
   ▼
PHY
   │
   ▼
MAC
   │
   ▼
Ethernet Driver
   │
   ▼
Linux Network Stack
   │
   ▼
Packet Filter
   │
   ▼
TCP/UDP/ICMP
   │
   ▼
Application
```

## TX Path

Outgoing packet:

```text
Application
   │
   ▼
TCP/UDP/ICMP
   │
   ▼
Linux Network Stack
   │
   ▼
Packet Filter
   │
   ▼
Ethernet Driver
   │
   ▼
MAC
   │
   ▼
PHY
   │
   ▼
Network
```

The exact filter-hook location depends on the implementation.

---

# 4. RX Packet Flow

The RX path starts when an Ethernet frame arrives from the network.

```text
Incoming Ethernet Frame
          │
          ▼
       Ethernet PHY
          │
          ▼
       Ethernet MAC
          │
          ▼
     DMA / RX Ring
          │
          ▼
   Ethernet Driver
          │
          ▼
 Linux Networking Stack
          │
          ▼
    Packet Filter Hook
          │
          ▼
     Packet Parsing
          │
          ▼
     Rule Matching
```

---

# 5. Ethernet PHY

The PHY converts the physical electrical signal into digital Ethernet data.

Conceptually:

```text
Network Cable
     │
     ▼
   PHY
     │
     ▼
Digital Ethernet Data
```

The PHY handles physical-layer functions such as:

* Link detection
* Auto-negotiation
* Speed
* Duplex
* Physical signaling

Check link status:

```bash
ethtool eth0
```

The actual interface name may differ.

---

# 6. Ethernet MAC

The MAC handles Ethernet frames.

A frame contains:

```text
+-------------------+
| Destination MAC   |
+-------------------+
| Source MAC        |
+-------------------+
| EtherType         |
+-------------------+
| Payload           |
+-------------------+
| FCS               |
+-------------------+
```

Example:

```text
Destination MAC
       │
       ▼
Source MAC
       │
       ▼
EtherType = IPv4
       │
       ▼
IP Packet
```

---

# 7. Ethernet Driver

The Linux Ethernet driver manages communication between the Ethernet controller and the Linux networking subsystem.

Typical responsibilities:

```text
Hardware Initialization
       │
       ├── TX/RX rings
       ├── DMA
       ├── Interrupts
       ├── MAC configuration
       └── PHY management
```

RX:

```text
Hardware
   │
   ▼
DMA
   │
   ▼
RX Ring
   │
   ▼
Driver
   │
   ▼
Linux Network Stack
```

---

# 8. DMA and RX Ring

For efficient packet reception, the Ethernet controller commonly uses DMA.

Conceptually:

```text
                 Ethernet Controller
                         │
                         │ DMA
                         ▼
                  +--------------+
                  | RX Ring      |
                  |              |
                  | Descriptor 0 |
                  | Descriptor 1 |
                  | Descriptor 2 |
                  | Descriptor 3 |
                  +------+-------+
                         │
                         ▼
                     Driver
```

The exact DMA architecture depends on the Ethernet controller and driver.

---

# 9. Linux Network Stack

After the Ethernet driver receives the packet, the packet enters the Linux networking stack.

Simplified:

```text
Ethernet Driver
      │
      ▼
    sk_buff
      │
      ▼
Linux Network Stack
      │
      ▼
Network Protocol Processing
```

Linux commonly represents packets using:

```c
struct sk_buff
```

The `sk_buff` contains packet data and metadata used by the network stack.

---

# 10. Packet Buffer

The packet is represented internally by an `skb`.

Conceptually:

```text
struct sk_buff
+----------------------+
| Packet Metadata      |
+----------------------+
| Ethernet Header      |
+----------------------+
| IP Header            |
+----------------------+
| TCP/UDP Header       |
+----------------------+
| Application Payload  |
+----------------------+
```

The packet filter should avoid unnecessary copying of packet data.

---

# 11. Packet Filter Hook

The filter is connected to the Linux networking path through the selected kernel networking mechanism.

Conceptually:

```text
             Linux Network Stack
                     │
                     ▼
              Filter Hook
                     │
                     ▼
             Packet Filter
```

Possible Linux mechanisms include:

```text
Netfilter
TC
XDP
eBPF
Network-driver integration
```

The project should use one clearly defined mechanism.

---

# 12. Packet Filter Entry

When a packet reaches the filtering point:

```text
Packet
  │
  ▼
Filter Entry
  │
  ▼
Check Filter Enabled
```

If filtering is disabled:

```text
Filtering Disabled
       │
       ▼
Continue Normal Processing
```

If filtering is enabled:

```text
Filtering Enabled
       │
       ▼
Packet Parsing
```

---

# 13. Packet Parsing

The driver extracts the fields required for matching.

Example:

```text
Ethernet Header
       │
       ▼
EtherType
       │
       ▼
IP Header
       │
       ├── Source IP
       ├── Destination IP
       └── Protocol
              │
              ▼
        TCP / UDP Header
              │
              ├── Source Port
              └── Destination Port
```

---

# 14. Ethernet Header Parsing

The Ethernet header provides:

```text
Destination MAC
Source MAC
EtherType
```

Example:

```text
EtherType
   │
   ├── IPv4
   ├── IPv6
   ├── ARP
   └── Other
```

The filter can decide which protocols it supports.

---

# 15. IPv4 Parsing

For IPv4 traffic:

```text
Ethernet
   │
   ▼
IPv4 Header
   │
   ├── Source IP
   ├── Destination IP
   ├── Protocol
   └── Header information
```

Example:

```text
Source      = 192.168.1.100
Destination = 192.168.1.20
Protocol    = TCP
```

---

# 16. TCP Parsing

For TCP:

```text
IPv4
 │
 ▼
TCP Header
 │
 ├── Source Port
 ├── Destination Port
 ├── Sequence Number
 ├── Flags
 └── ...
```

Example:

```text
Source IP      = 192.168.1.100
Destination IP = 192.168.1.20

Source Port      = 50000
Destination Port = 22

Protocol = TCP
```

---

# 17. UDP Parsing

For UDP:

```text
IPv4
 │
 ▼
UDP Header
 │
 ├── Source Port
 ├── Destination Port
 ├── Length
 └── Checksum
```

Example:

```text
Protocol         = UDP
Destination Port = 5000
```

---

# 18. ICMP Parsing

ICMP traffic does not use TCP/UDP ports.

Instead:

```text
IP
 │
 ▼
ICMP
 │
 ├── Type
 ├── Code
 └── Checksum
```

Example:

```text
ICMP Echo Request
```

The rule engine should treat ICMP separately from TCP/UDP.

---

# 19. Extracted Packet Metadata

The parser can create an internal metadata structure:

```c
struct pf_packet_info {
    __be32 src_ip;
    __be32 dst_ip;

    __be16 src_port;
    __be16 dst_port;

    u8 protocol;

    u16 ethertype;

    u32 packet_len;
};
```

Conceptually:

```text
Packet
  │
  ▼
Parser
  │
  ▼
pf_packet_info
  │
  ├── Source IP
  ├── Destination IP
  ├── Source Port
  ├── Destination Port
  ├── Protocol
  ├── EtherType
  └── Length
```

---

# 20. Rule Matching

After parsing:

```text
Packet Metadata
      │
      ▼
Rule Engine
      │
      ▼
Search Rule Table
```

Each rule may contain:

```text
Rule ID
Action
Protocol
Source IP
Source Mask
Destination IP
Destination Mask
Source Port
Destination Port
```

---

# 21. Rule Matching Algorithm

Basic algorithm:

```text
for each rule:

    compare protocol

    compare source IP

    compare destination IP

    compare source port

    compare destination port

    if all required fields match:
        rule matched
```

Conceptually:

```text
             Packet
                │
                ▼
          Rule 1 Match?
          │          │
         YES         NO
          │          │
          ▼          ▼
        Action     Rule 2
                     │
                     ▼
                  Rule 2 Match?
```

---

# 22. Whitelist Flow

Whitelist means explicitly allowed traffic.

Example:

```text
TCP
192.168.1.100
      →
192.168.1.20:22
```

Flow:

```text
Packet
  │
  ▼
Whitelist Search
  │
  ▼
Match?
  │
 YES
  │
  ▼
ALLOW
  │
  ▼
Continue Network Stack
```

---

# 23. Blacklist Flow

Blacklist means explicitly blocked traffic.

Example:

```text
TCP
Any Source
      →
Destination Port 23
```

Flow:

```text
Packet
  │
  ▼
Blacklist Search
  │
  ▼
Match?
  │
 YES
  │
  ▼
DROP
  │
  ▼
Packet Discarded
```

---

# 24. Monitoring Flow

Monitoring records or reports matching activity.

```text
Packet
  │
  ▼
Rule Matching
  │
  ▼
Monitoring Enabled?
  │
  YES
  │
  ▼
Generate Event
  │
  ├── Rule ID
  ├── Source IP
  ├── Destination IP
  ├── Protocol
  └── Action
```

Monitoring should not unnecessarily block the packet.

---

# 25. Rule Precedence

The project must define rule precedence.

Recommended model:

```text
             Packet
                │
                ▼
       Blacklist Match?
          │         │
         YES        NO
          │          │
          ▼          ▼
        DROP   Whitelist Match?
                    │       │
                   YES      NO
                    │        │
                    ▼        ▼
                  ALLOW   Default Policy
```

Therefore:

```text
Blacklist > Whitelist > Default Policy
```

If the project requires whitelist priority instead, that behavior must be documented and consistently implemented.

---

# 26. Default Policy

Packets that do not match any rule require a default action.

Possible policies:

```text
DEFAULT_ALLOW
DEFAULT_DROP
```

Example:

```text
No Rule Match
     │
     ▼
Default Policy
     │
 ┌───┴────┐
 ▼        ▼
ALLOW    DROP
```

For a security-oriented firewall, `DEFAULT_DROP` may be appropriate, but the project's actual security requirements should determine the choice.

---

# 27. Packet Decision

The complete decision logic:

```text
                    Packet
                      │
                      ▼
                Filter Enabled?
                 │          │
                NO         YES
                 │          │
                 ▼          ▼
               ALLOW     Parse Packet
                            │
                            ▼
                     Blacklist Match?
                       │          │
                      YES        NO
                       │          │
                       ▼          ▼
                     DROP    Whitelist Match?
                                  │       │
                                 YES      NO
                                  │        │
                                  ▼        ▼
                                ALLOW   Default Policy
```

---

# 28. Packet Drop

When the packet is rejected:

```text
Packet
  │
  ▼
Blacklist Match
  │
  ▼
DROP Decision
  │
  ├── dropped_packets++
  │
  ├── monitoring event
  │
  └── packet discarded
```

The packet does not continue to the destination application.

---

# 29. Packet Allow

When the packet is allowed:

```text
Packet
  │
  ▼
ALLOW Decision
  │
  ├── allowed_packets++
  │
  ├── monitoring event
  │
  ▼
Linux Network Stack
  │
  ▼
TCP/UDP/ICMP
  │
  ▼
Application
```

---

# 30. Statistics Flow

Statistics are updated based on packet decisions.

```text
                 Packet
                    │
                    ▼
                 Filter
                    │
          ┌─────────┼─────────┐
          ▼         ▼         ▼
        ALLOW      DROP      LOG
          │         │         │
          ▼         ▼         ▼
       allowed    dropped   logged
       counter    counter   counter
```

Example:

```c
stats->packets_received++;
```

For allowed packets:

```c
stats->packets_allowed++;
```

For dropped packets:

```c
stats->packets_dropped++;
```

---

# 31. Byte Statistics

The driver can also maintain byte counters:

```text
bytes_received
bytes_allowed
bytes_dropped
```

Example:

```text
Packet Length = 1500 bytes

Received:
    packets_received++
    bytes_received += 1500

Dropped:
    packets_dropped++
    bytes_dropped += 1500
```

---

# 32. Monitoring Event

A monitoring event could contain:

```c
struct pf_event {
    u32 rule_id;

    __be32 src_ip;
    __be32 dst_ip;

    __be16 src_port;
    __be16 dst_port;

    u8 protocol;
    u8 action;

    u32 packet_len;
};
```

Conceptual event:

```text
DROP
Rule ID: 12
SRC: 192.168.1.100
DST: 192.168.1.20
Protocol: TCP
DST Port: 23
Length: 512
```

---

# 33. TCP Connection Example

Consider an SSH connection.

```text
Client
192.168.1.100
      │
      │ TCP SYN
      ▼
BeagleBone AI-64
192.168.1.20:22
```

Packet filter sees:

```text
Protocol = TCP
Destination Port = 22
```

If whitelist contains:

```text
TCP → Port 22 → ALLOW
```

then:

```text
SYN
 │
 ▼
Filter
 │
 ▼
ALLOW
 │
 ▼
TCP Stack
 │
 ▼
SSH Service
```

---

# 34. Blocked TCP Example

Suppose:

```text
TCP Destination Port = 23
Action = DROP
```

Incoming packet:

```text
Client
   │
   │ TCP SYN :23
   ▼
BeagleBone
   │
   ▼
Packet Filter
   │
   ▼
Blacklist Match
   │
   ▼
DROP
```

The TCP service does not receive the packet.

---

# 35. UDP Example

Suppose:

```text
UDP Destination Port = 5000
Action = ALLOW
```

Flow:

```text
UDP Packet
    │
    ▼
Ethernet
    │
    ▼
IP
    │
    ▼
UDP
    │
    ▼
Packet Filter
    │
    ▼
Rule Match
    │
    ▼
ALLOW
    │
    ▼
Application Port 5000
```

---

# 36. Packet Capture Validation

Use `tcpdump` to validate traffic.

Example:

```bash
tcpdump -i eth0
```

For TCP:

```bash
tcpdump -i eth0 tcp
```

For UDP:

```bash
tcpdump -i eth0 udp
```

For a specific port:

```bash
tcpdump -i eth0 port 22
```

This helps compare:

```text
Actual Network Traffic
        │
        ▼
tcpdump Observation
        │
        ▼
Packet Filter Decision
        │
        ▼
Driver Statistics
```

---

# 37. RX Debugging

If an incoming packet is not reaching the application:

```text
Check PHY
   │
   ▼
Check Link
   │
   ▼
Check MAC
   │
   ▼
Check Ethernet Driver
   │
   ▼
Check RX Ring
   │
   ▼
Check Linux Network Stack
   │
   ▼
Check Packet Filter
   │
   ▼
Check TCP/UDP
   │
   ▼
Check Application
```

Useful commands:

```bash
ip link
```

```bash
ip addr
```

```bash
ethtool eth0
```

```bash
dmesg
```

```bash
tcpdump -i eth0
```

---

# 38. TX Packet Flow

Outgoing packets follow the reverse direction.

```text
Application
    │
    ▼
TCP / UDP
    │
    ▼
IP Layer
    │
    ▼
Packet Filter
    │
    ▼
Ethernet Driver
    │
    ▼
DMA / TX Ring
    │
    ▼
Ethernet MAC
    │
    ▼
PHY
    │
    ▼
Network
```

---

# 39. TX Filtering

If TX filtering is enabled:

```text
Application
     │
     ▼
Packet
     │
     ▼
TX Filter
     │
     ▼
Rule Match
     │
 ┌───┴────┐
 ▼        ▼
ALLOW    DROP
 │        │
 ▼        ▼
Driver   Discard
```

The project should explicitly document whether filtering applies to:

```text
RX only
TX only
RX + TX
```

---

# 40. Bidirectional Filtering

If both directions are filtered:

```text
                  BeagleBone AI-64
                       │
          ┌────────────┴────────────┐
          │                         │
          ▼                         ▼
         RX                        TX
          │                         │
          ▼                         ▼
     Packet Filter             Packet Filter
          │                         │
          ▼                         ▼
       Network                   Network
```

This provides firewall-style bidirectional policy enforcement.

---

# 41. Interface-Based Filtering

Rules may optionally include an interface.

Example:

```text
Interface = eth0
Protocol  = TCP
Port      = 22
Action    = ALLOW
```

Flow:

```text
Packet
  │
  ▼
Interface Check
  │
  ▼
eth0?
  │
  YES
  │
  ▼
Continue Rule Matching
```

---

# 42. Multi-Interface System

If the board has multiple interfaces:

```text
                Packet Filter
                     │
          ┌──────────┼──────────┐
          ▼          ▼          ▼
        eth0       eth1        wlan0
          │          │          │
          ▼          ▼          ▼
       Ethernet   Ethernet      Wi-Fi
```

Rules can be:

```text
Global
```

or:

```text
Per Interface
```

---

# 43. ARP Traffic

ARP is different from IPv4 TCP/UDP traffic.

```text
Ethernet
   │
   ▼
EtherType = ARP
   │
   ▼
ARP Packet
```

The packet filter must define whether ARP is:

```text
Allowed
Filtered
Ignored
Monitored
```

ARP filtering should not accidentally break normal network connectivity.

---

# 44. IPv6 Traffic

IPv6 follows a different packet structure.

```text
Ethernet
   │
   ▼
IPv6
   │
   ▼
Transport Protocol
   │
   ├── TCP
   ├── UDP
   └── ICMPv6
```

If IPv6 is not supported by the first implementation, the behavior should be explicitly documented.

Possible policies:

```text
IPv6 → bypass
IPv6 → allow
IPv6 → drop
IPv6 → fully filter
```

---

# 45. Fragmented Packets

IP fragmentation requires special consideration.

A fragmented packet may not contain transport-layer information in every fragment.

Example:

```text
Fragment 1
   │
   ├── IP Header
   └── TCP Header

Fragment 2
   │
   └── Payload only
```

Therefore, simple TCP/UDP port matching may not work for every fragment.

The implementation should define its fragmentation policy.

---

# 46. Malformed Packet Handling

Malformed packets should be rejected safely.

Flow:

```text
Packet
  │
  ▼
Parser
  │
  ▼
Invalid Header?
  │
 YES
  │
  ▼
DROP / REJECT
```

The parser must verify that enough packet data is available before accessing headers.

Never blindly access:

```c
struct iphdr *
struct tcphdr *
struct udphdr *
```

without validating packet boundaries.

---

# 47. Packet Boundary Validation

Conceptually:

```text
skb
 │
 ▼
Check Ethernet Header Length
 │
 ▼
Check IP Header Length
 │
 ▼
Check Transport Header Length
 │
 ▼
Access Fields
```

This protects against malformed packets and memory-access errors.

---

# 48. Packet Processing Context

Network packet processing can execute in contexts where sleeping is not allowed.

Therefore, code running on the packet-processing path must be carefully designed.

Avoid:

```text
Blocking operations
Sleeping locks
Large allocations
Long-running loops
Heavy logging
```

Use appropriate kernel synchronization and memory-allocation flags based on the actual execution context.

---

# 49. Fast Path vs Control Path

The driver has two different paths.

## Control Path

```text
User Application
      │
      ▼
ioctl()
      │
      ▼
Rule Management
      │
      ▼
Rule Database
```

## Data Path

```text
Network Packet
      │
      ▼
Packet Filter
      │
      ▼
Rule Lookup
      │
      ▼
ALLOW / DROP
```

The data path should be optimized because it can execute for every packet.

---

# 50. Configuration Flow

Configuration files:

```text
configs/rules/
├── whitelist.conf
├── blacklist.conf
└── monitoring.conf
```

Flow:

```text
Configuration File
       │
       ▼
User-Space Parser
       │
       ▼
Validation
       │
       ▼
IOCTL
       │
       ▼
Kernel Driver
       │
       ▼
Rule Database
```

---

# 51. Example Configuration

Whitelist:

```text
allow tcp 192.168.1.100 192.168.1.20 22
```

Blacklist:

```text
drop tcp any 192.168.1.20 23
```

Monitoring:

```text
monitor drop
```

The exact syntax must match the parser implementation.

---

# 52. Complete Configuration-to-Packet Flow

```text
whitelist.conf
      │
      ▼
User Parser
      │
      ▼
IOCTL
      │
      ▼
Kernel Rule Table
      │
      │
      │          Incoming Packet
      │                 │
      │                 ▼
      │            Packet Parser
      │                 │
      └──────────────► Rule Match
                        │
                        ▼
                     Decision
                        │
                 ┌──────┼──────┐
                 ▼      ▼      ▼
               ALLOW   DROP   LOG
```

---

# 53. Runtime Rule Update

Rules can be changed without rebooting.

Example:

```text
Running System
      │
      ▼
User adds rule
      │
      ▼
ioctl(PF_IOC_ADD_RULE)
      │
      ▼
Kernel Rule Table Updated
      │
      ▼
Next Packet Uses New Rule
```

This is one of the main benefits of the IOCTL control interface.

---

# 54. Rule Deletion During Runtime

```text
Existing Rule
     │
     ▼
ioctl(PF_IOC_DELETE_RULE)
     │
     ▼
Rule Removed
     │
     ▼
Next Packet
     │
     ▼
New Rule Evaluation
```

Synchronization is required if packet processing can access the rule simultaneously.

---

# 55. Statistics Retrieval

User space can periodically request statistics:

```text
Daemon
  │
  │ every N seconds
  ▼
PF_IOC_GET_STATS
  │
  ▼
Kernel Driver
  │
  ▼
Statistics
  │
  ▼
User Space
```

Example:

```text
Packets Received : 100000
Packets Allowed  : 98500
Packets Dropped  : 1500
```

---

# 56. Complete Runtime Flow

```text
                 SYSTEM START
                      │
                      ▼
               Linux Kernel Boot
                      │
                      ▼
             Packet Filter Driver
                      │
                      ▼
                Driver Ready
                      │
                      ▼
             User-Space Daemon
                      │
                      ▼
            Load Configuration
                      │
                      ▼
                  IOCTL
                      │
                      ▼
               Rule Database
                      │
                      ▼
                SYSTEM RUNNING
                      │
                      ▼
                Network Packet
                      │
                      ▼
                Ethernet Driver
                      │
                      ▼
                Network Stack
                      │
                      ▼
                Filter Hook
                      │
                      ▼
                Packet Parser
                      │
                      ▼
                Rule Matching
                      │
             ┌────────┼────────┐
             ▼        ▼        ▼
           ALLOW     DROP     LOG
             │        │        │
             ▼        ▼        ▼
          Continue  Discard  Monitor
```

---

# 57. Packet Flow Debugging

When a packet is unexpectedly dropped:

```text
1. Confirm packet was transmitted.
2. Confirm Ethernet link.
3. Capture traffic using tcpdump.
4. Confirm packet reaches Linux.
5. Confirm filter is enabled.
6. Check parsed packet metadata.
7. Check whitelist.
8. Check blacklist.
9. Check rule precedence.
10. Check default policy.
11. Check statistics.
12. Check monitoring logs.
```

---

# 58. Debug Logging

Useful debug points:

```c
pr_debug("packet received\n");

pr_debug("src=%pI4 dst=%pI4\n",
         &src_ip, &dst_ip);

pr_debug("protocol=%u\n", protocol);

pr_debug("rule=%u matched\n", rule_id);

pr_debug("packet allowed\n");

pr_debug("packet dropped\n");
```

Avoid enabling excessive logging in production because packet-level `printk()` can severely affect performance.

---

# 59. Packet Flow Test Matrix

| Test          | Packet             | Expected Result  |
| ------------- | ------------------ | ---------------- |
| Whitelist TCP | TCP/22             | ALLOW            |
| Blacklist TCP | TCP/23             | DROP             |
| Whitelist UDP | UDP/5000           | ALLOW            |
| Blacklist UDP | UDP/6000           | DROP             |
| Unknown TCP   | No rule            | Default policy   |
| Unknown UDP   | No rule            | Default policy   |
| ICMP          | Echo request       | Policy dependent |
| ARP           | ARP request        | Policy dependent |
| IPv6          | IPv6 packet        | Policy dependent |
| Monitoring    | Any monitored rule | Event generated  |

---

# 60. Performance Test

Measure:

```text
Packets per second
Latency
CPU utilization
Dropped packets
Allowed packets
Rule lookup time
```

Test with:

```text
1 rule
10 rules
100 rules
1000 rules
```

Conceptually:

```text
Traffic Rate
     │
     ▼
Packet Generator
     │
     ▼
BeagleBone AI-64
     │
     ▼
Packet Filter
     │
     ▼
Measure:
 ├── PPS
 ├── CPU
 ├── Drops
 └── Latency
```

---

# 61. Stress Testing

Stress conditions should include:

```text
High packet rate
Large packets
Small packets
Mixed TCP/UDP
Many simultaneous connections
Large rule table
Frequent rule updates
Monitoring enabled
Monitoring disabled
```

The driver must remain stable under sustained traffic.

---

# 62. Failure Handling

If the packet parser encounters an invalid packet:

```text
Invalid Packet
      │
      ▼
Reject Safely
      │
      ▼
Increment Error/Drop Counter
      │
      ▼
Optional Monitoring Event
```

The driver must never crash because of malformed network traffic.

---

# 63. Security Boundary

The complete security boundary is:

```text
External Network
       │
       ▼
   Untrusted Packet
       │
       ▼
Packet Parser
       │
       ▼
Validation
       │
       ▼
Rule Engine
       │
       ▼
ALLOW / DROP
       │
       ▼
Protected System
```

The packet itself must always be treated as untrusted input.

---

# 64. End-to-End Example

Consider:

```text
Source:
192.168.1.100

Destination:
192.168.1.20

Protocol:
TCP

Destination Port:
23
```

Blacklist:

```text
TCP port 23 → DROP
```

Complete flow:

```text
Network
   │
   ▼
PHY
   │
   ▼
MAC
   │
   ▼
Ethernet Driver
   │
   ▼
Linux Network Stack
   │
   ▼
Packet Filter
   │
   ▼
Parse:
SRC = 192.168.1.100
DST = 192.168.1.20
PROTO = TCP
DPORT = 23
   │
   ▼
Blacklist Lookup
   │
   ▼
MATCH
   │
   ▼
DROP
   │
   ├── packets_dropped++
   │
   ├── bytes_dropped += length
   │
   └── Monitoring Event
```

The packet never reaches the target application.

---

# 65. End-to-End Allowed Example

Rule:

```text
TCP port 22 → ALLOW
```

Packet:

```text
SRC = 192.168.1.100
DST = 192.168.1.20
PROTO = TCP
DPORT = 22
```

Flow:

```text
Network
   │
   ▼
Ethernet
   │
   ▼
Linux Network Stack
   │
   ▼
Packet Filter
   │
   ▼
Parse Packet
   │
   ▼
Whitelist Match
   │
   ▼
ALLOW
   │
   ▼
TCP Stack
   │
   ▼
SSH Application
```

---

# 66. Important Distinction

The packet-filter driver has two fundamentally different flows:

```text
                 PACKET DATA PATH
                       │
                       ▼
                  Network Packet
                       │
                       ▼
                 Rule Evaluation
                       │
                 ┌─────┴─────┐
                 ▼           ▼
               ALLOW        DROP
```

and:

```text
                CONTROL PATH
                       │
                       ▼
                Configuration
                       │
                       ▼
                  User Space
                       │
                       ▼
                    IOCTL
                       │
                       ▼
                  Kernel Rules
```

They interact through the rule database:

```text
Control Path
     │
     ▼
Rule Database
     ▲
     │
Data Path
```

---

# 67. Final Packet Flow

The complete packet path for the BeagleBone AI-64 packet-filter project is:

```text
                        EXTERNAL NETWORK
                               │
                               ▼
                        Ethernet PHY
                               │
                               ▼
                        Ethernet MAC
                               │
                               ▼
                         DMA / RX Ring
                               │
                               ▼
                       Ethernet Driver
                               │
                               ▼
                       Linux Network Stack
                               │
                               ▼
                        Packet Filter Hook
                               │
                               ▼
                         Packet Validation
                               │
                               ▼
                         Packet Parsing
                               │
                ┌──────────────┼──────────────┐
                ▼              ▼              ▼
            Source IP     Destination IP   Protocol
                               │
                               ▼
                          Port Parsing
                               │
                               ▼
                          Rule Lookup
                               │
                 ┌─────────────┼─────────────┐
                 ▼             ▼             ▼
             BLACKLIST      WHITELIST     MONITOR
                 │             │             │
                 ▼             ▼             ▼
               DROP          ALLOW        EVENT
                 │             │
                 │             ▼
                 │       Linux Protocol Stack
                 │             │
                 │             ▼
                 │        TCP / UDP / ICMP
                 │             │
                 │             ▼
                 │         Application
                 │
                 ▼
              DISCARD
```

---

# 68. Summary

The packet flow can be remembered as:

```text
PHY
 ↓
MAC
 ↓
Ethernet Driver
 ↓
Linux Network Stack
 ↓
Filter Hook
 ↓
Parse
 ↓
Match
 ↓
Decision
 ├── ALLOW → Continue
 ├── DROP  → Discard
 └── LOG   → Monitor
```

The complete project relationship is:

```text
configs/rules/
      │
      ▼
User-Space Configuration
      │
      ▼
IOCTL
      │
      ▼
Kernel Driver
      │
      ▼
Rule Database
      │
      ▼
Packet Filter
      │
      ▼
Network Packet
      │
      ▼
ALLOW / DROP
      │
      ▼
Statistics + Monitoring
```

This packet-flow model should be used together with:

```text
docs/kernel-driver.md
docs/ioctl-api.md
docs/packet-filter.md
docs/networking.md
docs/device-tree.md
docs/debugging.md
```

to understand the complete packet-filter implementation from configuration and kernel control down to the actual Ethernet packet path.

```
```

