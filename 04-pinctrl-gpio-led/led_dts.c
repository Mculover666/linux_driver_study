#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

dev_t led_dts_dev;
struct cdev *led_dts_cdev;
struct class *led_dts_class;
struct device *led_dts_device0;

static int led_dts_open(struct inode *inode, struct file *fp)
{
    return 0;
}

static int led_dts_read(struct file *fp, char __user *buf, size_t size, loff_t *off)
{
    return 0;
}

static int led_dts_write(struct file *fp, const char __user *buf, size_t size, loff_t *off)
{
    return 0;
}

static int led_dts_release(struct inode *inode, struct file *fp)
{
    return 0;
}

static struct file_operations led_dts_fops = {
    .owner = THIS_MODULE,
    .open = led_dts_open,
    .read = led_dts_read,
    .write = led_dts_write,
    .release = led_dts_release,
};

static int __init led_dts_init(void)
{
    int ret;

    //分配cdev设备号
    ret = alloc_chrdev_region(&led_dts_dev, 0, 1, "led_dts");
    if (ret != 0) {
        printk("alloc_chrdev_region fail!");
        return -1;
    }

    //初始化cdev
    led_dts_cdev = cdev_alloc();
    if (!led_dts_cdev) {
        printk("cdev_alloc fail!");
        return -1;
    }

    //设置fop操作函数
    led_dts_cdev->owner = THIS_MODULE;
    led_dts_cdev->ops = &led_dts_fops;

    //注册cdev
    cdev_add(led_dts_cdev, led_dts_dev, 1);

    // 创建设备类
    led_dts_class = class_create(THIS_MODULE, "led_dts_class");
    if (!led_dts_class) {
        printk("class_create fail!");
        return -1;
    }

    //创建设备节点
    led_dts_device0 = device_create(led_dts_class, NULL, led_dts_dev, NULL, "led0");
    if (IS_ERR(led_dts_device0)) {
        printk("device_create led_dts_device0 fail!");
        return -1;
    }

    return 0;
}

static void __exit led_dts_exit(void)
{
    // 将设备从内核删除
    cdev_del(led_dts_cdev);

    // 释放设备号
    unregister_chrdev_region(led_dts_dev, 1);

    // 删除设备节点
    device_destroy(led_dts_class, led_dts_dev);

    // 删除设备类
    class_destroy(led_dts_class);
}

module_init(led_dts_init);
module_exit(led_dts_exit);

MODULE_AUTHOR("Mculover666");
MODULE_LICENSE("GPL");
