# `docs/testing.md`

````markdown
# Testing – BeagleBone AI-64 / TI TDA4VM Packet Filter

## 1. Overview

This document defines the complete testing strategy for the packet-filtering
system running on the BeagleBone AI-64 based on the TI TDA4VM SoC.

The testing process validates:

- Linux kernel configuration
- Device Tree configuration
- Kernel driver
- Packet-filter logic
- Whitelist rules
- Blacklist rules
- Monitoring rules
- IOCTL interface
- Ethernet networking
- Packet forwarding
- Performance
- Stress behavior
- Error handling
- System stability

The overall testing flow is:

```text
Source Code
    │
    ▼
Build
    │
    ▼
Deploy
    │
    ▼
Boot BeagleBone AI-64
    │
    ▼
Verify Kernel / Driver
    │
    ▼
Verify Network
    │
    ▼
Load Packet Filter
    │
    ▼
Configure Rules
    │
    ▼
Generate Traffic
    │
    ▼
Verify Packet Decision
    │
    ▼
Collect Logs / Statistics
    │
    ▼
Performance / Stress Test
    │
    ▼
PASS / FAIL
````

---

# 2. Testing Levels

Testing is divided into multiple levels:

```text
                    Testing
                       │
       ┌───────────────┼────────────────┐
       │               │                │
       ▼               ▼                ▼
    Unit Test     Integration Test   System Test
       │               │                │
       └───────────────┼────────────────┘
                       ▼
                 Performance Test
                       │
                       ▼
                  Stress Test
                       │
                       ▼
                 Regression Test
```

---

# 3. Test Categories

| Test Category      | Purpose                                            |
| ------------------ | -------------------------------------------------- |
| Build Test         | Verify source compilation                          |
| Boot Test          | Verify board boots correctly                       |
| Driver Test        | Verify kernel driver loads                         |
| Device Tree Test   | Verify hardware configuration                      |
| IOCTL Test         | Verify user/kernel communication                   |
| Rule Test          | Verify rule configuration                          |
| Packet Test        | Verify packet filtering                            |
| Networking Test    | Verify Ethernet connectivity                       |
| Monitoring Test    | Verify monitoring functionality                    |
| Performance Test   | Measure PPS, throughput and CPU                    |
| Stress Test        | Verify behavior under heavy traffic                |
| Negative Test      | Verify invalid inputs are rejected                 |
| Regression Test    | Ensure changes do not break existing functionality |
| Long-Duration Test | Verify stability over time                         |

---

# 4. Test Environment

The test environment should contain:

```text
Host PC
  │
  ├── Linux
  ├── Yocto Build Environment
  ├── Git
  ├── Serial Terminal
  ├── Cross Compiler
  └── Traffic Generator
          │
          ▼
   BeagleBone AI-64
          │
          ├── TDA4VM
          ├── Linux Kernel
          ├── Packet Filter Driver
          ├── RootFS
          └── Ethernet
```

---

# 5. Hardware Test Setup

Recommended setup:

```text
                  Ethernet
 PC / Generator ───────────────► BeagleBone AI-64
                                      │
                                      ▼
                                Packet Filter
                                      │
                                      ▼
                                  Network
```

For forwarding tests:

```text
Traffic Generator
       │
       ▼
   Ethernet 1
       │
       ▼
BeagleBone AI-64
       │
   Packet Filter
       │
       ▼
   Ethernet 2
       │
       ▼
Traffic Receiver
```

The exact interface configuration depends on the board and network design.

---

# 6. Serial Console Test

Connect the board's serial console.

Verify that boot messages are visible:

```text
ROM
 ↓
TIFS
 ↓
SPL
 ↓
U-Boot
 ↓
Linux Kernel
 ↓
RootFS
 ↓
Login Prompt
```

Expected result:

```text
PASS – Board boots successfully
```

---

# 7. Kernel Verification

Check the running kernel:

```bash
uname -a
```

or:

```bash
uname -r
```

Verify:

```text
Kernel version
Architecture
Build configuration
Expected kernel image
```

Expected:

```text
PASS – Expected kernel is running
```

---

# 8. Device Tree Verification

Verify the Device Tree used by the kernel.

Check:

```bash
ls /proc/device-tree/
```

or:

```bash
find /proc/device-tree/ -maxdepth 2 -type d
```

Check required hardware nodes:

```text
Ethernet
PHY
GPIO
Interrupts
Clocks
Regulators
```

Expected:

```text
PASS – Required Device Tree nodes are present
```

---

# 9. Kernel Log Test

Check kernel messages:

```bash
dmesg
```

Search for driver messages:

```bash
dmesg | grep -i packet
```

Search Ethernet:

```bash
dmesg | grep -i ethernet
```

Search errors:

```bash
dmesg | grep -i error
```

Search warnings:

```bash
dmesg | grep -i warning
```

Expected:

```text
No unexpected driver or hardware errors
```

---

# 10. Driver Loading Test

If the packet filter is a loadable kernel module:

```bash
modprobe packet_filter
```

Verify:

```bash
lsmod | grep packet
```

Check kernel log:

```bash
dmesg | tail -50
```

Expected:

```text
packet_filter: module loaded
```

---

# 11. Driver Unload Test

Unload the module:

```bash
rmmod packet_filter
```

Verify:

```bash
lsmod | grep packet
```

Expected:

```text
Module removed successfully
```

Check for:

```text
Kernel crash
Use-after-free
Resource leak
Stale device node
```

---

# 12. Device Node Test

If the driver exposes a character device:

```bash
ls -l /dev/
```

Example:

```text
/dev/packet_filter
```

Verify:

```bash
test -e /dev/packet_filter && echo PASS
```

Expected:

```text
PASS – Device node exists
```

---

# 13. Driver Initialization Test

Test the following sequence:

```text
insmod
   │
   ▼
Driver initialization
   │
   ├── Allocate resources
   ├── Register device
   ├── Initialize rule database
   └── Register packet hook
```

Verify each stage using kernel logs.

---

# 14. IOCTL Test

The user-space application communicates with the kernel driver through IOCTLs.

Flow:

```text
User Application
       │
       ▼
open()
       │
       ▼
ioctl()
       │
       ▼
Kernel Driver
       │
       ▼
Packet Filter
```

Test:

```text
ADD RULE
DELETE RULE
GET RULE
CLEAR RULES
GET STATISTICS
ENABLE FILTER
DISABLE FILTER
```

Expected:

```text
PASS – All supported IOCTL operations behave correctly
```

---

# 15. IOCTL Invalid Input Test

Test invalid parameters:

```text
NULL pointer
Invalid rule ID
Invalid protocol
Invalid IP address
Invalid port
Invalid command
Oversized structure
Invalid flags
```

Expected behavior:

```text
Kernel must reject invalid input safely.
```

Verify:

```text
No kernel crash
No memory corruption
No unexpected rule creation
```

---

# 16. Rule Database Test

Start with an empty rule database:

```text
Rules = 0
```

Verify:

```text
GET_RULE_COUNT
```

Expected:

```text
0
```

Then add rules:

```text
Rule 1
Rule 2
Rule 3
```

Verify:

```text
Rule count = 3
```

---

# 17. Whitelist Test

Whitelist rules should allow explicitly permitted traffic.

Example:

```text
Whitelist:
TCP
Destination Port: 443
```

Traffic:

```text
TCP → 443
```

Expected:

```text
ALLOW
```

Traffic:

```text
TCP → 22
```

Expected behavior depends on the configured default policy.

The test must verify the documented policy exactly.

---

# 18. Blacklist Test

Blacklist rules should reject explicitly blocked traffic.

Example:

```text
Blacklist:
Source IP = 192.168.1.100
```

Traffic:

```text
192.168.1.100 → Board
```

Expected:

```text
DROP
```

Traffic from an allowed source should follow the configured default policy.

---

# 19. Monitoring Rule Test

Monitoring rules should generate monitoring information without incorrectly
changing the packet decision.

Flow:

```text
Packet
  │
  ▼
Rule Match
  │
  ├── Decision
  │
  └── Monitoring Event
```

Verify:

```text
Packet decision
Monitoring counter
Monitoring event
Timestamp / metadata if supported
```

---

# 20. Rule Priority Test

If multiple rules match the same packet, verify the documented priority.

Example:

```text
Rule 1: ALLOW TCP 443
Rule 2: DROP TCP 443
```

The expected result depends on the project's rule-priority definition.

Test:

```text
Same packet
Different rule ordering
```

Verify that the implementation follows the documented ordering.

---

# 21. Default Policy Test

Test the system with no matching rule.

Possible policies:

```text
DEFAULT ALLOW
```

or:

```text
DEFAULT DROP
```

Test both only if both are supported.

Expected:

```text
No rule match
      │
      ▼
Default policy
      │
      ▼
ALLOW / DROP
```

---

# 22. Rule Deletion Test

Add:

```text
Rule 1
Rule 2
Rule 3
```

Delete:

```text
Rule 2
```

Verify:

```text
Rule 1 exists
Rule 2 does not exist
Rule 3 exists
```

Then test traffic matching Rule 2.

Expected:

```text
Packet follows the remaining/default policy.
```

---

# 23. Clear-All-Rules Test

Add multiple rules:

```text
Rule 1
Rule 2
Rule 3
Rule 4
```

Execute:

```text
CLEAR_RULES
```

Verify:

```text
Rule count = 0
```

Then test packet behavior.

---

# 24. Ethernet Connectivity Test

Check interfaces:

```bash
ip link
```

Check IP addresses:

```bash
ip addr
```

Bring interface up:

```bash
ip link set <interface> up
```

Configure IP if required:

```bash
ip addr add <ip>/<mask> dev <interface>
```

Test:

```bash
ping <peer-ip>
```

Expected:

```text
PASS – Ethernet connectivity established
```

---

# 25. TCP Test

Use a TCP service on the receiver.

Example:

```bash
iperf3 -s
```

Client:

```bash
iperf3 -c <server-ip>
```

Verify:

```text
TCP connection
Packet filtering
Allow/drop behavior
Throughput
```

---

# 26. UDP Test

Start UDP traffic:

```bash
iperf3 -s
```

Client:

```bash
iperf3 -c <server-ip> -u -b 100M
```

Verify:

```text
UDP packets
Packet filtering
Packet drops
Packet counters
```

---

# 27. ICMP Test

Use:

```bash
ping <peer-ip>
```

Test:

```text
ICMP allowed
ICMP blocked
```

Verify the packet-filter counters.

---

# 28. Port Filtering Test

Test common ports:

```text
22
53
80
443
5000
8080
```

Example:

```text
ALLOW TCP 443
DROP TCP 22
```

Verify:

```text
HTTPS traffic → ALLOW
SSH traffic   → DROP
```

---

# 29. Protocol Filtering Test

Test:

```text
TCP
UDP
ICMP
```

Example:

```text
TCP → ALLOW
UDP → DROP
ICMP → ALLOW
```

Generate each type of traffic and verify the result.

---

# 30. Source IP Filtering Test

Example:

```text
Source IP = 192.168.1.10
```

Generate traffic from:

```text
192.168.1.10
192.168.1.20
```

Expected:

```text
192.168.1.10 → Rule action
192.168.1.20 → Default policy
```

---

# 31. Destination IP Filtering Test

Create:

```text
Destination IP = 192.168.1.50
```

Generate:

```text
Traffic → 192.168.1.50
Traffic → 192.168.1.60
```

Verify that only the intended destination matches.

---

# 32. Source Port Test

Example:

```text
Source Port = 5000
Action = DROP
```

Generate traffic from:

```text
5000
5001
```

Expected:

```text
5000 → DROP
5001 → Default policy
```

---

# 33. Destination Port Test

Example:

```text
Destination Port = 443
Action = ALLOW
```

Generate:

```text
TCP 443
TCP 80
```

Verify the expected rule behavior.

---

# 34. Packet Counter Test

After generating traffic:

```text
GET_STATISTICS
```

Verify:

```text
Packets received
Packets allowed
Packets dropped
Rules matched
Monitoring events
```

Example:

```text
Before traffic:
RX = 0

Generate 1000 packets

After:
RX = 1000
```

The exact counter semantics must match the implementation.

---

# 35. Counter Accuracy Test

Generate a known number of packets.

Example:

```text
Generated = 10,000
```

Read counters:

```text
RX        = measured
Allowed   = measured
Dropped   = measured
```

Verify the relationship:

```text
RX ≈ Allowed + Dropped
```

Any intentional exceptions, such as packets counted at different stages,
must be documented.

---

# 36. Packet Capture Test

Use:

```bash
tcpdump -i <interface>
```

Example:

```bash
tcpdump -i eth0
```

Capture packets:

```bash
tcpdump -i eth0 -nn
```

Save to file:

```bash
tcpdump -i eth0 -w capture.pcap
```

Use the capture to verify packet behavior independently of driver counters.

---

# 37. Wireshark Analysis

Copy the capture to the host and inspect it with Wireshark.

Check:

```text
Ethernet
IP
TCP
UDP
ICMP
Source
Destination
Port
Flags
```

Use packet captures to confirm whether a packet was:

```text
Received
Transmitted
Dropped
```

---

# 38. Packet Drop Verification

To verify DROP behavior:

```text
Traffic Generator
       │
       ▼
BeagleBone AI-64
       │
       ▼
Packet Filter
       │
       ├── DROP
       │
       ▼
Receiver
```

Expected:

```text
Receiver does not receive blocked traffic.
```

Also verify the driver's drop counter.

---

# 39. Packet Allow Verification

For ALLOW:

```text
Traffic Generator
       │
       ▼
Packet Filter
       │
       ├── ALLOW
       │
       ▼
Receiver
```

Expected:

```text
Receiver receives the expected traffic.
```

---

# 40. Negative Testing

Negative tests verify that invalid operations fail safely.

Test:

```text
Invalid IOCTL
Invalid rule
Invalid IP
Invalid port
Invalid protocol
Duplicate rule
Non-existing rule deletion
Oversized input
NULL input
Invalid device access
```

Expected:

```text
Operation rejected
No crash
No memory corruption
No invalid state
```

---

# 41. Duplicate Rule Test

Add:

```text
Rule A
```

Add the same rule again.

The implementation should have a documented policy:

```text
Reject duplicate
```

or:

```text
Allow duplicate
```

Verify the actual behavior against the specification.

---

# 42. Non-Existing Rule Test

Attempt:

```text
DELETE Rule 9999
```

when Rule 9999 does not exist.

Expected:

```text
Clean error
No kernel crash
No unrelated rule deletion
```

---

# 43. Maximum Rule Test

Determine the configured maximum rule count.

Example:

```text
MAX_RULES = <configured value>
```

Add rules until the limit is reached.

Expected:

```text
Rule MAX-1 → SUCCESS
Rule MAX   → SUCCESS
Rule MAX+1 → REJECT
```

The exact boundary depends on the implementation.

---

# 44. Kernel Crash Test

During testing monitor:

```bash
dmesg -w
```

Look for:

```text
Oops
BUG
WARNING
Call Trace
general protection fault
NULL pointer
use-after-free
```

Any unexpected kernel fault is a test failure.

---

# 45. Driver Reload Test

Repeat:

```text
modprobe packet_filter
rmmod packet_filter
```

multiple times.

Example:

```text
Load
Unload
Load
Unload
...
```

Verify:

```text
No memory leak
No stale device
No duplicate registration
No crash
```

---

# 46. Network Interface Restart Test

Test:

```bash
ip link set <interface> down
ip link set <interface> up
```

Verify:

```text
Network recovers
Packet filter remains functional
Rules remain valid or are restored according to design
Counters behave as documented
```

---

# 47. Reboot Test

Configure rules and generate traffic.

Then:

```bash
reboot
```

After reboot verify:

```text
Kernel
Driver
Network
Rules
Monitoring
Statistics
```

Determine whether rules are:

```text
Persistent
```

or:

```text
Cleared after reboot
```

according to the project design.

---

# 48. Performance Test

Performance testing should cover:

```text
PPS
Throughput
CPU usage
Memory
Latency
Rule lookup
Packet drops
```

Test packet sizes:

```text
64 B
128 B
256 B
512 B
1024 B
1500 B
```

Test rule counts:

```text
1
10
100
500
1000
5000
```

Refer to:

```text
docs/performance.md
```

for the detailed performance methodology.

---

# 49. Stress Test

Stress the system with:

```text
High traffic rate
Large rule database
Monitoring enabled
Frequent rule updates
Long duration
```

Monitor:

```bash
top
```

```bash
dmesg -w
```

```bash
cat /proc/interrupts
```

```bash
cat /proc/softirqs
```

Expected:

```text
No crash
No memory leak
No deadlock
No uncontrolled packet loss
```

---

# 50. Long-Duration Test

Run continuous traffic for:

```text
10 minutes
30 minutes
1 hour
4 hours
24 hours
```

Monitor:

```text
CPU
Memory
Temperature
Packet counters
Dropped packets
Kernel logs
Network state
```

Compare initial and final values.

---

# 51. Memory Leak Test

Perform repeated:

```text
Load driver
Create rules
Delete rules
Clear rules
Unload driver
```

Check memory before and after.

The expected result is:

```text
Memory usage returns close to baseline
```

after resources are released.

Kernel memory-debugging facilities may be enabled when deeper leak analysis
is required.

---

# 52. Concurrency Test

Run simultaneously:

```text
Packet traffic
+
Rule updates
+
Statistics queries
+
Monitoring
```

Example:

```text
Thread 1 → Generate traffic
Thread 2 → Add/delete rules
Thread 3 → Read statistics
Thread 4 → Monitoring
```

Verify:

```text
No race condition
No deadlock
No corrupted rule table
No kernel crash
```

---

# 53. Race Condition Test

Focus on:

```text
Add rule
Delete rule
Lookup rule
Clear rules
```

at the same time.

Example:

```text
CPU0 → Packet lookup
CPU1 → Delete rule
CPU2 → Add rule
CPU3 → Read statistics
```

The rule database must remain consistent.

---

# 54. Regression Testing

Every driver or kernel change should execute:

```text
Build
 ↓
Boot
 ↓
Driver load
 ↓
Network test
 ↓
Rule test
 ↓
Packet test
 ↓
IOCTL test
 ↓
Performance smoke test
```

Regression testing prevents previously working functionality from breaking.

---

# 55. Automated Test Flow

A test script can automate:

```text
1. Check board connectivity
2. Check kernel
3. Check driver
4. Check network
5. Clear rules
6. Add rules
7. Generate traffic
8. Read counters
9. Validate expected result
10. Remove rules
11. Collect logs
12. Generate PASS/FAIL report
```

Example structure:

```text
tests/
├── unit/
├── integration/
├── networking/
├── packet/
├── ioctl/
├── performance/
├── stress/
└── regression/
```

---

# 56. Test Script Example

Example test execution:

```bash
./tests/run_tests.sh
```

Possible flow:

```text
========================================
BeagleBone AI-64 Packet Filter Tests
========================================

[1] Kernel Test             PASS
[2] Driver Test             PASS
[3] Device Node Test        PASS
[4] Ethernet Test           PASS
[5] IOCTL Test              PASS
[6] Whitelist Test          PASS
[7] Blacklist Test          PASS
[8] Monitoring Test         PASS
[9] Packet Test             PASS
[10] Statistics Test        PASS
[11] Stress Test            PASS

========================================
RESULT: PASS
========================================
```

---

# 57. Test Result Format

Each test should record:

```text
Test ID:
Test Name:
Purpose:
Environment:
Preconditions:
Steps:
Expected Result:
Actual Result:
Status:
Logs:
Git Commit:
Date:
```

Example:

```text
Test ID      : PF-NET-001
Test Name    : TCP 443 Whitelist
Purpose      : Verify HTTPS traffic is allowed

Precondition:
Packet filter loaded
TCP 443 rule configured

Expected:
TCP 443 traffic is allowed

Actual:
Measured result

Status:
PASS
```

---

# 58. Test Case ID Convention

Use consistent IDs.

```text
PF-BLD-xxx    Build tests
PF-BOOT-xxx   Boot tests
PF-DRV-xxx    Driver tests
PF-DT-xxx     Device Tree tests
PF-IOCTL-xxx  IOCTL tests
PF-RULE-xxx   Rule tests
PF-NET-xxx    Network tests
PF-PKT-xxx    Packet tests
PF-MON-xxx    Monitoring tests
PF-PERF-xxx   Performance tests
PF-STR-xxx    Stress tests
PF-REG-xxx    Regression tests
```

---

# 59. Test Matrix

| ID           | Test           | Expected Result                |
| ------------ | -------------- | ------------------------------ |
| PF-BLD-001   | Kernel build   | Build succeeds                 |
| PF-BOOT-001  | Board boot     | Linux boots                    |
| PF-DRV-001   | Driver load    | Driver loads                   |
| PF-DRV-002   | Driver unload  | Driver unloads                 |
| PF-DT-001    | Ethernet DT    | Ethernet initialized           |
| PF-IOCTL-001 | Add rule       | Rule added                     |
| PF-IOCTL-002 | Delete rule    | Rule deleted                   |
| PF-IOCTL-003 | Get statistics | Correct statistics             |
| PF-RULE-001  | Whitelist      | Expected traffic allowed       |
| PF-RULE-002  | Blacklist      | Expected traffic dropped       |
| PF-RULE-003  | Default policy | Correct default action         |
| PF-NET-001   | Ping           | Connectivity works             |
| PF-NET-002   | TCP            | TCP behavior correct           |
| PF-NET-003   | UDP            | UDP behavior correct           |
| PF-PKT-001   | Packet allow   | Packet reaches receiver        |
| PF-PKT-002   | Packet drop    | Packet does not reach receiver |
| PF-MON-001   | Monitoring     | Event/counter generated        |
| PF-PERF-001  | PPS            | Target achieved                |
| PF-STR-001   | Stress         | System remains stable          |
| PF-REG-001   | Regression     | Existing tests pass            |

---

# 60. Full Testing Flow

```text
                    START
                      │
                      ▼
                Build Validation
                      │
                      ▼
                Flash / Deploy
                      │
                      ▼
                 Board Boot
                      │
                      ▼
              Kernel Validation
                      │
                      ▼
             Device Tree Validation
                      │
                      ▼
               Driver Validation
                      │
                      ▼
              Device Node Check
                      │
                      ▼
               Network Validation
                      │
                      ▼
                IOCTL Testing
                      │
                      ▼
                 Rule Testing
                      │
                      ▼
               Packet Testing
                      │
                      ▼
             Monitoring Testing
                      │
                      ▼
             Performance Testing
                      │
                      ▼
                Stress Testing
                      │
                      ▼
             Long-Duration Testing
                      │
                      ▼
              Regression Testing
                      │
                      ▼
                 Test Report
                      │
                      ▼
                 PASS / FAIL
```

---

# 61. Pre-Test Checklist

```text
[ ] Correct board connected
[ ] Serial console working
[ ] Correct boot image installed
[ ] Correct kernel running
[ ] Correct Device Tree loaded
[ ] Driver built
[ ] Driver loaded
[ ] Device node available
[ ] Ethernet interface available
[ ] IP configuration completed
[ ] Traffic generator available
[ ] Test rules prepared
[ ] Debug logs enabled only where required
[ ] Test scripts available
```

---

# 62. Post-Test Checklist

```text
[ ] Test results collected
[ ] dmesg collected
[ ] Packet captures collected
[ ] Driver logs collected
[ ] Performance results collected
[ ] CPU usage recorded
[ ] Memory usage recorded
[ ] Packet counters recorded
[ ] Errors analyzed
[ ] Test report generated
[ ] Failed tests documented
[ ] Regression status verified
```

---

# 63. PASS / FAIL Criteria

A test is considered **PASS** when:

```text
Actual Result == Expected Result
```

and there are no unexpected:

```text
Kernel crashes
Memory corruption
Deadlocks
Resource leaks
Network failures
Unexpected packet decisions
```

A test is **FAIL** when:

```text
Expected behavior is not observed
```

or the system experiences an unexpected critical error.

---

# 64. Final Test Validation

Before releasing the packet-filter software, verify:

```text
Build
  ✓

Boot
  ✓

Kernel
  ✓

Device Tree
  ✓

Driver
  ✓

IOCTL
  ✓

Rules
  ✓

Ethernet
  ✓

Packet filtering
  ✓

Monitoring
  ✓

Performance
  ✓

Stress
  ✓

Long duration
  ✓

Regression
  ✓
```

---

# 65. Final Testing Principle

The testing strategy follows:

```text
Build
  ↓
Boot
  ↓
Initialize
  ↓
Configure
  ↓
Generate
  ↓
Observe
  ↓
Measure
  ↓
Stress
  ↓
Repeat
```

The most important validation is not only whether the packet filter can
ALLOW or DROP a packet, but whether it continues to make the **correct
decision safely, efficiently, and consistently under real network traffic,
large rule sets, concurrent configuration, and long-duration operation.**

```
```

