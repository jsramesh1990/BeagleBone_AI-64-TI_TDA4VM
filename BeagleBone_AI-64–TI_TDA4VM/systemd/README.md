What this service does
systemd
   │
   ▼
network-online.target
   │
   ▼
packet-filter.service
   │
   ▼
modprobe packet_filter
   │
   ▼
packet_filter.ko
   │
   ▼
Kernel Packet Filter
   │
   ├── packet_parser
   ├── rule_engine
   ├── statistics
   └── logging

During shutdown:

systemd stop
     │
     ▼
packet-filter.service
     │
     ▼
modprobe -r packet_filter
     │
     ▼
Driver cleanup
     │
     ├── unregister hooks
     ├── release resources
     ├── destroy statistics
     └── remove module
Install it on the target
sudo cp systemd/packet-filter.service \
    /etc/systemd/system/

Then reload systemd:

sudo systemctl daemon-reload

Enable at boot:

sudo systemctl enable packet-filter.service

Start it:

sudo systemctl start packet-filter.service

Check:

sudo systemctl status packet-filter.service

Verify the module:

lsmod | grep packet_filter

View service logs:

journalctl -u packet-filter.service
Stop/unload
sudo systemctl stop packet-filter.service

Verify:

lsmod | grep packet_filter
Test automatic boot
sudo systemctl enable packet-filter.service
sudo reboot

After reboot:

systemctl status packet-filter.service
lsmod | grep packet_filter
dmesg | grep -i packet_filter

One important point for your project: the systemd service should handle driver/module startup, while the actual packet-filter configuration/rules should be handled separately. That keeps your architecture clean:

systemd
   │
   ▼
packet-filter.service
   │
   ▼
Load packet_filter.ko
   │
   ▼
Kernel Driver
   │
   ├── packet_parser.c
   ├── rule_engine.c
   ├── statistics.c
   └── logging.c
            ▲
            │
       userspace/config
            │
            ├── whitelist.conf
            ├── blacklist.conf
            └── monitoring.conf
