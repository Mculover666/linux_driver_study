#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    int food;

    // 打开看门狗
    fd = open("/dev/watchdog", O_WRONLY);
    if (fd < 0) {
        printf("open watchdog fail!\n");
    } else {
        printf("open watchdog success!\n");
    }

    // 定时喂狗
    while (1) {
        ret = write(fd, &food, sizeof(food));
        if (ret < 0) {
            printf("feed watch dog fail!\n");
        } else {
            printf("feed dog!\n");
        }
        sleep(10);
    }

    // 一旦此程序停止，看门狗没喂到，1min之后系统就会自动重启
}