/**
 * @brief   基于platform平台驱动模型点亮led
 * @author  mculover666
 * @date    2022/07/02
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

struct gpioled_dev {
    dev_t led_dts_dev;                  /*!< 设备号 */
    struct cdev *led_dts_cdev;          /*!< cdev对象 */
    struct class *led_dts_class;        /*!< 设备类  */
    struct device *led_dts_device0;     /*!< 设备节点 */

    struct device_node *node;           /*!< 设备树节点  */
    int led_gpio;                       /*!< led使用的gpio编号  */
};

static struct gpioled_dev gpioled;

static int board_led_init(void)
{
    int ret;

    /* 设置LED所使用的GPIO */
    
    //在设备树寻找节点
    gpioled.node = of_find_node_by_path("/led0");
    if (!gpioled.node) {
        printk("of_find_node_by_path fail!");
        return -1;
    }

    //获取gpio属性值，得到LED使用的gpio编号
    gpioled.led_gpio = of_get_named_gpio(gpioled.node, "led-gpio", 0);
    if (gpioled.led_gpio < 0) {
        printk("of_get_named_gpio fail!");
        return -2;
    }
    printk("led-gpio num is %d", gpioled.led_gpio);
   
    // 申请gpio
    ret = gpio_request(gpioled.led_gpio, "led-gpio");
    if (ret < 0) {
        printk("gpio %d request fail!\n", gpioled.led_gpio);
        return -3;
    }

    //设置gpio方向并输出默认电平
    ret = gpio_direction_output(gpioled.led_gpio, 1);
    if (ret < 0) {
        printk("gpio_direction_output fail!");
        gpio_free(gpioled.led_gpio);
        return -4;
    }

    return 0;
}

static int led_dts_open(struct inode *inode, struct file *fp)
{
    /* 设置私有数据 */
    fp->private_data = &gpioled;
    return 0;
}

static int led_dts_read(struct file *fp, char __user *buf, size_t size, loff_t *off)
{
    return 0;
}

static int led_dts_write(struct file *fp, const char __user *buf, size_t size, loff_t *off)
{
     int ret;
    unsigned char data_buf[1];
    unsigned char led_status;
    struct gpioled_dev *dev = fp->private_data;

    // 拷贝用户传入数据
    ret = copy_from_user(data_buf, buf, 1);
    if (ret < 0) {
        printk("led write failed!\n");
        return -EFAULT;
    }

    // 控制LED
    led_status = data_buf[0];
    if (led_status == 0) {
        // 关闭LED，高电平关闭
        gpio_set_value(dev->led_gpio, 1);
    } else if (led_status == 1){
        // 打开LED，低电平打开
        gpio_set_value(dev->led_gpio, 0);
    }

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

static int led_probe(struct platform_device *dev)
{
    int ret;

    printk("led probe called!\n");

    // led硬件初始化
    ret = board_led_init();
    if (ret < 0) {
        printk("board_led_init fail, err is %d", ret);
        return -1;
    }

    //分配cdev设备号
    ret = alloc_chrdev_region(&gpioled.led_dts_dev, 0, 1, "led_dts");
    if (ret != 0) {
        printk("alloc_chrdev_region fail!");
        return -1;
    }

    //初始化cdev
    gpioled.led_dts_cdev = cdev_alloc();
    if (!gpioled.led_dts_cdev) {
        printk("cdev_alloc fail!");
        return -1;
    }

    //设置fop操作函数
    gpioled.led_dts_cdev->owner = THIS_MODULE;
    gpioled.led_dts_cdev->ops = &led_dts_fops;

    //注册cdev
    cdev_add(gpioled.led_dts_cdev, gpioled.led_dts_dev, 1);

    // 创建设备类
    gpioled.led_dts_class = class_create(THIS_MODULE, "led_dts_class");
    if (!gpioled.led_dts_class) {
        printk("class_create fail!");
        return -1;
    }

    //创建设备节点
    gpioled.led_dts_device0 = device_create(gpioled.led_dts_class, NULL, gpioled.led_dts_dev, NULL, "led0");
    if (IS_ERR(gpioled.led_dts_device0)) {
        printk("device_create led_dts_device0 fail!");
        return -1;
    }

    return 0;
}

static int led_remove(struct platform_device *dev)
{
    printk("led remove called!\n");

    // 将设备从内核删除
    cdev_del(gpioled.led_dts_cdev);

    // 释放设备号
    unregister_chrdev_region(gpioled.led_dts_dev, 1);

    // 删除设备节点
    device_destroy(gpioled.led_dts_class, gpioled.led_dts_dev);

    // 删除设备类
    class_destroy(gpioled.led_dts_class);

    return 0;
}

/**
 * @brief   设备树匹配列表
*/
static const struct of_device_id plat_led_of_match[] = {
    { .compatible = "gpio-led" },
    { },
};

/**
 * @brief   传统id方式匹配列表
*/
static const struct platform_device_id plat_led_id[] = {
    { "gpio-led", 0 },
    { },
};

static struct platform_driver led_driver = {
    .probe = led_probe,
    .remove = led_remove,
    .driver = {
        .name = "platform_led",
        .owner = THIS_MODULE,
        .of_match_table = plat_led_of_match,
    },
    .id_table = plat_led_id,
};

static int __init platform_led_init(void)
{
    int ret;

    ret = platform_driver_register(&led_driver);
    if (ret < 0) {
        printk("platform_driver_register fail!\n");
        return -1;
    }
    printk("platform_driver_register ok!\n");
    return 0;
}

static void __exit platform_led_exit(void)
{
    printk("platform_driver_unregister!\n");
    platform_driver_unregister(&led_driver);
}

module_init(platform_led_init);
module_exit(platform_led_exit);

MODULE_AUTHOR("Mculover666");
MODULE_LICENSE("GPL");
