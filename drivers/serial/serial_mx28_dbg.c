/*
 * (C) Copyright 2020
 * weiming, AutoIO. weiming@autoio.cn.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* Simple U-Boot driver for the i.MX28 DEBUG UARTs */

#include <common.h>
#include <errno.h>
#include <watchdog.h>
#include <asm/io.h>
#include <serial.h>
#include <serial_mx28_dbg.h>
#include <linux/compiler.h>


DECLARE_GLOBAL_DATA_PTR;


#define REGS_UARTDBG_BASE 		(0x80074000)
#define HW_UARTDBGCR 			(0x00000030)
#define HW_UARTDBGFBRD 			(0x00000028)
#define HW_UARTDBGIBRD 			(0x00000024)
#define HW_UARTDBGLCR_H			(0x0000002C)
#define BM_UARTDBGLCR_H_WLEN	(0x00000060)
#define BM_UARTDBGLCR_H_FEN		(0x00000010)
#define HW_UARTDBGIMSC 			(0x00000038)
#define HW_UARTDBGFR			(0x00000018)
#define BM_UARTDBGFR_TXFF		(0x00000020)
#define HW_UARTDBGDR			(0x00000000)
#define BM_UARTDBGFR_RXFE		(0x00000010)
#define BM_UARTDBG_TXE			(0x00000100)
#define BM_UARTDBG_RXE			(0x00000200)
#define BM_UARTDBGCR_UARTEN		(0x00000001)



#define REG_RD(base, reg) \
		(*(volatile unsigned int *)((base) + (reg)))
		
#define REG_WR(base, reg, value) \
		((*(volatile unsigned int *)((base) + (reg))) = (value))
		
#define REG_SET(base, reg, value) \
		((*(volatile unsigned int *)((base) + (reg  ## _SET))) = (value))

#define REG_CLR(base, reg, value) \
		((*(volatile unsigned int *)((base) + (reg  ## _CLR))) = (value))

#define REG_TOG(base, reg, value) \
		((*(volatile unsigned int *)((base) + (reg  ## _TOG))) = (value))


/* 设置一些串口参数 */
void mx28_dbg_uart_setbrg(void)
{
	u32 cr;
	u32 quot;

	cr = REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGCR);
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, 0);

	quot = (CONFIG_MX28_UARTDBG_CLOCK * 4) / gd->baudrate;
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGFBRD, quot & 0x3f);	
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGIBRD, quot >> 6);

	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGLCR_H, \
		BM_UARTDBGLCR_H_WLEN | BM_UARTDBGLCR_H_FEN);

	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, cr);
}

/* 初始化串口 */
int mx28_dbg_uart_init(void)
{
	/* 重新设置一下IO引脚? 这里可以设置也可以不设置
		因为在mx28evk.c里面已经设置过了 
	*/
	
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, 0);
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGIMSC, 0);

	mx28_dbg_uart_setbrg();

	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, \
		BM_UARTDBG_TXE | BM_UARTDBG_RXE |BM_UARTDBGCR_UARTEN);
	
	return 0;
}

void mx28_dbg_uart_putc(const char c)
{
	while (REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGFR) & BM_UARTDBGFR_TXFF)
		;
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGDR, c);

	if (c == '\n')
		serial_putc('\r');
}

void mx28_dbg_uart_puts(const char *s)
{
	while(*s)
		serial_putc(*s++);
}

int mx28_dbg_uart_tstc(void)
{
	return !(REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGFR) & BM_UARTDBGFR_RXFE);
}

int mx28_dbg_uart_getc(void)
{
	while (REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGFR) & BM_UARTDBGFR_RXFE)
		;
	return REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGDR) & 0xff;
}


static struct serial_device mx28dbg_serial_drv = {
	.name	= "mx28_dbg_serial",
	.start	= mx28_dbg_uart_init,
	.stop	= NULL,
	.setbrg	= mx28_dbg_uart_setbrg,
	.putc	= mx28_dbg_uart_putc,
	.puts	= mx28_dbg_uart_puts,
	.getc	= mx28_dbg_uart_getc,
	.tstc	= mx28_dbg_uart_tstc,
};

void mx28dbg_serial_initialize(void)
{
	/* 注册一个串口设备 */
	serial_register(&mx28dbg_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
	return &mx28dbg_serial_drv;
}

