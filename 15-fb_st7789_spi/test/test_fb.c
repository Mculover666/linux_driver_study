#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
    int fd = 0;
    char *fd_map = NULL;
    struct fb_fix_screeninfo fix_info;
    struct fb_var_screeninfo var_info;
    int ret = -1;
    
    memset(&fix_info, 0, sizeof(struct fb_fix_screeninfo));
    memset(&var_info, 0, sizeof(struct fb_var_screeninfo));

    if (argc != 2) {
        printf("usage: ./test_fb [device]\n");
        return -1;
    }
    
    // 打开framebuffer节点
    fd = open(argv[1], O_RDWR);
    if(fd < 0)
    {
        char *error_msg = strerror(errno);
        printf("Open %s failed, errno:%d, error message:%s\n", 
            argv[1], errno, error_msg);
        return -1;
    }

    // 通过IOCTL获取可变参数结构体内容
    ret = ioctl(fd, FBIOGET_VSCREENINFO, &var_info);
    if(ret < 0)
    {
        char *error_msg = strerror(errno);
        printf("Get %s var info error, errno:%d, error message:%s\n", 
            argv[1], errno, error_msg);
        return ret;
    }

    // 通过IOCTL获取固定参数结构体内容
    ret = ioctl(fd, FBIOGET_FSCREENINFO, &fix_info);
    if(ret < 0)
    {
        char *error_msg = strerror(errno);
        printf("Get %s fix info error, errno:%d, error message:%s\n", 
            argv[1], errno, error_msg);
        return ret;
    }

    printf("%s var info, xres=%d, yres=%d\n", argv[1], var_info.xres, var_info.yres);
    printf("%s var info, xres_virtual=%d, yres_virtual=%d\n", 
        argv[1], var_info.xres_virtual, var_info.yres_virtual);
    printf("%s var info, bits_per_pixel=%d\n", argv[1], var_info.bits_per_pixel);
    printf("%s var info, xoffset=%d, yoffset=%d\n", 
        argv[1], var_info.xoffset, var_info.yoffset);

    int screen_size = var_info.xres * var_info.yres * var_info.bits_per_pixel / 8;

    // 将Framebuffer映射到进程的内存空间
    fd_map = (char *)mmap(NULL, screen_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(fd_map == (char *)-1)
    {
        char *error_msg = strerror(errno);
        printf("Mmap %s failed, errno:%d, error message:%s\n", 
            argv[1], errno, error_msg);
        return -1;
    }
    
    // 将内容写到进程内存空间，Framebuffer驱动会同步刷新到显示屏上
    for(unsigned int i = 0; i < var_info.yres / 3; i++)
    {
        unsigned int argb = 0;
        argb = argb | (0xff << 8);
        for(int j = 0; j < var_info.xres; j++)
            *((unsigned int *)(fd_map + j * 4 + i * var_info.xres * 4)) = argb;
    }

    for(unsigned int i = var_info.yres/3; i < 2 * var_info.yres / 3; i++)
    {
        unsigned int argb = 0;
        argb = argb | (0xff);
        for(int j = 0; j < var_info.xres; j++)
            *((unsigned int *)(fd_map + j * 4 + i * var_info.xres * 4)) = argb;
    }

    munmap(fd_map, screen_size);
    close(fd);
    return 0;
}