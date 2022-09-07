#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

// Whether to enable display inversion
#define USE_DISPLAY_INVERSION       1

/*
    some useful color, rgb565 format.
    of courcse, you can use rgb2hex_565 API.
*/
#define WHITE         0xFFFF
#define YELLOW        0xFFE0
#define BRRED 		  0XFC07
#define PINK          0XF81F
#define RED           0xF800
#define BROWN 		  0XBC40
#define GRAY  		  0X8430
#define GBLUE		  0X07FF
#define GREEN         0x07E0
#define BLUE          0x001F
#define BLACK         0x0000

struct st7789_lcd_spi {
    struct spi_device *spi;

    int cs_gpio;
    int reset_gpio;
    int dc_gpio;
    int bl_gpio;
    int have_cs_pin;

    int width;
    int height;
    int direction;
};

typedef struct lcd_color_params_st {
    uint16_t    background_color;
    uint16_t    foreground_color;
} lcd_color_params_t;


static struct st7789_lcd_spi lcd;

static lcd_color_params_t s_lcd_color_params = {
    .background_color = WHITE,
    .foreground_color = BLACK
};

static int lcd_blk(int status)
{
    int ret;

    if (status) {
        ret = gpio_direction_output(lcd.bl_gpio, 1);
    } else {
        ret = gpio_direction_output(lcd.bl_gpio, 0);
    }

    return ret == 0 ? 0 : -1;
}

static int lcd_rst(int status)
{
    int ret;

    if (status) {
        ret = gpio_direction_output(lcd.reset_gpio, 1);
    } else {
        ret = gpio_direction_output(lcd.reset_gpio, 0);
    }
    
    return ret == 0 ? 0 : -1;
}

static int lcd_dc(int status)
{
    int ret;

    if (status) {
        ret = gpio_direction_output(lcd.dc_gpio, 1);
    } else {
        ret = gpio_direction_output(lcd.dc_gpio, 0);
    }

    return ret == 0 ? 0 : -1;
}

static int lcd_cs(int status)
{
    int ret;

    if (!lcd.have_cs_pin) {
        return 0;
    }

    if (status) {
        ret = gpio_direction_output(lcd.cs_gpio, 1);
    } else {
        ret = gpio_direction_output(lcd.cs_gpio, 0);
    }

    return ret == 0 ? 0 : -1;
}

static int lcd_write_bytes(uint8_t *data, int size)
{
    int ret;

    struct spi_transfer t = {
        .tx_buf = data,
        .len = size,
    };

    struct spi_message msg;
    spi_message_init(&msg);
    spi_message_add_tail(&t, &msg);
    ret = spi_sync(lcd.spi, &msg);
    if (ret < 0) {
        printk(KERN_ERR "lcd_write_bytes fail, ret is %d\n", ret);
        return -1;
    }
    return 0;
}

static int lcd_write_byte(uint8_t data)
{
    return lcd_write_bytes(&data, 1);
}

static void lcd_hard_reset(void)
{
    lcd_rst(0);
    msleep(100);
    lcd_rst(1);
}

static void lcd_write_cmd(uint8_t cmd)
{
    lcd_dc(0);
    lcd_write_byte(cmd);
}

static void lcd_write_data(uint8_t dat)
{
    lcd_dc(1);
    lcd_write_byte(dat);
}

static void lcd_write_color(const uint16_t color)
{
    lcd_write_byte(color >> 8);
    lcd_write_byte(color);
}

uint16_t rgb2hex_565(uint16_t r, uint16_t g, uint16_t b)
{
    uint16_t color;

    r = (r & 0x1F) << 11;
    g = (g & 0x3F) << 5;
    b = b & 0x1F;
    color = r | g | b;

    return color;
}

static void lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_cs(0);
    lcd_write_cmd(0x2a);
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1);
    lcd_write_data(x2 >> 8);
    lcd_write_data(x2);

    lcd_write_cmd(0x2b);
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1);
    lcd_write_data(y2 >> 8);
    lcd_write_data(y2);

    lcd_write_cmd(0x2C);
    lcd_cs(1);
}

void lcd_display_on(void)
{
    lcd_blk(1);
}

void lcd_display_off(void)
{
    lcd_blk(0);
}

void lcd_color_set(uint16_t back_color, uint16_t fore_color)
{
    s_lcd_color_params.background_color = back_color;
    s_lcd_color_params.foreground_color = fore_color;
}

void lcd_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint32_t size, i;
    uint8_t data[2] = {0};
    uint8_t *mem;

    size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
    data[0] = color >> 8;
    data[1] = color;
    mem = (uint8_t *)kzalloc(size, GFP_KERNEL);
    if (!mem) {
        printk(KERN_ERR "lcd_fill_rect: alloc %d bytes mem fail!\n", size);
    }
    for (i = 0; i < size; i++) {
        mem[i * 2] =  data[0];
        mem[i * 2 + 1] =  data[1];
    }

    printk("lcd_fill_rect: lcd_write_color start!\n");
    lcd_address_set(0, 0, lcd.width - 1, lcd.height - 1);
    lcd_cs(0);
    lcd_dc(1);
    lcd_write_bytes(mem, size);
    lcd_cs(1);
    kfree(mem);
    printk("lcd_fill_rect: lcd_write_color finish!\n");
}

void lcd_clear(void)
{
    uint16_t back_color = s_lcd_color_params.background_color;

    lcd_fill_rect(0, 0, lcd.width -1, lcd.height - 1, back_color);
}

int lcd_init(void)
{
    printk(KERN_INFO "------ st7789 spi lcd init in ------\n");

    lcd_display_on();

    /* LCD Hard Reset */
    lcd_hard_reset();
    msleep(120);
    printk(KERN_INFO "lcd hard reset finish!\n");

    /* Sleep Out */
    lcd_write_cmd(0x11);
    msleep(120);
    printk(KERN_INFO "lcd sleep out finish!\n");

    /* Memory Data Access Control */
    lcd_write_cmd(0x36);
    if (lcd.direction == 0) {
        lcd_write_data(0x00);
    } else if (lcd.direction == 1) {
        lcd_write_data(0x60);
    } else if (lcd.direction == 2) {
        lcd_write_data(0xA0);
    } else if (lcd.direction == 3) {
        lcd_write_data(0xC0);
    } else {
        lcd_write_data(0x00);
    }

    /* RGB 5-6-5-bit  */
    lcd_write_cmd(0x3A);
    lcd_write_data(0x05);

    /* Porch Setting */
    lcd_write_cmd(0xB2);
    lcd_write_data(0x0C);
    lcd_write_data(0x0C);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);

    /*  Gate Control */
    lcd_write_cmd(0xB7);
    lcd_write_data(0x72);

    /* VCOM Setting */
    lcd_write_cmd(0xBB);
    lcd_write_data(0x3D);   //Vcom=1.625V

    /* LCM Control */
    lcd_write_cmd(0xC0);
    lcd_write_data(0x2C);

    /* VDV and VRH Command Enable */
    lcd_write_cmd(0xC2);
    lcd_write_data(0x01);

    /* VRH Set */
    lcd_write_cmd(0xC3);
    lcd_write_data(0x19);

    /* VDV Set */
    lcd_write_cmd(0xC4);
    lcd_write_data(0x20);

    /* Frame Rate Control in Normal Mode */
    lcd_write_cmd(0xC6);
    lcd_write_data(0x01);	//111MHZ

    /* Power Control 1 */
    lcd_write_cmd(0xD0);
    lcd_write_data(0xA4);
    lcd_write_data(0xA1);

    /* Positive Voltage Gamma Control */
    lcd_write_cmd(0xE0);
    lcd_write_data(0xD0);
    lcd_write_data(0x04);
    lcd_write_data(0x0D);
    lcd_write_data(0x11);
    lcd_write_data(0x13);
    lcd_write_data(0x2B);
    lcd_write_data(0x3F);
    lcd_write_data(0x54);
    lcd_write_data(0x4C);
    lcd_write_data(0x18);
    lcd_write_data(0x0D);
    lcd_write_data(0x0B);
    lcd_write_data(0x1F);
    lcd_write_data(0x23);

    /* Negative Voltage Gamma Control */
    lcd_write_cmd(0xE1);
    lcd_write_data(0xD0);
    lcd_write_data(0x04);
    lcd_write_data(0x0C);
    lcd_write_data(0x11);
    lcd_write_data(0x13);
    lcd_write_data(0x2C);
    lcd_write_data(0x3F);
    lcd_write_data(0x44);
    lcd_write_data(0x51);
    lcd_write_data(0x2F);
    lcd_write_data(0x1F);
    lcd_write_data(0x1F);
    lcd_write_data(0x20);
    lcd_write_data(0x23);

#if USE_DISPLAY_INVERSION
    /* Display Inversion On */
    lcd_write_cmd(0x21);
    lcd_write_cmd(0x29);
#else
    /* Display Inversion Off */
    lcd_write_cmd(0x20);
    lcd_write_cmd(0x29);
#endif
    printk(KERN_INFO "lcd init finish!\n");

    lcd_clear();

    printk(KERN_INFO "lcd clear finish!\n");

    printk(KERN_INFO "------ st7789 spi lcd init in ------\n");

    return 0;
}

/*
	spilcd: st7789@1 {
		compatible = "spilcd,st7789";
		spi-max-frequency = <8000000>;
		reg = <1>;
		pinctrl-names = "default";
		pinctrl-0 = <&pinctrl_st7789>;

		//unuse
		//cs-gpio = <&gpio1 0 GPIO_ACTIVE_LOW>;
		reset-gpio = <&gpio1 1 GPIO_ACTIVE_LOW>;
		dc-gpio = <&gpio1 2 GPIO_ACTIVE_LOW>;
		bl-gpio = <&gpio1 4 GPIO_ACTIVE_LOW>;

		width = <240>;
		height = <240>;
        // 0: normal, 1: left 90, 2: 180, 3: right 90.
		direction = <0>;
	};
*/

static int st7789_gpio_parse_dt(struct st7789_lcd_spi *st7789, struct device *dev)
{
    int ret;

    st7789->reset_gpio = of_get_named_gpio(dev->of_node, "reset-gpio", 0);
    if (st7789->reset_gpio < 0) {
        printk(KERN_ERR "reset-gpio find fail!");
        return -1;
    }

    st7789->dc_gpio = of_get_named_gpio(dev->of_node, "dc-gpio", 0);
    if (st7789->dc_gpio < 0) {
        printk(KERN_ERR "dc-gpio find fail!");
        return -1;
    }

    st7789->bl_gpio = of_get_named_gpio(dev->of_node, "bl-gpio", 0);
    if (st7789->bl_gpio < 0) {
        printk(KERN_ERR "bl-gpio find fail!");
        return -1;
    }

    st7789->cs_gpio = of_get_named_gpio(dev->of_node, "cs-gpio", 0);
    if (st7789->cs_gpio < 0) {
        lcd.have_cs_pin = 0;
    } else {
        lcd.have_cs_pin = 1;
    }

    ret = gpio_request(st7789->reset_gpio, "spi-lcd-rst-gpio");
    if (ret < 0) {
        printk("gpio %d request fail!\n", st7789->reset_gpio);
        return -1;
    }

    ret = gpio_request(st7789->dc_gpio, "spi-lcd-dc-gpio");
    if (ret < 0) {
        printk("gpio %d request fail!\n", st7789->dc_gpio);
        return -1;
    }
    ret = gpio_request(st7789->bl_gpio, "spi-lcd-bl-gpio");
    if (ret < 0) {
        printk("gpio %d request fail!\n", st7789->bl_gpio);
        return -1;
    }

    if (lcd.have_cs_pin) {
        ret = gpio_request(st7789->cs_gpio, "spi-lcd-cs-gpio");
        if (ret < 0) {
            printk("gpio %d request fail!\n", st7789->cs_gpio);
            return -1;
        }
    }
    
    printk(KERN_INFO "reset-gpio:%d, dc-gpio:%d, bl-gpio:%d cs-gpio:%d\n", 
        st7789->reset_gpio, st7789->dc_gpio, st7789->bl_gpio, st7789->cs_gpio);

    return 0;
}

static int st7789_params_parse_dt(struct st7789_lcd_spi *st7789, struct device *dev)
{
    if (of_property_read_u32(dev->of_node, "width", &st7789->width) < 0) {
        printk(KERN_ERR "screen width find fail!");
        return -1;
    }

    if (of_property_read_u32(dev->of_node, "height", &st7789->height) < 0) {
        printk(KERN_ERR "screen height find fail!");
        return -1;
    }

    if (of_property_read_u32(dev->of_node, "direction", &st7789->direction) < 0) {
        printk(KERN_ERR "screen height find fail!");
        return -1;
    }

    printk(KERN_INFO "width:%d, height:%d, direction:%d\n", 
        st7789->width, st7789->height, st7789->direction);

    return 0;
}

static int st7789_probe(struct spi_device *spi)
{
    printk(KERN_INFO "------ st7789 spi lcd probe in ------\n");

    if (st7789_gpio_parse_dt(&lcd, &spi->dev) < 0) {
        printk(KERN_ERR "st7789_gpio_parse_dt fail!\n");
        return -1;
    }

    if (st7789_params_parse_dt(&lcd, &spi->dev) < 0) {
        printk(KERN_ERR "st7789_params_parse_dt fail!\n");
        return -1;
    }

    spi->mode = SPI_MODE_3;
    spi_setup(spi);
    lcd.spi = spi;
    if (lcd_init() < 0) {
        printk(KERN_ERR "st7789 lcd init fail!\n");
        return -1;
    }
    
    printk(KERN_INFO "------ st7789 spi lcd probe out ------\n");

    return 0;
}

static int st7789_remove(struct spi_device *spi)
{
    lcd_display_off();

    gpio_free(lcd.bl_gpio);
    gpio_free(lcd.dc_gpio);
    gpio_free(lcd.reset_gpio);

    return 0;
}

static const struct of_device_id st7789_of_match[] = {
    { .compatible = "spilcd,st7789" },
    { },
};

static const struct spi_device_id st7789_id[] = {
    { "spilcd,st7789", 0 },
    { },
};

static struct spi_driver st7789_driver = {
    .probe = st7789_probe,
    .remove = st7789_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "fb_st7789",
        .of_match_table = st7789_of_match,
    },
    .id_table = st7789_id,
};

static int __init st7789_module_init(void)
{
    int ret;

    ret = spi_register_driver(&st7789_driver);
    if (ret < 0) {
        printk("st7789_driver register fail!\n");
        return -1;
    }

    printk("st7789_driver register ok!\n");

    return 0;
}

static void __exit st7789_module_exit(void)
{
    spi_unregister_driver(&st7789_driver);
}

module_init(st7789_module_init)
module_exit(st7789_module_exit)

MODULE_AUTHOR("Mculover666");
MODULE_LICENSE("GPL");
