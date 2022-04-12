#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

struct icm20608_row_data {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temperature;
};

struct icm20608_row_data data;

int icm20608_read_data(char *filename)
{
    int fd;
    int ret;
    float gyro_x_act, gyro_y_act, gyro_z_act;
    float accel_x_act, accel_y_act, accel_z_act;
    float temp_act;

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
    ret = read(fd, &data, sizeof(data));
    if (ret != 0) {
        printf("read fail!\n");
        close(fd);
        return -1;
    } else {
        printf("accel x:%d, y:%d, z:%d\n", data.accel_x, data.accel_y, data.accel_z);
        printf("gyro x:%d, y:%d, z:%d\n", data.gyro_x, data.gyro_y, data.gyro_z);
        printf("temperature: %d\n", data.temperature);
    }

    // 计算实际值
    gyro_x_act = (float)(data.gyro_x) / 16.4;
    gyro_y_act = (float)(data.gyro_y) / 16.4;
    gyro_z_act = (float)(data.gyro_z) / 16.4;
    accel_x_act = (float)(data.accel_x) / 2048;
    accel_y_act = (float)(data.accel_y) / 2048;
    accel_z_act = (float)(data.accel_z) / 2048;
    temp_act = ((float)(data.temperature) - 25 ) / 326.8 + 25;

    printf("act accel x:%.2f, y:%.2f, z:%.2f\n", accel_x_act, accel_y_act, accel_z_act);
    printf("act gyro x:%.2f, y:%.2f, z:%.2f\n", gyro_x_act, gyro_y_act, gyro_z_act);
    printf("act temperature: %.2f\n", temp_act);
    
    // 关闭文件
    close(fd);

    return 0;
}

int main(int argc, char *argv[])
{
    uint32_t interval;

    // 检查参数
    if (argc != 3) {
        printf("usage: ./test_icm20608 [device] [read interval(s)]\n");
        return -1;
    }

    interval = atoi(argv[2]);
    if (interval < 1) {
        interval = 1;
    }

    while (1) {
        icm20608_read_data(argv[1]);
        sleep(interval);
    }
}