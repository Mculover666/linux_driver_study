#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <linux/input.h>
#include <string.h>

static struct input_event event;

int main(int argc, char *argv[])
{
    int fd;
    int ret;

    if (argc != 2) {
        printf("usage: ./test_input_key [device]\n");
        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        printf("open file %s fail!", argv[1]);
        return -1;
    }

    while (1) {
        memset(&event, 0, sizeof(event));
        ret = read(fd, &event, sizeof(event));
        switch (event.type) {
            case EV_SYN:
                break;
            case EV_KEY:
                if (event.code < BTN_MISC) {    // 键盘键值
                    printf("key %d %s\n", event.code, event.value ? "press" : "release");
                } else {
                    printf("btn %d %s\n", event.code, event.value ? "press" : "release");
                }
                break;
            case EV_REL:
                break;
            case EV_ABS:
                break;
            case EV_MSC:
                break;
            case EV_SW:
                break;
        }
    }
}