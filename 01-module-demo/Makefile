KERNEL_DIR := /home/mculover666/imx6ull/kernel/linux-imx6ull

# Kernel modules
obj-m += module_demo.o

# Specify flags for the module compilation.
#EXTRA_CFLAGS=-g -O0

build: kernel_modules

kernel_modules:
	make -C $(KERNEL_DIR) M=$(CURDIR) modules

clean:
	make -C $(KERNEL_DIR) M=$(CURDIR) clean