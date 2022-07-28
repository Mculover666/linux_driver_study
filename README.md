# 源码说明

该源码是基于正点原子 imx6ull ALPHA 开发板学习嵌入式Linux驱动时所编写的源码，相对于正点原子提供的源码，具有更好的代码风格。

# 内核源码

基于 4.1.15 版本内核.

内核地址在：[https://git.code.tencent.com/mculover666/linux-imx6ull](https://git.code.tencent.com/mculover666/linux-imx6ull)。

使用时请选择内核的 4.1.15 分支。

# 编译环境

使用 arm-linux-gnueabihf-gcc 编译器，在 7.5.0 版本下测试通过，其它高版本未测试。

编译时可以直接使用脚本设置环境：
```bash
source build_env.sh
```

# 目录说明

- .vscode：用于配置内核路径，配置后在编写驱动时可以使用VSCode自动提示。
- ko_release：驱动模块发布版本，基于 4.1.15内核，7.5.0编译器。
- xx-xxx：驱动模块源码
- xx-xxx/test：驱动模块对应的测试app

# 教程

- [i.MX6ULL驱动开发 | 01 - Linux内核模块的编写与使用](https://mculover666.blog.csdn.net/article/details/123446346)
- [i.MX6ULL驱动开发 | 02 - 字符设备驱动框架](https://mculover666.blog.csdn.net/article/details/123445472)
- [i.MX6ULL驱动开发 | 03 - 基于字符设备驱动框架点亮LED](https://mculover666.blog.csdn.net/article/details/123478060)
- [i.MX6ULL驱动开发 | 04 - Linux设备树基本语法与实例解析](https://mculover666.blog.csdn.net/article/details/123519917)
- [i.MX6ULL驱动开发 | 05 - 基于设备树点亮LED]()（这篇没有学习的必要，所以没有学习笔记）
- [i.MX6ULL驱动开发 | 06 - pinctrl子系统](https://mculover666.blog.csdn.net/article/details/123832524)
- [i.MX6ULL驱动开发 | 07 - gpio子系统](https://mculover666.blog.csdn.net/article/details/123905748)
- [i.MX6ULL驱动开发 | 08 - 基于pinctrl子系统和gpio子系统点亮LED](https://blog.csdn.net/Mculover666/article/details/123921437)
- [i.MX6ULL驱动开发 | 09 - 基于Linux自带的LED驱动点亮LED](https://mculover666.blog.csdn.net/article/details/123999277)
- [i.MX6ULL驱动开发 | 10 - 修改LCD驱动点亮LCD显示小企鹅logo](https://mculover666.blog.csdn.net/article/details/124017736)
- [i.MX6ULL驱动开发 | 11 - Linux I2C 驱动框架](https://mculover666.blog.csdn.net/article/details/124025447)
- [i.MX6ULL驱动开发 | 12 - 基于 Linux I2C 驱动读取AP3216C传感器](https://mculover666.blog.csdn.net/article/details/124024664)
- [i.MX6ULL驱动开发 | 13 - Linux SPI 驱动框架](https://mculover666.blog.csdn.net/article/details/124098484)
- [i.MX6ULL驱动开发 | 14 - 基于 Linux SPI 驱动框架读取ICM-20608传感器](https://mculover666.blog.csdn.net/article/details/124100085)
- [i.MX6ULL驱动开发 | 15 - Linux UART 驱动框架](https://mculover666.blog.csdn.net/article/details/124143195)
- [i.MX6ULL驱动开发 | 16 - 基于 UART 驱动框架发送/接收串口数据](https://mculover666.blog.csdn.net/article/details/124147474)
- [i.MX6ULL驱动开发 | 17 - Linux中断机制及使用方法（tasklet、workqueue、软中断）](https://blog.csdn.net/Mculover666/article/details/124244171)
- [i.MX6ULL驱动开发 | 18 - 使用中断方式检测按键](https://blog.csdn.net/Mculover666/article/details/124267028)
- [i.MX6ULL驱动开发 | 19 - Linux内核定时器的编程方法与使用示例](https://blog.csdn.net/Mculover666/article/details/124297680)
- [i.MX6ULL驱动开发 | 20 - Linux input 子系统](https://blog.csdn.net/Mculover666/article/details/124322791)
- [i.MX6ULL驱动开发 | 21 - 按键驱动使用 input 子系统上报事件](https://mculover666.blog.csdn.net/article/details/124338588)
- [i.MX6ULL驱动开发 | 22 - 使用PCF8574扩展gpio](https://mculover666.blog.csdn.net/article/details/124457915)
- [i.MX6ULL驱动开发 | 24 - 基于platform平台驱动模型点亮LED](https://blog.csdn.net/Mculover666/article/details/125576294)
- [i.MX6ULL驱动开发 | 25 - 基于Linux自带的KEY驱动检测按键](https://blog.csdn.net/Mculover666/article/details/125581427)
- [i.MX6ULL驱动开发 | 26 - Linux内核的RTC驱动](https://blog.csdn.net/Mculover666/article/details/125583949)
- [i.MX6ULL驱动开发 | 27 - 使用WM8960 CODEC播放音频](https://blog.csdn.net/Mculover666/article/details/125665589)
- [i.MX6ULL驱动开发 | 28 - 使用FT5426多点电容触摸](https://mculover666.blog.csdn.net/article/details/125689736)
- [i.MX6ULL驱动开发 | 29 - 使用USB WIFI网卡（RTL8188EU）](https://blog.csdn.net/Mculover666/article/details/125710392)
- [i.MX6ULL驱动开发 | 30 - 使用EC20 4G网卡（移植移远GobiNet驱动）](https://mculover666.blog.csdn.net/article/details/125740257)
- [i.MX6ULL驱动开发 | 31 - Linux内核网络设备驱动框架](https://mculover666.blog.csdn.net/article/details/125993944)
- [i.MX6ULL驱动开发 | 32 - 手动编写一个虚拟网卡设备](https://blog.csdn.net/Mculover666/article/details/126003032)