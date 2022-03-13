#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define HELLO_DRV_NAME  "hello drv"
#define HELLO_DRV_CLASS "hello_drv_class"
#define HELLO_DRV_DEVICE "hello_drv0"

#define MEM_SIZE 0x1000

static dev_t hello_drv_dev;
static struct cdev  *hello_drv_cdev;
static struct class *hello_drv_class;
static struct device *hello_drv0;

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

    // 动态申请cdev
    hello_drv_cdev = cdev_alloc();
    if (!hello_drv_cdev) {
        printk(KERN_WARNING"cdev_alloc failed!\n");
        return -1;
    }

    // 动态申请设备号
    ret = alloc_chrdev_region(&hello_drv_dev, 0, 1, HELLO_DRV_NAME);
    if (ret != 0) {
        printk(KERN_WARNING"alloc_chrdev_region failed!\n");
        return -1;
    }

    // 初始化cdev结构体
    hello_drv_cdev->owner = THIS_MODULE;
    hello_drv_cdev->ops = &hello_drv_fops;

    // 将设备添加到内核中
    cdev_add(hello_drv_cdev, hello_drv_dev, 1);

    // 创建设备类
    hello_drv_class = class_create(THIS_MODULE, HELLO_DRV_CLASS);
    if (!hello_drv_class) {
        printk(KERN_WARNING"class_create failed!\n");
        return -1;
    }

    // 创建设备节点
    hello_drv0 = device_create(hello_drv_class, NULL, hello_drv_dev, NULL, HELLO_DRV_DEVICE);
    if (IS_ERR(hello_drv0)) {
        printk(KERN_WARNING"device_create failed!\n");
        return -1;
    }

    printk("hello drv init success!\n");
    return 0;
}

static void __exit hello_drv_exit(void)
{
    // 将设备从内核删除
    cdev_del(hello_drv_cdev);

    // 释放设备号
    unregister_chrdev_region(hello_drv_dev, 1);

    // 删除设备节点
    device_destroy(hello_drv_class, hello_drv_dev);

    // 删除设备类
    class_destroy(hello_drv_class);

    printk("hello drv exit!\n");
}

module_init(hello_drv_init);
module_exit(hello_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mculover666");
