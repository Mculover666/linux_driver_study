#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/input.h>
#include <asm/bitops.h>

struct key_dev {
    struct device_node *node;   /*!< 设备树节点         */
    int gpio;                   /*!< key使用的gpio编号  */
    int debounce_interval;      /*!< 去抖时长           */
    struct timer_list  timer;   /*!< 用于软件消抖        */
    struct input_dev   *input;  /*!< 输入设备           */

    int irq;                    /*!< 中断号             */
};

static struct key_dev key;

static irqreturn_t key0_handler(int irq, void *dev_id)
{
    struct key_dev *dev = (struct key_dev *)dev_id;

    // 启动软件定时器, 消抖时间后再去读取
    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->debounce_interval));
    
    return IRQ_RETVAL(IRQ_HANDLED);
}

static void timer_handler(unsigned long arg)
{
    int val;
    struct key_dev *dev = (struct key_dev *)arg;
    static int i = 0;

    // 上报输入事件
    val = gpio_get_value(dev->gpio);
    printk("--->report, i = %d\n", i++);
    if (val == 0) {
        input_report_key(dev->input, KEY_0, 1);
        input_sync(dev->input);
        input_report_key(dev->input, KEY_0, 0);
        input_sync(dev->input);
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
       
    // 设置软件定时器用于消抖
    init_timer(&key.timer);
    key.timer.function = timer_handler;
    key.debounce_interval = debounce_interval;

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

    // 分配输入设备
    key.input = input_allocate_device();
    if (!key.input) {
        printk("input_allocate_device fail!\n");
        goto error;
    }

    // 初始化输入设备
    key.input->name = "keyinput";
    set_bit(EV_KEY, key.input->evbit);  // 按键事件
    set_bit(EV_REP, key.input->evbit);  // 重复事件
    set_bit(KEY_0, key.input->keybit);  // 设置产生哪些按键 

    // 注册输入设备
    ret = input_register_device(key.input);
    if (ret < 0) {
        printk("input_register_device fail!\n");
        goto error;
    }   

    return 0;

error:
    gpio_free(key.gpio);
    return -1;
}

static void key_deinit(void)
{
    // 注销输入设备
    input_unregister_device(key.input);

    // 释放输入设备
    input_free_device(key.input);

    // 释放中断
    free_irq(key.irq, &key);

    // 释放gpio
    gpio_free(key.gpio);

    // 删除定时器
    del_timer(&key.timer);
}

static int __init key_module_init(void)
{
    return key_init();
}

static void __exit key_module_exit(void)
{
    key_deinit();
}

module_init(key_module_init);
module_exit(key_module_exit);

MODULE_AUTHOR("Mculover666");
MODULE_LICENSE("GPL");