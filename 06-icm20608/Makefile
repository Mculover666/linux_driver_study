KERNEL_DIR = /home/mculover666/imx6ull/kernel/linux-imx6ull
obj-m = icm20608.o

build: kernel_module

kernel_module:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean
