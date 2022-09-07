KERNEL_DIR = /home/mculover666/develop/imx6ull/linux-imx6ull/output
obj-m := fb_st7789_spi.o

build: kernel_module

kernel_module:
	$(MAKE)	-C $(KERNEL_DIR) M=$(CURDIR) modules

clean:
	$(MAKE)	-C $(KERNEL_DIR) M=$(CURDIR) clean