KERNEL_DIR = /home/mculover666/develop/imx6ull/linux-imx6ull
obj-m := platform_led.o

build: kernel_module

kernel_module:
	$(MAKE)	-C $(KERNEL_DIR) M=$(CURDIR) modules

clean:
	$(MAKE)	-C $(KERNEL_DIR) M=$(CURDIR) clean