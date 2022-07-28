#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/etherdevice.h>

static struct net_device *virt_eth_dev;

//伪造一个收的ping函数
static void virt_rs_pack(struct sk_buff *skb, struct net_device *dev)
{
    struct ethhdr *eh;
    struct iphdr *ih;
    unsigned char tmp_dev_addr[ETH_ALEN];
    __be32 *saddr, *daddr, tmp;
    struct sk_buff *rx_skb;
    unsigned char *type;

    /* 对调ethhdr结构体 源/目的 地址 */
    eh = (struct ethhdr *)skb->data;
    memcpy(tmp_dev_addr, eh->h_dest, ETH_ALEN);
    memcpy(eh->h_dest, eh->h_source, ETH_ALEN);
    memcpy(eh->h_source, tmp_dev_addr, ETH_ALEN);

    /* 对调iphdr结构体 源/目的 地址 */
    ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
    saddr = &(ih->saddr);
    daddr = &(ih->daddr);
    tmp = *saddr;
    *saddr = *daddr;
    *daddr = tmp;

    /* 重新获取iphdr结构体的校验码 */
    ih->check = 0;
    ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);

    /* 设置数据类型 */
    type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);
    *type = 0;      //之前是发送ping包0x08,需要改为0x00,表示接收ping包

    /* 构造一个新的sk_buff */
    rx_skb = dev_alloc_skb(skb->len + 2);

    /* 腾出2字节头部空间 */
    skb_reserve(rx_skb, 2);

    /* 拷贝之前修改好的sk_buff */
    //note: skb_put():来动态扩大sk_buff结构体里中的数据区，避免溢出.
    memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);

    /* 设置新的sk_buff其它成员 */
    rx_skb->dev = dev;
    rx_skb->ip_summed = CHECKSUM_UNNECESSARY;

    /* 获取上层协议 */
    rx_skb->protocol = eth_type_trans(rx_skb, dev);

    /* 更新接收统计信息 */
    dev->stats.rx_packets++;
    dev->stats.rx_bytes += skb->len;
    dev->last_rx= jiffies;

    /* 把数据包交给上层协议 */
    netif_rx(rx_skb);
}

static int virt_eth_open(struct net_device *dev)
{
    /* 激活数据传输队列 */
    netif_start_queue(dev);
    return 0;
}

static int virt_eth_stop(struct net_device *dev)
{
    /* 停止数据传输队列 */
    netif_stop_queue(dev);
    return 0;
}

static netdev_tx_t virt_eth_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    /* 阻止上层向网络设备驱动层发送数据包 */
    netif_stop_queue(dev);

    /* 调用收包函数 */
    virt_rs_pack(skb, dev);

    /* 释放发送的sk_buff缓冲区 */
    dev_kfree_skb(skb);

    /* 更新发送的统计信息 */
    dev->stats.tx_packets++;
    dev->stats.tx_bytes += skb->len;
    dev->trans_start = jiffies;

    /* 唤醒被阻塞的上层 */
    netif_wake_queue(dev);

    return NETDEV_TX_OK;
}

static const struct net_device_ops virt_eth_dev_ops = {
    .ndo_init = NULL,
    .ndo_uninit = NULL,
    .ndo_open = virt_eth_open,
    .ndo_stop = virt_eth_stop,
    .ndo_start_xmit = virt_eth_start_xmit,
};

static int virt_eth_register(void)
{
    int rc;

    /* 分配net_device结构体 */
    virt_eth_dev = alloc_netdev(sizeof(struct net_device), "virt_eth0", NET_NAME_UNKNOWN, ether_setup);
    if (!virt_eth_dev) {
        printk("alloc_netdev fail!\r\n");
        return -1;
    }

    /* 设置net_device结构体成员 */
    virt_eth_dev->dev_addr[0] = 0x08;
    virt_eth_dev->dev_addr[1] = 0x89;
    virt_eth_dev->dev_addr[2] = 0x89;
    virt_eth_dev->dev_addr[3] = 0x89;
    virt_eth_dev->dev_addr[4] = 0x89;
    virt_eth_dev->dev_addr[5] = 0x89;

    virt_eth_dev->netdev_ops = &virt_eth_dev_ops;

    virt_eth_dev->flags    |= IFF_NOARP;
    virt_eth_dev->features |= NETIF_F_HW_CSUM;

    /* 注册net_device结构体 */
    rc = register_netdev(virt_eth_dev);
    if (rc) {
        printk("register_netdev error!\r\n");
        free_netdev(virt_eth_dev);
        return -1;
    }

    return 0;
}

static void virt_eth_unregister(void)
{
    /* 注销net_device结构体 */
    unregister_netdev(virt_eth_dev);

    /* 释放net_device结构体 */
    free_netdev(virt_eth_dev);
}

static int __init virt_eth_module_init(void)
{
    virt_eth_register();
    return 0;
}

static void __exit virt_eth_module_exit(void)
{
    virt_eth_unregister();
    return;
}

module_init(virt_eth_module_init);
module_exit(virt_eth_module_exit);

MODULE_AUTHOR("Mculover666");
MODULE_LICENSE("GPL");