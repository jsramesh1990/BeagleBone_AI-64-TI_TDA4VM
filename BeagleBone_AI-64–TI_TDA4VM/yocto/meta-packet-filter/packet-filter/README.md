Add the layer to bblayers.conf

From your Yocto build directory:

bitbake-layers add-layer \
    ../yocto/meta-packet-filter

Verify:

bitbake-layers show-layers

You should see:

meta-packet-filter
6. Build
bitbake packet-filter

Then integrate it into your image:

IMAGE_INSTALL:append = " packet-filter"

For example in conf/local.conf:

IMAGE_INSTALL:append = " packet-filter"

Then:

bitbake <your-image>
Overall Yocto flow
yocto/meta-packet-filter
          |
          v
       layer.conf
          |
          v
    packet-filter.bb
          |
          +------------------+
          |                  |
          v                  v
   Kernel module        systemd service
   packet_filter.ko     packet-filter.service
          |                  |
          +---------+--------+
                    |
                    v
              Root Filesystem
                    |
                    v
              SD/eMMC Image
                    |
                    v
             BeagleBone AI-64
                    |
                    v
             modprobe driver
                    |
                    v
             /dev/packet_filter
                    |
                    v
             userspace tools

One correction to keep the project clean: your existing meta-packet-filter/packet-filter/ directory should be treated as layer data/configuration, while the actual kernel source should remain under:

BeagleBone_AI-64–TI_TDA4VM/kernel/packet_filter/

and the Yocto recipe should package/build that source rather than maintaining a second copy of the driver.
