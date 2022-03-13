#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int fd;
    char *filename = NULL;
    int tmp;

    // 检查参数
    if (argc != 2) {
        printf("usage: ./test_hello_drv [device]\n");
    }

    // 打开设备文件
    filename = argv[1];
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        printf("open %s error!\n", filename);
        return 0;
    }

    // 读文件
    read(fd, &tmp, 4);

    // 写文件
    write(fd, &tmp, 4);

    // 关闭文件
    close(fd);

    return 0;
}