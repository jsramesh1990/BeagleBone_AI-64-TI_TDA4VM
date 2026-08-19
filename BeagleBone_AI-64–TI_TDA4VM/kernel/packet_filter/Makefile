# ============================================================
# Packet Filter Kernel Module
#
# Platform : BeagleBone AI-64
# SoC      : TI TDA4VM
# Kernel   : Linux
#
# Directory:
#   kernel/packet_filter/
#
# ============================================================

# ------------------------------------------------------------
# Kernel Module Name
# ------------------------------------------------------------

obj-m += packet_filter.o


# ------------------------------------------------------------
# Packet Filter Module Objects
# ------------------------------------------------------------

packet_filter-objs := \
	packet_filter_main.o \
	packet_parser.o \
	rule_engine.o \
	logging.o \
	statistics.o


# ------------------------------------------------------------
# Kernel Build Directory
# ------------------------------------------------------------

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build


# ------------------------------------------------------------
# Current Source Directory
# ------------------------------------------------------------

PWD := $(shell pwd)


# ------------------------------------------------------------
# Cross Compilation
#
# For BeagleBone AI-64:
#
#   ARCH=arm64
#   CROSS_COMPILE=aarch64-linux-gnu-
#
# These can be overridden from command line.
# ------------------------------------------------------------

ARCH ?= arm64

CROSS_COMPILE ?= aarch64-linux-gnu-


# ------------------------------------------------------------
# Default Target
# ------------------------------------------------------------

.PHONY: all

all:
	$(MAKE) -C $(KERNEL_SRC) \
		M=$(PWD) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		modules


# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

.PHONY: build

build: all


# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

.PHONY: clean

clean:
	$(MAKE) -C $(KERNEL_SRC) \
		M=$(PWD) \
		clean


# ------------------------------------------------------------
# Install
# ------------------------------------------------------------

.PHONY: install

install: all
	$(MAKE) -C $(KERNEL_SRC) \
		M=$(PWD) \
		modules_install


# ------------------------------------------------------------
# Rebuild
# ------------------------------------------------------------

.PHONY: rebuild

rebuild: clean all


# ------------------------------------------------------------
# Check Kernel Configuration
# ------------------------------------------------------------

.PHONY: kernel-info

kernel-info:
	@echo "============================================"
	@echo " Packet Filter Kernel Build Information"
	@echo "============================================"
	@echo "Kernel Source : $(KERNEL_SRC)"
	@echo "Architecture  : $(ARCH)"
	@echo "Cross Compile : $(CROSS_COMPILE)"
	@echo "Source Dir    : $(PWD)"
	@echo "============================================"


# ------------------------------------------------------------
# Check Source Files
# ------------------------------------------------------------

.PHONY: check

check:
	@echo "Checking packet filter source files..."
	@test -f packet_filter_main.c || \
		(echo "ERROR: packet_filter_main.c missing"; exit 1)
	@test -f packet_parser.c || \
		(echo "ERROR: packet_parser.c missing"; exit 1)
	@test -f rule_engine.c || \
		(echo "ERROR: rule_engine.c missing"; exit 1)
	@test -f logging.c || \
		(echo "ERROR: logging.c missing"; exit 1)
	@test -f statistics.c || \
		(echo "ERROR: statistics.c missing"; exit 1)
	@echo "All source files found."


# ------------------------------------------------------------
# Static Analysis
# ------------------------------------------------------------

.PHONY: sparse

sparse:
	$(MAKE) -C $(KERNEL_SRC) \
		M=$(PWD) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		C=1 \
		modules


# ------------------------------------------------------------
# Verbose Build
# ------------------------------------------------------------

.PHONY: V

V:
	$(MAKE) -C $(KERNEL_SRC) \
		M=$(PWD) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		V=1 \
		modules


# ------------------------------------------------------------
# Module Information
# ------------------------------------------------------------

.PHONY: modinfo

modinfo: all
	@echo
	@echo "============================================"
	@echo " Module Information"
	@echo "============================================"
	@if [ -f packet_filter.ko ]; then \
		modinfo ./packet_filter.ko; \
	else \
		echo "packet_filter.ko not found"; \
	fi


# ------------------------------------------------------------
# Show Generated Files
# ------------------------------------------------------------

.PHONY: list

list:
	@echo "============================================"
	@echo " Generated Packet Filter Files"
	@echo "============================================"
	@find . -maxdepth 1 -type f \
		\( -name "*.o" \
		-o -name "*.ko" \
		-o -name "*.mod" \
		-o -name "*.mod.c" \
		-o -name "*.cmd" \
		-o -name "modules.order" \
		-o -name "Module.symvers" \) \
		-print


# ------------------------------------------------------------
# Remove Generated Files
# ------------------------------------------------------------

.PHONY: distclean

distclean:
	$(MAKE) -C $(KERNEL_SRC) \
		M=$(PWD) \
		clean
	rm -f *.ko
	rm -f *.o
	rm -f *.mod
	rm -f *.mod.c
	rm -f modules.order
	rm -f Module.symvers
	rm -f .*.cmd
	rm -rf .tmp_versions


# ------------------------------------------------------------
# Help
# ------------------------------------------------------------

.PHONY: help

help:
	@echo
	@echo "Packet Filter Kernel Module"
	@echo
	@echo "Available targets:"
	@echo
	@echo "  make                 Build kernel module"
	@echo "  make build           Build kernel module"
	@echo "  make clean           Clean generated files"
	@echo "  make rebuild         Clean and rebuild"
	@echo "  make install         Install module"
	@echo "  make kernel-info     Show kernel build configuration"
	@echo "  make check           Check source files"
	@echo "  make sparse          Run sparse/kernel static checking"
	@echo "  make V               Verbose build"
	@echo "  make modinfo         Display module information"
	@echo "  make list            List generated files"
	@echo "  make distclean       Deep clean"
	@echo "  make help            Display this help"
	@echo
