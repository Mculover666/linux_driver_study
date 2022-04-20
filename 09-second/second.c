#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

struct second_dev {
    dev_t dev;                  /*!< 设备号             */
    struct cdev *cdev;          /*!< cdev对象           */
    struct class *class;        /*!< 设备类             */
    struct device *device0;     /*!< 设备节点           */

    struct timer_list timer;    /*!< 软件定时器         */
    atomic_t counter;           /*!< 秒计数             */
};

static struct second_dev second;

static void second_timer_handler(unsigned long arg)
{
    // 重启定时器
    mod_timer(&second.timer, jiffies + HZ);

    // 秒计数
    atomic_inc(&second.counter);

    printk(KERN_INFO "current jiffies is %ld\n", jiffies);
}

static int second_open(struct inode *inode, struct file *fp)
{
    // 初始化定时器，超时时间1s
    init_timer(&second.timer);
    second.timer.expires = jiffies + HZ;
    second.timer.function = second_timer_handler;
    
    // 初始化秒计数值
    atomic_set(&second.counter, 0);

    // 注册定时器到内核，开始运行
    add_timer(&second.timer);

    fp->private_data = &second;

    return 0;
}

static int second_read(struct file *fp, char __user *buf, size_t size, loff_t *off)
{
    int ret;
    int sec;
    struct second_dev *dev = (struct second_dev *)fp->private_data;

    sec = atomic_read(&dev->counter);
    ret = copy_to_user(buf, &sec, sizeof(sec));

    return 0;
}

static int second_write(struct file *fp, const char __user *buf, size_t size, loff_t *off)
{
    return 0;
}

static int second_release(struct inode *inode, struct file *fp)
{
    // 将定时器从内核删除
    del_timer(&second.timer);
    return 0;
}

static struct file_operations second_fops = {
    .owner = THIS_MODULE,
    .open = second_open,
    .read = second_read,
    .write = second_write,
    .release = second_release,
};

static int __init second_module_init(void)
{
    int ret;

    //分配cdev设备号
    ret = alloc_chrdev_region(&second.dev, 0, 1, "second");
    if (ret != 0) {
        printk("alloc_chrdev_region fail!");
        return -1;
    }

    //初始化cdev
    second.cdev = cdev_alloc();
    if (!second.cdev) {
        printk("cdev_alloc fail!");
        return -1;
    }

    //设置fop操作函数
    second.cdev->owner = THIS_MODULE;
    second.cdev->ops = &second_fops;

    //注册cdev
    cdev_add(second.cdev, second.dev, 1);

    // 创建设备类
    second.class = class_create(THIS_MODULE, "second_class");
    if (!second.class) {
        printk("class_create fail!");
        return -1;
    }

    //创建设备节点
    second.device0 = device_create(second.class, NULL, second.dev, NULL, "second0");
    if (IS_ERR(second.device0)) {
        printk("device_create device0 fail!");
        return -1;
    }

    return 0;
}

static void __exit second_module_exit(void)
{
    // 将设备从内核删除
    cdev_del(second.cdev);

    // 释放设备号
    unregister_chrdev_region(second.dev, 1);

    // 删除设备节点
    device_destroy(second.class, second.dev);

    // 删除设备类
    class_destroy(second.class);
}

module_init(second_module_init);
module_exit(second_module_exit);

MODULE_AUTHOR("Mculover666");
MODULE_LICENSE("GPL");
