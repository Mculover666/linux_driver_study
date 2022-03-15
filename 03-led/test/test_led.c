#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    char *filename = NULL;
    unsigned char data_buf[1];

    // 检查参数
    if (argc != 3) {
        printf("usage: ./test_led [device] [led status]\n");
    }

    // 打开设备文件
    filename = argv[1];
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        printf("open %s error!\n", filename);
        return 0;
    }

    // 写文件
    data_buf[0] = atoi(argv[2]);

    ret = write(fd, data_buf, sizeof(data_buf));
    if (ret < 0) {
        printf("write %s error!\n", data_buf);
    }

    // 关闭文件
    close(fd);

    return 0;
}