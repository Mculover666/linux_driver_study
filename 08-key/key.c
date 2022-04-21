/******************************************************************
 * @brief       按键驱动
 * @author      Mculover666
 * 
 * changelog
 * v1.0 2022/4/20 初步完成驱动，基于标志位原始写法          
 * v1.1 2022/4/21 添加软件定时器进行消抖
 * 
******************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/timer.h>

#define KEY0_VALUE  0x01

struct key_dev {
    dev_t dev;                  /*!< 设备号             */
    struct cdev *cdev;          /*!< cdev对象           */
    struct class *class;        /*!< 设备类             */
    struct device *device0;     /*!< 设备节点           */

    struct device_node *node;   /*!< 设备树节点         */
    int gpio;                   /*!< key使用的gpio编号  */
    int debounce_interval;      /*!< 去抖时长           */
    atomic_t    keyval;         /*!< 键值               */
    atomic_t    press;          /*!< 标志按键是否按下    */
    struct timer_list  timer;   /*!< 用于软件消抖        */

    int irq;                    /*!< 中断号             */
};

static struct key_dev key;

static irqreturn_t key0_handler(int irq, void *dev_id)
{
    int val;
    struct key_dev *dev = (struct key_dev *)dev_id;

    if (dev->debounce_interval) {
        // 启动软件定时器, 消抖时间后再去读取
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->debounce_interval));
        dev->timer.data = (volatile long)dev_id;
    } else {
        // 立即读取
        val = gpio_get_value(dev->gpio);
        if (val == 0) {
            atomic_set(&dev->keyval, KEY0_VALUE);
            atomic_set(&key.press, 1);
        } else {
            atomic_set(&dev->keyval, 0xFF);
        }
    }

    return IRQ_RETVAL(IRQ_HANDLED);
}

static void timer_handler(unsigned long arg)
{
    int val;
    struct key_dev *dev = (struct key_dev *)arg;

    val = gpio_get_value(dev->gpio);
    if (val == 0) {
        atomic_set(&dev->keyval, KEY0_VALUE);
        atomic_set(&key.press, 1);
    } else {
        atomic_set(&dev->keyval, 0xFF);
    }
}

static int key_init(void)
{
    int ret;
    uint32_t debounce_interval = 0;

    // 获取设备树节点
    key.node = of_find_node_by_name(NULL, "key0");
    if (!key.node) {
        printk("key0 node find fail!\n");
        return -1;
    }

    // 提取gpio
    key.gpio = of_get_named_gpio(key.node, "key-gpio", 0);
    if (key.gpio < 0) {
        printk("find key-gpio propname fail!\n");
        return -1;
    }

    // 初始化gpio
    ret = gpio_request(key.gpio, "key-gpio");
    if (ret < 0) {
        printk("gpio request fail!\n");
        return -1;
    }
    gpio_direction_input(key.gpio);

    // 解析设备树，获取去抖时长
    if (of_property_read_u32(key.node, "debounce-interval", &debounce_interval) < 0) {
        printk("find debounce-interval fail!\n");
        goto error;
    }

    // 设置去抖时长
    if (debounce_interval) {
        ret = gpio_set_debounce(key.gpio, key.debounce_interval * 1000);
        if (ret < 0) {
            printk("gpio_set_debounce not support, use soft timer!\n");
            
            // 设置软件定时器用于消抖
            init_timer(&key.timer);
            key.timer.function = timer_handler;
            key.debounce_interval = debounce_interval;
        }
    }

    // 获取中断号
    key.irq = gpio_to_irq(key.gpio);
    if (key.irq < 0) {
        printk("gpio_to_irq fail!\n");
        goto error;
    }

    // 申请中断
    ret = request_irq(key.irq, key0_handler, IRQF_TRIGGER_FALLING, "key_irq", &key);
    if (ret < 0) {
        printk("irq request fail, ret is %d!\n", ret);
        goto error;
    }

    // 初始化键值与标志位
    atomic_set(&key.keyval, 0xFF);
    atomic_set(&key.press, 0);

    return 0;

error:
    gpio_free(key.gpio);
    return -1;
}

static void key_deinit(void)
{
    // 释放中断
    free_irq(key.irq, &key);

    // 释放gpio
    gpio_free(key.gpio);

    // 删除定时器
    del_timer(&key.timer);
}

static int key_open(struct inode *inode, struct file *fp)
{
    fp->private_data = &key;
    return 0;
}

static int key_read(struct file *fp, char __user *buf, size_t size, loff_t *off)
{
    int ret;
    int keyval, press;
    struct key_dev *dev = (struct key_dev *)fp->private_data;

    press = atomic_read(&dev->press);
    keyval = atomic_read(&dev->keyval);

    if (press == 1 && keyval != 0xFF) {
        atomic_set(&key.press, 0);
        ret = copy_to_user(buf, &keyval, sizeof(keyval));
        return 0;
    }
   
    return -1;
}

static int key_write(struct file *fp, const char __user *buf, size_t size, loff_t *off)
{
    return 0;
}

static int key_release(struct inode *inode, struct file *fp)
{
    fp->private_data = NULL;
    return 0;
}

static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
    .write = key_write,
    .release = key_release,
};


static int gpio_key_probe(struct platform_device *pdev)
{
    int ret;

    //分配cdev设备号
    ret = alloc_chrdev_region(&key.dev, 0, 1, "key");
    if (ret != 0) {
        printk("alloc_chrdev_region fail!");
        return -1;
    }

    //初始化cdev
    key.cdev = cdev_alloc();
    if (!key.cdev) {
        printk("cdev_alloc fail!");
        return -1;
    }

    //设置fop操作函数
    key.cdev->owner = THIS_MODULE;
    key.cdev->ops = &key_fops;

    //注册cdev
    cdev_add(key.cdev, key.dev, 1);

    // 创建设备类
    key.class = class_create(THIS_MODULE, "key_class");
    if (!key.class) {
        printk("class_create fail!");
        return -1;
    }

    //创建设备节点
    key.device0 = device_create(key.class, NULL, key.dev, NULL, "key0");
    if (IS_ERR(key.device0)) {
        printk("device_create device0 fail!");
        return -1;
    }

    ret = key_init();
    if (ret < 0) {
        printk("key init fail!\n");
        return -1;
    }

    return 0;
}

static int gpio_key_remove(struct platform_device *pdev)
{
    // 按键去初始化
    key_deinit();

    // 将设备从内核删除
    cdev_del(key.cdev);

    // 释放设备号
    unregister_chrdev_region(key.dev, 1);

    // 删除设备节点
    device_destroy(key.class, key.dev);

    // 删除设备类
    class_destroy(key.class);

    return 0;
}

static const struct of_device_id gpio_key_of_match[] = {
    { .compatible = "atk, gpio-key" },
    { },
};

static struct platform_driver gpio_key_device_driver = {
	.probe		= gpio_key_probe,
	.remove		= gpio_key_remove,
	.driver		= {
        .owner  = THIS_MODULE,
		.name	= "gpio-key",
		.of_match_table = of_match_ptr(gpio_key_of_match),
	}
};

static int __init key_module_init(void)
{
    return platform_driver_register(&gpio_key_device_driver);
}

static void __exit key_module_exit(void)
{
    platform_driver_unregister(&gpio_key_device_driver);
}

module_init(key_module_init);
module_exit(key_module_exit);

MODULE_AUTHOR("Mculover666");
MODULE_LICENSE("GPL");