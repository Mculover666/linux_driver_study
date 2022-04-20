#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    int sec;

    // 检查参数
    if (argc != 2) {
        printf("usage: ./test_second [device]\n");
        return -1;
    }

    fd = open(argv[1], O_RDWR);

    while (1) {
        ret = read(fd, &sec, sizeof(sec));
        if (ret < 0) {
            printf("read sec fail\n");
        }
        printf("sec is %d\n", sec);
        sleep(1);
    }
}