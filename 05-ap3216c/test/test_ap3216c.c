#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int ap3216_read_data(char *filename)
{
    int fd;
    int ret;
    unsigned char data_buf[3];

    if (!filename) {
        return -1;
    }

    // 打开设备文件
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        printf("open %s error!\n", filename);
        return 0;
    }

    // 读文件
    ret = read(fd, data_buf, 3);
    if (ret != 0) {
        printf("read fail!\n");
    } else {
        printf("ir:%d als: %d ps: %d\n", data_buf[0], data_buf[1], data_buf[2]);
    }

    // 关闭文件
    close(fd);

    return 0;
}

int main(int argc, char *argv[])
{
    uint32_t interval;

    // 检查参数
    if (argc != 3) {
        printf("usage: ./test_ap3216c [device] [read interval(s)]\n");
        return -1;
    }

    interval = atoi(argv[2]);
    if (interval < 1) {
        interval = 1;
    }

    while (1) {
        ap3216_read_data(argv[1]);
        sleep(interval);
    }
}