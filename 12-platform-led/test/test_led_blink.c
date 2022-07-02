#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int led_ctrl(char *filename, int status)
{
    int fd;
    int ret;
    unsigned char data_buf[1];

    if (!filename) {
        return -1;
    }

    // 打开设备文件
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        printf("open %s error!\n", filename);
        return 0;
    }

    // 写文件
    data_buf[0] = status;

    ret = write(fd, data_buf, sizeof(data_buf));
    if (ret < 0) {
        printf("write %s error!\n", data_buf);
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
        printf("usage: ./test_led_blink [device] [led blink interval(s)]\n");
        return -1;
    }

    interval = atoi(argv[2]);
    if (interval < 1) {
        interval = 1;
    }

    while (1) {
        led_ctrl(argv[1], 1);
        sleep(interval);

        led_ctrl(argv[1], 0);
        sleep(interval);
    }
}