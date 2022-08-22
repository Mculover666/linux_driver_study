
/*
*	WKIC Ltd.
*	WK2xxx.c
*	wk2xxx_GPIO_I2C DEMO Ver

ion :1.0 Data:2018-08-08
*	By  xuxunwei Tech 
*
*/
#ifndef	_SERIAL_WK2XXX_H       //_SERIAL_WK2XXX_H
#define  _SERIAL_WK2XXX_H
	
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/console.h>
#include <asm/irq.h>



/*****************************
***wkxxxx  Global register address defines
******************************/
#define 	WK2XXX_GENA     0X00
#define 	WK2XXX_GRST     0X01
#define		WK2XXX_GMUT     0X02
#define 	WK2XXX_GIER     0X10
#define 	WK2XXX_GIFR     0X11
#define 	WK2XXX_GPDIR    0X21
#define 	WK2XXX_GPDAT    0X31


/*****************************
*wkxxxx  slave uarts  register address defines
******************************/
#define 	WK2XXX_SPAGE    0X03
#define 	WK2XXX_PAGE1    1
#define 	WK2XXX_PAGE0    0

/*PAGE0*/
#define 	WK2XXX_SCR      0X04
#define 	WK2XXX_LCR      0X05
#define 	WK2XXX_FCR      0X06
#define 	WK2XXX_SIER     0X07
#define 	WK2XXX_SIFR     0X08
#define 	WK2XXX_TFCNT    0X09
#define 	WK2XXX_RFCNT    0X0A
#define 	WK2XXX_FSR      0X0B
#define 	WK2XXX_LSR      0X0C
#define 	WK2XXX_FDAT     0X0D
#define 	WK2XXX_FWCR     0X0E
#define 	WK2XXX_RS485    0X0F
/*PAGE1*/
#define 	WK2XXX_BAUD1    0X04
#define 	WK2XXX_BAUD0    0X05
#define 	WK2XXX_PRES     0X06
#define 	WK2XXX_RFTL     0X07
#define 	WK2XXX_TFTL     0X08
#define 	WK2XXX_FWTH     0X09
#define 	WK2XXX_FWTL     0X0A
#define 	WK2XXX_XON1     0X0B
#define 	WK2XXX_XOFF1    0X0C
#define 	WK2XXX_SADR     0X0D
#define 	WK2XXX_SAEN     0X0E
#define 	WK2XXX_RRSDLY   0X0F


//wkxxx register bit defines
/*GENA register*/
#define 	WK2XXX_UT4EN	  0x08
#define 	WK2XXX_UT3EN	  0x04
#define 	WK2XXX_UT2EN	  0x02
#define 	WK2XXX_UT1EN	  0x01
/*GRST register*/
#define 	WK2XXX_UT4SLEEP	0x80
#define 	WK2XXX_UT3SLEEP	0x40
#define 	WK2XXX_UT2SLEEP	0x20
#define 	WK2XXX_UT1SLEEP	0x10
#define 	WK2XXX_UT4RST	    0x08
#define 	WK2XXX_UT3RST		0x04
#define 	WK2XXX_UT2RST		0x02
#define 	WK2XXX_UT1RST		0x01
/*GIER register*/
#define 	WK2XXX_UT4IE		0x08
#define 	WK2XXX_UT3IE		0x04
#define 	WK2XXX_UT2IE		0x02
#define 	WK2XXX_UT1IE		0x01
/*GIFR register*/
#define 	WK2XXX_UT4INT		0x08
#define 	WK2XXX_UT3INT		0x04
#define 	WK2XXX_UT2INT		0x02
#define 	WK2XXX_UT1INT		0x01
/*SPAGE register*/
#define 	WK2XXX_SPAGE0	    0x00
#define 	WK2XXX_SPAGE1     0x01
/*SCR register*/
#define 	WK2XXX_SLEEPEN    0x04
#define 	WK2XXX_TXEN       0x02
#define 	WK2XXX_RXEN       0x01
/*LCR register*/
#define 	WK2XXX_BREAK	  0x20
#define 	WK2XXX_IREN     0x10
#define 	WK2XXX_PAEN     0x08
#define 	WK2XXX_PAM1     0x04
#define 	WK2XXX_PAM0     0x02
#define 	WK2XXX_STPL     0x01
/*FCR register*/
#define 	WK2XXX_TFEN     0x08
#define 	WK2XXX_RFEN     0x04
#define 	WK2XXX_TFRST    0x02
#define 	WK2XXX_RFRST    0x01
/*SIER register*/
#define 	WK2XXX_FERR_IEN      0x80
#define 	WK2XXX_CTS_IEN       0x40
#define 	WK2XXX_RTS_IEN       0x20
#define 	WK2XXX_XOFF_IEN      0x10
#define 	WK2XXX_TFEMPTY_IEN   0x08
#define 	WK2XXX_TFTRIG_IEN    0x04
#define 	WK2XXX_RXOUT_IEN     0x02
#define 	WK2XXX_RFTRIG_IEN    0x01
/*SIFR register*/
#define 	WK2XXX_FERR_INT      0x80
#define 	WK2XXX_CTS_INT       0x40
#define 	WK2XXX_RTS_INT       0x20
#define 	WK2XXX_XOFF_INT      0x10
#define 	WK2XXX_TFEMPTY_INT   0x08
#define 	WK2XXX_TFTRIG_INT    0x04
#define 	WK2XXX_RXOVT_INT     0x02
#define 	WK2XXX_RFTRIG_INT    0x01

/*TFCNT register*/
/*RFCNT register*/
/*FSR register*/
#define 	WK2XXX_RFOE      0x80
#define 	WK2XXX_RFBI      0x40
#define 	WK2XXX_RFFE      0x20
#define 	WK2XXX_RFPE      0x10
#define 	WK2XXX_RDAT      0x08
#define 	WK2XXX_TDAT      0x04
#define 	WK2XXX_TFULL     0x02
#define 	WK2XXX_TBUSY     0x01
/*LSR register*/
#define 	WK2XXX_OE        0x08
#define 	WK2XXX_BI        0x04
#define 	WK2XXX_FE        0x02
#define 	WK2XXX_PE        0x01
/*FWCR register*/
#define 	WK2XXX_RTS       0x02
#define 	WK2XXX_CTS       0x01
/*RS485 register*/
#define 	WK2XXX_RSRS485   0x40
#define 	WK2XXX_ATADD     0x20
#define 	WK2XXX_DATEN     0x10
#define 	WK2XXX_RTSEN     0x02
#define 	WK2XXX_RTSINV    0x01



//
#define 	NR_PORTS 	4

//
#define 	SERIAL_WK2XXX_MAJOR	    	207
#define 	CALLOUT_WK2XXX_MAJOR		  208	
#define 	MINOR_START		            5
//wk2xxx hardware configuration
#define		wk2xxx_spi_speed	10000000
#define 	IRQ_WK2XXX		GPIO7_A2
#define 	WK_CRASTAL_CLK		(3686400*2)
#define     MAX_WK2XXX           	4
#define 	WK2XXX_ISR_PASS_LIMIT	50
#define		PORT_WK2XXX            1
#endif

