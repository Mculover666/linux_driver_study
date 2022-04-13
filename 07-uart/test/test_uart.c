#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
#include <string.h>

int uart_setup(int fd)
{
    struct termios newtio;

    // 获取原有串口配置
    if  (tcgetattr(fd, &newtio) < 0) {
        return -1;
    }

    // 修改控制模式，保证程序不会占用串口
    newtio.c_cflag  |=  CLOCAL;

    // 修改控制模式，能够从串口读取数据
    newtio.c_cflag  |=  CREAD;

    // 不使用流控制
    newtio.c_cflag &= ~CRTSCTS;

    // 设置数据位
    newtio.c_cflag &= ~CSIZE;
    newtio.c_cflag |= CS8;

    // 设置奇偶校验位
    newtio.c_cflag &= ~PARENB;
    newtio.c_iflag &= ~INPCK; 

    // 设置停止位
    newtio.c_cflag &= ~CSTOPB;

    // 设置最少字符和等待时间
    newtio.c_cc[VTIME]  = 1;
    newtio.c_cc[VMIN] = 1;

    // 修改输出模式，原始数据输出
    newtio.c_oflag &= ~OPOST;
    newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // 设置波特率
    cfsetispeed(&newtio, B115200); 
    cfsetospeed(&newtio, B115200);

    // 清空终端未完成的数据
    tcflush(fd,TCIFLUSH);

    // 设置新属性
    if(tcsetattr(fd, TCSANOW, &newtio) < 0) {
        return -1;
    }

    return 0;
}

int uart_send(int fd, char *buf, int len)
{
    int count;

    count = write(fd, buf, len);

    return count == len ? len : -1;
}

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    int count = 100;
    char send_buf[] = "Hello World!\r\n";

    if (argc != 2) {
        printf("usage: ./test_uart [device]\n");
        return -1;
    }

    /* 打开串口 */
    fd = open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        printf("open dev fail!\n");
        return -1;
    }

    /* 设置串口 */
    ret = uart_setup(fd);
    if (ret < 0) {
        printf("uart setup fail!\n");
        close(fd);
        return -1;
    }

    while (count--) {
        ret = uart_send(fd, send_buf, strlen(send_buf));
        if (ret < 0) {
            printf("send fail!\n");
        } else {
            printf("send ok!\n");
        }
        sleep(2);
    }

    close(fd);
}