The communication flow is:

userspace application
        |
        | filter_add_rule()
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
        +-------------------+
        |                   |
        v                   v
 packet_parser.c       rule_engine.c
                            |
                            v
                       statistics.c

One thing you should fix before compiling the whole project: libfilter.c and test_rule_engine.c currently assume specific ioctl/function names. Your kernel/packet_filter/ioctl_defs.h must define the actual ABI consistently. Otherwise you'll get compile errors such as PACKET_FILTER_IOCTL_ADD_RULE undeclared. The clean approach is to make ioctl_defs.h the single shared kernel/userspace ABI header.
