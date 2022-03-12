#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/module.h>

#define HELLO_DRV_MAJOR 200
#define HELLO_DRV_NAME  "hello drv"

static int hello_drv_open(struct inode *inode, struct file *filp)
{
    printk("hello drv open!\n");

    return 0;
}

ssize_t hello_drv_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    printk("hello drv read!\n");

    return 0;
}

ssize_t hello_drv_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    printk("hello drv write!\n");

    return 0;
}

static int hello_drv_release(struct inode *inode, struct file *filp)
{
    printk("hello drv release!\n");

    return 0;   
}

static struct file_operations hello_drv_fops = {
    .owner = THIS_MODULE,
    .open = hello_drv_open,
    .read = hello_drv_read,
    .write = hello_drv_write,
    .release = hello_drv_release,
};

static int __init hello_drv_init(void)
{
    int ret;

    ret = register_chrdev(HELLO_DRV_MAJOR, HELLO_DRV_NAME, &hello_drv_fops);
    if (ret < 0) {
        printk("hello drv register fail\n");
    }

    printk("hello drv init!\n");

    return 0;
}

static void __exit hello_drv_exit(void)
{
    unregister_chrdev(HELLO_DRV_MAJOR, HELLO_DRV_NAME);
    printk("hello drv exit!\n");
}

module_init(hello_drv_init);
module_exit(hello_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mculover666");
