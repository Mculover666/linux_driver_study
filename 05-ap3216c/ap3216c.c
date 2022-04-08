#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

struct ap3216c_data {
    uint16_t ir;    /*!< Infrared Radiation, 红外LED */
    uint16_t als;   /*!< Ambilent Light Sensor, 数字环境光传感器，16位有效线性输出 */
    uint16_t ps;    /*!< Proximity Sensor, 接近传感器，10位有效线性输出  */
};

struct ap3216c_dev {
    dev_t dev;
    struct cdev *cdev;
    struct class *class;
    struct device *device;

    struct i2c_client *client;

    struct ap3216c_data data;
};

static struct ap3216c_dev ap3216c;

/**
 * @brief       读AP3216C寄存器
 * @param[in]   dev AP3216C设备指针
 * @param[in]   reg 要读取的寄存器地址
 * @param[out]  val 读取到的值
 * @param[in]   len 要读取的数据长度
 * @return      errcode, 0 is success, -1 is fail
*/
static int ap3216c_read_reg(struct ap3216c_dev *dev, uint8_t reg, uint8_t *val)
{
    struct i2c_client *client = dev->client;
    struct i2c_msg msg[2];
    int ret;

    // 组装数据，要读取的寄存器地址
    msg[0].addr = client->addr;
    msg[0].flags = 0;           // 标记为发送数据
    msg[0].buf = &reg;
    msg[0].len = 1;

    // 组装数据，读取到的寄存器数据
    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;    // 标记为读取数据
    msg[1].buf = (void*)val;
    msg[1].len = 1;

    // 传输数据
    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret != 2) {
        printk("ap3216c_read_reg fail, ret is %d, reg is 0x%x\n", ret, reg);
        return -1;
    }

    return 0;
}

/**
 * @brief       写AP3216C寄存器
 * @param[in]   dev AP3216C设备指针
 * @param[in]   reg 要写入的寄存器地址
 * @param[in]   val 要写入的值
 * @param[in]   len 要写入的数据长度
 * @return      errcode, 0 is success, -1 is fail
*/
static int ap3216c_write_reg(struct ap3216c_dev *dev, uint8_t reg, uint8_t val)
{
    struct i2c_client *client = dev->client;
    struct i2c_msg msg;
    int ret;

    uint8_t send_buf[2];

    // 组装数据
    send_buf[0] = reg;
    send_buf[1] = val;

    msg.addr = client->addr;
    msg.flags = 0;           // 标记为发送数据
    msg.buf = send_buf;
    msg.len = 2;

    // 传输数据
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret != 1) {
        printk("ap3216c_write_reg fail, ret is %d, reg is 0x%x\n", ret, reg);
        return -1;
    }

    return 0;
}

/**
 * @brief   AP3216C传感器硬件初始化
 * @param   none
 * @retval  errcode
*/
static int ap3216c_board_init(void)
{
    int ret;
    uint8_t val;

    // 设置AP3216C系统模式，软复位
    ret = ap3216c_write_reg(&ap3216c, 0x00, 0x04);
    if (ret < 0) {
        printk("ap3216 soft reset fail!\n");
        return -1;
    }

    // 软复位后至少等待10ms
    mdelay(150);

    // 设置AP3216C系统模式，ALS+PS+IR单次模式
    ret = ap3216c_write_reg(&ap3216c, 0x00, 0x03);
    if (ret < 0) {
        printk("ap3216 activate fail!\n");
        return -1;
    }

    mdelay(150);

    // 读模式寄存器，确认模式写入成功
    ret = ap3216c_read_reg(&ap3216c, 0x00, &val);
    if (ret < 0) {
        printk("ap3216 read mode fail!\n");
        return -1;
    }

    printk("ap3216 mode: %d\n", val);

    return 0;
}

/**
 * @brief   AP3216C传感器读取数据
 * @param   none
 * @retval  errcode
*/
static int ap3216c_board_read_data(struct ap3216c_dev *dev)
{
    uint8_t low_val, high_val;

    // IR
    ap3216c_read_reg(dev, 0x0A, &low_val);
    ap3216c_read_reg(dev, 0x0B, &high_val);
    if (low_val & 0x80) {
        dev->data.ir = 0;
    } else {
        dev->data.ir = ((uint16_t)high_val << 2) | (low_val & 0x03);
    }

    // ALS
    ap3216c_read_reg(dev, 0x0C, &low_val);
    ap3216c_read_reg(dev, 0x0D, &high_val);
    dev->data.als = ((uint16_t)high_val << 8) | low_val;

    // PS
    ap3216c_read_reg(dev, 0x0E, &low_val);
    ap3216c_read_reg(dev, 0x0F, &high_val);
    if (low_val & 0x40) {
        dev->data.ps = 0;
    } else {
        dev->data.ps = ((uint16_t)(high_val & 0x3F) << 4) | (low_val & 0x0F);
    }

    return 0;
}

static int ap3216c_open(struct inode *node, struct file *fp)
{
    fp->private_data = &ap3216c;
    return 0;
}

static int ap3216c_read(struct file *fp, char __user *buf, size_t size, loff_t *off)
{
    struct ap3216c_dev *dev = (struct ap3216c_dev *)fp->private_data;
    uint16_t data[3];
    long ret;

    if (!dev) {
        printk("ap3216c dev get fail!\n");
        return -1;
    }

    ap3216c_board_read_data(dev);

    data[0] = dev->data.ir;
    data[1] = dev->data.als;
    data[2] = dev->data.ps;

    ret = copy_to_user(buf, data, sizeof(data));

    return 0;
}

static int ap3216c_write(struct file *fp, const char __user *buf, size_t size, loff_t *off)
{
    return 0;
}

static int ap3216c_release(struct inode *node, struct file *fp)
{
    return 0;
}

static struct file_operations ap3216c_fops = {
    .owner = THIS_MODULE,
    .open = ap3216c_open,
    .read = ap3216c_read,
    .write = ap3216c_write,
    .release = ap3216c_release,
};

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;

    // 申请设备号
    ret = alloc_chrdev_region(&ap3216c.dev, 0, 1, "ap3216c");
    if (ret != 0) {
        printk("alloc_chrdev_region fail!");
        return -1;
    }

    // 创建cdev
    ap3216c.cdev = cdev_alloc();
    if (!ap3216c.cdev) {
        printk("cdev_alloc fail!");
        return -1;
    }
    ap3216c.cdev->owner = THIS_MODULE;
    ap3216c.cdev->ops = &ap3216c_fops;

    // 注册cdev
    cdev_add(ap3216c.cdev, ap3216c.dev, 1);

    // 创建设备类
    ap3216c.class = class_create(THIS_MODULE, "ap3216c");
    if (!ap3216c.class) {
        printk("class_create fail!");
        return -1;
    }

    // 创建设备节点
    ap3216c.device = device_create(ap3216c.class, NULL, ap3216c.dev, NULL, "ap3216c");
    if (IS_ERR(ap3216c.device)) {
        printk("device_create led_dts_device0 fail!");
        return -1;
    }

    ap3216c.client = client;

    // 硬件初始化
    ret = ap3216c_board_init();
    if (ret < 0) {
        printk("ap3216c_board_init fail!\n");
    }

    return 0;
}

static int ap3216c_remove(struct i2c_client *client)
{
    // 将设备从内核删除
    cdev_del(ap3216c.cdev);

    // 释放设备号
    unregister_chrdev_region(ap3216c.dev, 1);

    // 删除设备节点
    device_destroy(ap3216c.class, ap3216c.dev);

    // 删除设备类
    class_destroy(ap3216c.class);

    return 0;
}

/**
 * @brief   设备树匹配列表
*/
static const struct of_device_id ap3216c_of_match[] = {
    { .compatible = "atk,ap3216c" },
    { },
};

/**
 * @brief   传统id方式匹配列表
*/
static const struct i2c_device_id ap3216c_id[] = {
    { "atk,ap3216c", 0 },
    { },
};

/**
 * @brief   i2c驱动结构体
*/
static struct i2c_driver ap3216c_driver = {
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ap3216c",
        .of_match_table = ap3216c_of_match,
    },
    .id_table = ap3216c_id,
};

static int __init ap3216c_module_init(void)
{
    int ret;

    ret = i2c_add_driver(&ap3216c_driver);
    if (ret < 0) {
        printk("i2c_add_driver fail!\n");
        return -1;
    }

    return 0;
}

static void __exit ap3216c_module_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216c_module_init)
module_exit(ap3216c_module_exit)

MODULE_AUTHOR("Mculover666");
MODULE_LICENSE("GPL");