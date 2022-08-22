ARCH= arm
MVTOOL_PREFIX = /home/xxw/rk3288/arm-eabi-4.6/bin/arm-eabi-
CROSS_COMPILE= $(MVTOOL_PREFIX)
KDIR := /home/xxw/rk3288/firefly-rk3288-kernel
TARGET				=wk2xxx_spi
EXEC = $(TARGET)
obj-m :=$(TARGET).o
PWD :=$(shell pwd)
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.o *~core.depend.*.cmd *.ko *.mod.c .tmp_versions $(TARGET)
