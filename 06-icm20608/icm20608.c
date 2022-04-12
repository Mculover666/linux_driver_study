#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

struct icm20608_row_data {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temperature;
};

struct icm20608_dev {
    dev_t dev;
    struct cdev *cdev;
    struct class *class;
    struct device *device;

    struct device_node *node;
    int cs_gpio;
    
    struct spi_device *spi;

    struct icm20608_row_data data;
};

static struct icm20608_dev icm20608;

static int icm20608_send_then_recv(struct icm20608_dev *dev, uint8_t *send_buf, ssize_t send_len, uint8_t *recv_buf, ssize_t recv_len)
{
    int ret;
    struct spi_device *spi;
    struct spi_message m;
    struct spi_transfer *t;

    if (!dev) {
        return -1;
    }

    spi = dev->spi;
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    if (!t) {
        printk("spi_transfer kzalloc fail!\n");
        return -1;
    }

    /* 使能片选 */
    gpio_set_value(dev->cs_gpio, 0);

    /* 发送数据 */
    if (send_buf && send_len != 0) {
        t->tx_buf = send_buf;
        t->len = send_len;
        spi_message_init(&m);
        spi_message_add_tail(t, &m);
        ret = spi_sync(spi, &m);        
        if (ret < 0) {
            printk("spi_sync fail!\n");
            goto exit;
        }
    }

    /* 接收数据 */
    if (recv_buf && recv_len != 0) {
        t->rx_buf = recv_buf;
        t->len = send_len;
        spi_message_init(&m);
        spi_message_add_tail(t, &m);
        ret = spi_sync(spi, &m);    
        if (ret < 0) {
            printk("spi_sync fail!\n");
            goto exit;
        }
    }

    ret = 0;

    /* 禁止片选 */
exit:
    gpio_set_value(dev->cs_gpio, 1);
    kfree(t);
    return ret;
}

static int icm20608_write_reg(struct icm20608_dev *dev, uint8_t reg, uint8_t dat)
{
    int ret;
    uint8_t send_buf[2];

    send_buf[0] = reg & (~0x80);   // MSB is W(1)
    send_buf[1] = dat;
    ret = icm20608_send_then_recv(dev, send_buf, 2, NULL, 0);

    return ret < 0 ? -1 : 0;
}

static int icm20608_read_reg(struct icm20608_dev *dev, uint8_t reg, uint8_t *dat)
{
    int ret;
    uint8_t send_buf;

    send_buf = reg | 0x80;   // MSB is R(0)
    ret = icm20608_send_then_recv(dev, &send_buf, 1, dat, 1);

    return ret < 0 ? -1 : 0;
}

static int icm20608_board_soft_reset(void)
{
    // reset the internal registers and restore the default settings.
    icm20608_write_reg(&icm20608, 0x6B, 0x80);
    mdelay(50);

    // auto select the best available clock source.
    icm20608_write_reg(&icm20608, 0x6B, 0x01);
    mdelay(50);

    return 0;
}

static int icm20608_board_read_id(uint8_t *id)
{
    int ret;

    ret = icm20608_read_reg(&icm20608, 0x75, id);

    return ret < 0 ? -1 : 0;
}

static int icm20608_board_config(void)
{
    icm20608_write_reg(&icm20608, 0x19, 0x00);  // SMPLRT_DIV
    icm20608_write_reg(&icm20608, 0x1A, 0x04);  // CONFIG
    icm20608_write_reg(&icm20608, 0x1B, 0x18);  // GYRO_CONFIG
    icm20608_write_reg(&icm20608, 0x1C, 0x18);  // ACCEL_CONFIG
    icm20608_write_reg(&icm20608, 0x1D, 0x04);  // ACCEL_CONFIG2
    icm20608_write_reg(&icm20608, 0x1E, 0x00);  // LP_MODE_CFG
    icm20608_write_reg(&icm20608, 0x23, 0x00);  // FIFO_EN
    icm20608_write_reg(&icm20608, 0x6C, 0x00);  // PWR_MGMT_2

    return 0;
}

static int icm20608_board_init(void)
{
    uint8_t id = 0;

    if (icm20608_board_soft_reset() < 0) {
        printk("icm20608_board_soft_reset fail\n!");
        return -1;
    }

    if (icm20608_board_read_id(&id) < 0) {
        printk("icm20608_board_read_id fail\n!");
        return -1;
    }

    if (icm20608_board_config() < 0) {
        printk("icm20608_board_config fail\n!");
        return -1;
    }

    printk("icm20608 id: 0x%x\n", id);

    return 0;
}

static int icm20608_board_read_row_data(struct icm20608_dev *dev)
{
    int i;
    int ret;
    uint8_t reg;
    uint8_t data[14];

    if (!dev) {
        return -1;
    }

    reg = 0x3B;
    for (i = 0; i < 14; i++) {
        ret = icm20608_read_reg(dev, reg++, &data[i]);
        if (ret < 0) {
            break;
        }
    }

    if (i < 14) {
        printk("icm20608_board_read_row_data fail, i = %d!", i);
        return -1;
    }
    
    dev->data.accel_x = (int16_t)((data[0] << 8) | data[1]);
    dev->data.accel_y = (int16_t)((data[2] << 8) | data[3]);
    dev->data.accel_z = (int16_t)((data[4] << 8) | data[5]);
    dev->data.gyro_x = (int16_t)((data[8] << 8) | data[9]);
    dev->data.gyro_x = (int16_t)((data[10] << 8) | data[11]);
    dev->data.gyro_x = (int16_t)((data[12] << 8) | data[13]);
    dev->data.temperature = (int16_t)((data[6] << 8) | data[7]);

    return 0;
}

static int icm20608_open(struct inode *node, struct file *fp)
{
    fp->private_data = &icm20608;
    return 0;
}

static int icm20608_read(struct file *fp, char __user *buf, size_t size, loff_t *off)
{
    int ret;
    long err = 0;
    struct icm20608_dev *dev = (struct icm20608_dev *)fp->private_data;

    ret = icm20608_board_read_row_data(dev);
    if (ret < 0) {
        return -1;
    }

    err = copy_to_user(buf, &dev->data, sizeof(struct icm20608_row_data));

    return 0;
}

static int icm20608_write(struct file *fp, const char __user *buf, size_t size, loff_t *off)
{
    return 0;
}

static int icm20608_release(struct inode *node, struct file *fp)
{
    return 0;
}

static struct file_operations icm20608_fops = {
    .owner = THIS_MODULE,
    .open = icm20608_open,
    .read = icm20608_read,
    .write = icm20608_write,
    .release = icm20608_release,
};

static int icm20608_probe(struct spi_device *spi)
{
    int ret;

    // 申请设备号
    ret = alloc_chrdev_region(&icm20608.dev, 0, 1, "icm20608");
    if (ret != 0) {
        printk("alloc_chrdev_region fail!");
        return -1;
    }

    // 创建cdev
    icm20608.cdev = cdev_alloc();
    if (!icm20608.cdev) {
        printk("cdev_alloc fail!");
        return -1;
    }
    icm20608.cdev->owner = THIS_MODULE;
    icm20608.cdev->ops = &icm20608_fops;

    // 注册cdev
    cdev_add(icm20608.cdev, icm20608.dev, 1);

    // 创建设备类
    icm20608.class = class_create(THIS_MODULE, "icm20608");
    if (!icm20608.class) {
        printk("class_create fail!");
        return -1;
    }

    // 创建设备节点
    icm20608.device = device_create(icm20608.class, NULL, icm20608.dev, NULL, "icm20608");
    if (IS_ERR(icm20608.device)) {
        printk("device_create led_dts_device0 fail!");
        return -1;
    }

    // 获取设备树中spi节点信息
    icm20608.node = of_find_compatible_node(NULL, NULL, "atk,icm20608");
    if (!icm20608.node) {
        printk("icm20608 node find fail!");
        return -1;
    }

    // 进一步获取gpio片选引脚信息
    icm20608.cs_gpio = of_get_named_gpio(icm20608.node, "cs-gpio", 0);
    if (icm20608.cs_gpio < 0) {
        printk("cs-gpio propname in icm20608 node find fail!");
        return -1;
    }
    
    // 设置gpio引脚方向并默认输出高电平
    ret = gpio_direction_output(icm20608.cs_gpio, 1);
    if (ret < 0) {
        printk("gpio_direction_output fail!");
        return -1;
    }

    // 存储spi_device值
    spi->mode = SPI_MODE_0; 
    spi_setup(spi);
    icm20608.spi = spi;

    // board_init
    ret = icm20608_board_init();
    if (ret < 0) {
        printk("icm20608_board_init fail!");
        return -1;
    }
    
    return 0;
}

static int icm20608_remove(struct spi_device *spi)
{
    // 将设备从内核删除
    cdev_del(icm20608.cdev);

    // 释放设备号
    unregister_chrdev_region(icm20608.dev, 1);

    // 删除设备节点
    device_destroy(icm20608.class, icm20608.dev);

    // 删除设备类
    class_destroy(icm20608.class);

    return 0;
}

/* 设备树匹配 */
static const struct of_device_id icm20608_of_match[] = {
    { .compatible = "atk,icm20608" },
    { },
};

/* 传统id方式匹配  */
static const struct spi_device_id icm20608_id[] = {
    { "atk,icm20608", 0 },
    { },
};

/**
 *@brief    spi驱动结构体
*/
static struct spi_driver icm20608_driver = {
    .probe = icm20608_probe,
    .remove = icm20608_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "icm20608",
        .of_match_table = icm20608_of_match,
    },
    .id_table = icm20608_id,
};

static int __init icm20608_module_init(void)
{
    int ret;

    /* 注册spi_driver */
    ret = spi_register_driver(&icm20608_driver);

    if (ret < 0) {
        printk("spi_register_driver fail!\n");
        return -1;
    }

    return 0;
}

static void __exit icm20608_module_exit(void)
{
    /* 注销spi_driver */
    spi_unregister_driver(&icm20608_driver);
}

module_init(icm20608_module_init)
module_exit(icm20608_module_exit)

MODULE_AUTHOR("Mculover666");
MODULE_LICENSE("GPL");
