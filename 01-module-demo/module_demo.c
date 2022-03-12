/**
 * a linux kernel module example.
 * 
 * mculover666
 * 
 * 2022/3/12
*/

#include <linux/init.h>
#include <linux/module.h>

static int __init module_demo_init(void)
{
    printk("module demo init success!\n");
    return 0;
}

static void __exit module_demo_exit(void)
{
    printk("module demo exit success!\n");
}

module_init(module_demo_init);
module_exit(module_demo_exit);

MODULE_AUTHOR("mculover666 <mculover666@qq.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A example Module");

