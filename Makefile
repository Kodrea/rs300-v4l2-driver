obj-m += rs300.o

ifdef DEBUG
ccflags-y += -DDEBUG
endif

ifdef CONFIG_RS300_LEGACY_MENU
ccflags-y += -DCONFIG_RS300_LEGACY_MENU
endif

KERNELRELEASE ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KERNELRELEASE)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

dtbo: rs300-overlay.dtbo

%.dtbo: %.dts
	dtc -@ -I dts -O dtb -o $@ $<
