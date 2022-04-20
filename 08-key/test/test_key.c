#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    int keyval;

    // 检查参数
    if (argc != 2) {
        printf("usage: ./test_key [device]\n");
        return -1;
    }

    fd = open(argv[1], O_RDWR);

    while (1) {
        ret = read(fd, &keyval, sizeof(keyval));
        if (ret < 0) {
            // 键值无效
        } else {
            if (keyval == 0x01) {
                printf("key 0 press!\n");
            }
        }
    }
}