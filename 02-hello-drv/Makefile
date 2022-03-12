KERNEL_DIR := /home/mculover666/imx6ull/kernel/linux-imx6ull
CURRENT_PATH := $(shell pwd)
obj-m := hello_drv.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURRENT_PATH) clean