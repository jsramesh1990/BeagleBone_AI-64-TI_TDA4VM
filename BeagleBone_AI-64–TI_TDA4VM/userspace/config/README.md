One important point: 192.168.1.0 is a subnet, not a single IPv4 host. If your current rule_engine.c only performs exact uint32_t IP matching, rules such as:

ALLOW 192.168.1.0  0.0.0.0  0  22 TCP

will not automatically mean 192.168.1.x. For true subnet rules, your configuration format should eventually include a prefix length, for example:

ALLOW 192.168.1.0/24  0.0.0.0/0  0  22 TCP

That is the better design for the final packet-filter project.
