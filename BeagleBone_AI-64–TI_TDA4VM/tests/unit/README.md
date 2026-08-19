Compile

From the project root:

cd BeagleBone_AI-64–TI_TDA4VM


gcc -Wall -Wextra -O2 \
    -I./kernel/packet_filter \
    -o tests/unit/test_packet_parser \
    tests/unit/test_packet_parser.c \
    kernel/packet_filter/packet_parser.c

Then:

./tests/unit/test_packet_parser

Expected structure:

BeagleBone_AI-64–TI_TDA4VM/
├── kernel/
│   └── packet_filter/
│       ├── packet_parser.h
│       └── packet_parser.c
│
└── tests/
    └── unit/
        └── test_packet_parser.c

Important: because your packet_parser.h/.c were created earlier, the exact parser API may differ from the assumptions in this test file. The test file above is intentionally self-contained for constructing Ethernet/IPv4/TCP/UDP/ICMP test frames, but when we wire it to your actual packet_parser.c, we should align the parser function names and output structures exactly.


Important

This test expects your rule_engine.h/.c to expose APIs approximately like:

int rule_engine_init(void);


int rule_engine_add_rule(
    uint32_t src_ip,
    uint32_t dst_ip,
    uint16_t src_port,
    uint16_t dst_port,
    uint8_t protocol,
    int action);


int rule_engine_process(
    uint32_t src_ip,
    uint32_t dst_ip,
    uint16_t src_port,
    uint16_t dst_port,
    uint8_t protocol);


int rule_engine_remove_rule(
    uint32_t src_ip,
    uint32_t dst_ip,
    uint16_t src_port,
    uint16_t dst_port,
    uint8_t protocol);


int rule_engine_clear_rules(void);

If your previously created rule_engine.h uses different function names/structures, don't create a second API just for the test. The test should be adjusted to your existing driver API so the whole project remains consistent.

