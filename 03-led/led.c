#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define CCM_CCGR1_BASE          (0X020C406C)
#define SW_MUX_GPIO1_IO03_BASE  (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE  (0X020E02F4)
#define GPIO1_DR_BASE           (0X0209C000)
#define GPIO1_GDIR_BASE         (0X0209C004)

static dev_t led_num;
static struct cdev *led_cdev;
static struct class *led_class;
static struct device *led0;

static void __iomem *iMX6ULL_CCM_CCGR1;
static void __iomem *iMX6ULL_SW_MUX_GPIO1_IO03;
static void __iomem *iMX6ULL_SW_PAD_GPIO1_IO03;
static void __iomem *iMX6ULL_GPIO_GDIR;
static void __iomem *iMX6ULL_GPIO1_DR;

/**
 * @brief   LED板级初始化
*/
static int led_board_init(void)
{
    u32 val;

    // 设置寄存器地址映射
    iMX6ULL_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
    iMX6ULL_SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    iMX6ULL_SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    iMX6ULL_GPIO_GDIR = ioremap(GPIO1_GDIR_BASE, 4);
    iMX6ULL_GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);

    // 使能外设时钟
    val = readl(iMX6ULL_CCM_CCGR1);
    val &= ~(3 << 26);
    val |= (3 << 26);
    writel(val, iMX6ULL_CCM_CCGR1);

    // 设置IOMUXC引脚复用和引脚属性
    writel(5, iMX6ULL_SW_MUX_GPIO1_IO03);
    writel(0x10B0, iMX6ULL_SW_PAD_GPIO1_IO03);

    // 设置GPIO引脚方向
    val = readl(iMX6ULL_GPIO_GDIR);
    val &= ~(1 << 3);
    val |= (1 << 3);
    writel(val, iMX6ULL_GPIO_GDIR);

    // 设置GPIO输出高电平，默认关闭LED
    val = readl(iMX6ULL_GPIO1_DR);
    val |= (1 << 3);
    writel(val, iMX6ULL_GPIO1_DR);

    return 0;
}

/**
 * @brief   LED板级释放
*/
static void led_board_deinit(void)
{
    // 取消寄存器地址映射
    iounmap(iMX6ULL_CCM_CCGR1);
    iounmap(iMX6ULL_SW_MUX_GPIO1_IO03);
    iounmap(iMX6ULL_SW_PAD_GPIO1_IO03);
    iounmap(iMX6ULL_GPIO_GDIR);
    iounmap(iMX6ULL_GPIO1_DR);
}

/**
 * @brief   LED板级释放
*/
static void led_board_ctrl(int status)
{
    u32 val;

    if (status == 1) {
        // 打开LED
        val = readl(iMX6ULL_GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, iMX6ULL_GPIO1_DR);
    } else {
        // 关闭LED
        val = readl(iMX6ULL_GPIO1_DR);
        val |= (1 << 3);
        writel(val, iMX6ULL_GPIO1_DR);
    }
}

static int led_open(struct inode *node, struct file *fp)
{
    return 0;
}

static ssize_t led_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
    return 0;
}

static ssize_t led_write(struct file *fp, const char __user *buf, size_t len, loff_t *off)
{
    int ret;
    unsigned char data_buf[1];
    unsigned char led_status;

    // 拷贝用户传入数据
    ret = copy_from_user(data_buf, buf, 1);
    if (ret < 0) {
        printk("led write failed!\n");
        return -EFAULT;
    }

    // 控制LED
    led_status = data_buf[0];
    if (led_status == 0) {
        led_board_ctrl(0);
    } else if (led_status == 1){
        led_board_ctrl(1);
    }

    return 0;
}

static int led_close(struct inode *node, struct file *fp)
{

    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open  = led_open,
    .read  = led_read,
    .write = led_write,
    .release = led_close
};

static int __init led_module_init(void)
{
    int ret;

    ret = led_board_init();
    if (ret != 0) {
        printk(KERN_WARNING"led_board_init failed!\n");
        return -1;
    }

    // 1. 申请设备号
    ret = alloc_chrdev_region(&led_num, 0, 1, "led");
    if (ret != 0) {
        printk(KERN_WARNING"alloc_chrdev_region failed!\n");
        return -1;
    }

    // 2. 申请cdev
    led_cdev = cdev_alloc();
    if (!led_cdev) {
        printk(KERN_WARNING"cdev_alloc failed!\n");
        return -1;
    }

    // 3. 初始化cdev
    led_cdev->owner = THIS_MODULE;
    led_cdev->ops = &led_fops;

    // 4. 注册cdev
    ret = cdev_add(led_cdev, led_num, 1);
    if (ret != 0) {
        printk(KERN_WARNING"cdev_add failed!\n");
        return -1;
    }

    // 5. 创建设备类
    led_class = class_create(THIS_MODULE, "led_class");
    if (!led_class) {
        printk(KERN_WARNING"class_create failed!\n");
        return -1;
    }

    // 6. 创建设备节点
    led0 = device_create(led_class, NULL, led_num, NULL, "led0");
    if (IS_ERR(led0)) {
        printk(KERN_WARNING"device_create failed!\n");
        return -1;
    }

    return 0;
}

static void __exit led_module_exit(void)
{
    led_board_deinit();

    // 1. 删除设备节点
    device_destroy(led_class, led_num);

    // 2. 删除设备类
    class_destroy(led_class);

    // 3. 删除cdev
    cdev_del(led_cdev);

    // 4. 释放设备号
    unregister_chrdev_region(led_num, 1);
}

module_init(led_module_init);
module_exit(led_module_exit);

MODULE_AUTHOR("mculover666");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("a led demo");