obj-m := rs300.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

dtbo: rs300-overlay.dtbo

%.dtbo: %.dts
	dtc -@ -I dts -O dtb -o $@ $<
