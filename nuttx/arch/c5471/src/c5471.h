/************************************************************
 * c5471.h
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************/

#ifndef __C5471_H
#define __C5471_H

/************************************************************
 * Included Files
 ************************************************************/

#include <nuttx/config.h>

#ifndef __ASSEMBLY__
# include <sys/types.h>
#endif

#if defined(CONFIG_ARCH_BOARD_C5471EVM)
# include <arch/board/c5471evm.h>
#else
# warning "Undefined C5471 Board"
#endif

/************************************************************
 * Definitions
 ************************************************************/

/* Arm7Tdmi *************************************************/

/* CPSR bits */

#define USR26_MODE             0x00
#define FIQ26_MODE             0x01
#define IRQ26_MODE             0x02
#define SVC26_MODE             0x03
#define USR_MODE               0x10
#define FIQ_MODE               0x11
#define IRQ_MODE               0x12
#define SVC_MODE               0x13
#define ABT_MODE               0x17
#define UND_MODE               0x1b
#define SYSTEM_MODE            0x1f
#define MODE_MASK              0x1f
#define T_BIT                  0x20
#define F_BIT                  0x40
#define I_BIT                  0x80
#define CC_V_BIT               (1 << 28)
#define CC_C_BIT               (1 << 29)
#define CC_Z_BIT               (1 << 30)
#define CC_N_BIT               (1 << 31)

/* UARTs ****************************************************/

#define UART_IRDA_BASE         0xffff0800
#define UART_MODEM_BASE        0xffff1000
#define UARTn_IO_RANGE         0x00000800

/* Common UART Registers.  Expressed as offsets from the BASE address */

#define UART_RHR_OFFS          0x00000000 /* Rcv Holding Register */
#define UART_THR_OFFS          0x00000004 /* Xmit Holding Register */
#define UART_FCR_OFFS          0x00000008 /* FIFO Control Register */
#define UART_RFCR_OFFS         0x00000008 /* Rcv FIFO Control Register */
#define UART_TFCR_OFFS         0x00000008 /* Xmit FIFO Control Register */
#define UART_SCR_OFFS          0x0000000c /* Status Control Register */
#define UART_LCR_OFFS          0x00000010 /* Line Control Register */
#define UART_LSR_OFFS          0x00000014 /* Line Status Register */
#define UART_SSR_OFFS          0x00000018 /* Supplementary Status Register */
#define UART_MCR_OFFS          0x0000001c /* Modem Control Register */
#define UART_MSR_OFFS          0x00000020 /* Modem Status Register */
#define UART_IER_OFFS          0x00000024 /* Interrupt Enable Register */
#define UART_ISR_OFFS          0x00000028 /* Interrupt Status Register */
#define UART_EFR_OFFS          0x0000002c /* Enhanced Feature Register */
#define UART_XON1_OFFS         0x00000030 /* XON1 Character Register */
#define UART_XON2_OFFS         0x00000034 /* XON2 Character Register */
#define UART_XOFF1_OFFS        0x00000038 /* XOFF1 Character Register */
#define UART_XOFF2_OFFS        0x0000003c /* XOFF2 Character Register */
#define UART_SPR_OFFS          0x00000040 /* Scratch-pad Register */
#define UART_DIV_115K_OFFS     0x00000044 /* Divisor for baud generation */
#define UART_DIV_BIT_RATE_OFFS 0x00000048 /* For baud rate generation */
#define UART_TCR_OFFS          0x0000004c /* Transmission Control Register */
#define UART_TLR_OFFS          0x00000050 /* Trigger Level Register */
#define UART_MDR_OFFS          0x00000054 /* Mode Definition Register */

/* Registers available only for the IrDA UART (absolute address). */

#define UART_IRDA_MDR1         0xffff0854 /* Mode Definition Register 1 */
#define UART_IRDA_MDR2         0xffff0858 /* Mode Definition Register 2 */
#define UART_IRDA_TXFLL        0xffff085c /* LS Xmit Frame Length Register */
#define UART_IRDA_TXFLH        0xffff0860 /* MS Xmit Frame Length Register */
#define UART_IRDA_RXFLL        0xffff0864 /* LS Rcvd Frame Length Register */
#define UART_IRDA_RXFLH        0xffff0868 /* MS Rcvd Frame Length Register */
#define UART_IRDA_SFLSR        0xffff086c /* Status FIFO Line Status Reg */
#define UART_IRDA_SFREGL       0xffff0870 /* LS Status FIFO Register */
#define UART_IRDA_SFREGH       0xffff0874 /* MS Status FIFO Register */
#define UART_IRDA_BLR          0xffff0878 /* Begin of File Length Register */
#define UART_IRDA_PULSE_WIDTH  0xffff087c /* Pulse Width Register */
#define UART_IRDA_ACREG        0xffff0880 /* Auxiliary Control Register */
#define UART_IRDA_PULSE_START  0xffff0884 /* Start time of pulse */
#define UART_IRDA_RX_W_PTR     0xffff0888 /* RX FIFO write pointer */
#define UART_IRDA_RX_R_PTR     0xffff088c /* RX FIFO read pointer */
#define UART_IRDA_TX_W_PTR     0xffff0890 /* TX FIFO write pointer */
#define UART_IRDA_TX_R_PTR     0xffff0894 /* TX FIFO read pointer */
#define UART_IRDA_STATUS_W_PTR 0xffff0898 /* Write pointer of status FIFO */
#define UART_IRDA_STATUS_R_PTR 0xffff089c /* Read pointer of status FIFO */
#define UART_IRDA_RESUME       0xffff08a0 /* Resume register */
#define UART_IRDA_MUX          0xffff08a4 /* Selects UART_IRDA output mux */

/* Registers available for the Modem UART (absolute addresses) */

#define UART_MODEM_MDR         0xffff1054 /* Mode Definition Register */
#define UART_MODEM_UASR        0xffff1058 /* UART Auto-baud Status Register */
#define UART_MODEM_RDPTR_URX   0xffff105c /* RX FIFO Read Pointer Register */
#define UART_MODEM_WRPTR_URX   0xffff1060 /* RX FIFO Write Pointer Register */
#define UART_MODEM_RDPTR_UTX   0xffff1064 /* TX FIFO Read Pointer Register */
#define UART_MODEM_WRPTR_UTX   0xffff1068 /* TX FIFO Write Pointer Register */

/* UART Settings ********************************************/

/* Miscellaneous UART settings. */

#define UART_RX_FIFO_NOEMPTY 0x00000001
#define UART_SSR_TXFULL      0x00000001
#define UART_LSR_TREF        0x00000020

#define UART_XMIT_FIFO_SIZE      64
#define UART_IRDA_XMIT_FIFO_SIZE 64

/* UART_LCR Register */
                                        /* Bits 31-7: Reserved */
#define UART_LCR_BOC         0x00000040 /* Bit 6: Break Control */
                                        /* Bit 5: Parity Type 2 */
#define UART_LCR_PAREVEN     0x00000010 /* Bit 4: Parity Type 1 */
#define UART_LCR_PARODD      0x00000000
#define UART_LCR_PAREN       0x00000008 /* Bit 3: Paity Enable */
#define UART_LCR_PARDIS      0x00000000
#define UART_LCR_2STOP       0x00000004 /* Bit 2: Number of stop bits */
#define UART_LCR_1STOP       0x00000000
#define UART_LCR_5BITS       0x00000000 /* Bits 0-1: Word-length */
#define UART_LCR_6BITS       0x00000001
#define UART_LCR_7BITS       0x00000002
#define UART_LCR_8BITS       0x00000003

#define UART_FCR_FTL         0x00000000
#define UART_FCR_FIFO_EN     0x00000001
#define UART_FCR_TX_CLR      0x00000002
#define UART_FCR_RX_CLR      0x00000004

#define UART_IER_RECVINT     0x00000001
#define UART_IER_XMITINT     0x00000002
#define UART_IER_LINESTSINT  0x00000004
#define UART_IER_MODEMSTSINT 0x00000008 /* IrDA UART only */
#define UART_IER_XOFFINT     0x00000020
#define UART_IER_RTSINT      0x00000040 /* IrDA UART only */
#define UART_IER_CTSINT      0x00000080 /* IrDA UART only */
#define UART_IER_INTMASK     0x000000ff

#define BAUD_115200          0x00000001
#define BAUD_57600           0x00000002
#define BAUD_38400           0x00000003
#define BAUD_19200           0x00000006
#define BAUD_9600            0x0000000C
#define BAUD_4800            0x00000018
#define BAUD_2400            0x00000030
#define BAUD_1200            0x00000060

#define MDR_UART_MODE        0x00000000  /* Both IrDA and Modem UARTs */
#define MDR_SIR_MODE         0x00000001  /* IrDA UART only */
#define MDR_AUTOBAUDING_MODE 0x00000002  /* Modem UART only */
#define MDR_RESET_MODE       0x00000007  /* Both IrDA and Modem UARTs */

/* SPI ******************************************************/

#define MAX_SPI 3

#define SPI_REGISTER_BASE    0xffff2000

/* GIO ******************************************************/

#define MAX_GIO (35)

#define GIO_REGISTER_BASE    0xffff2800

#define GPIO_IO              0xffff2800 /* Writeable when I/O is configured
					 * as an output; reads value on I/O
					 * pin when I/O is configured as an
					 * input */
#define GPIO_CIO             0xffff2804 /* GPIO configuration register */
#define GPIO_IRQA            0xffff2808 /* In conjunction with GPIO_IRQB
					 * determines the behavior when GPIO
					 * pins configured as input IRQ */
#define GPIO_IRQB            0xffff280c /* Determines the behavior when GPIO
					 * pins configured as input IRQ */
#define GPIO_DDIO            0xffff2810 /* Delta Detect Register
					 * (detects changes in the I/O pins) */
#define GPIO_EN              0xffff2814 /* Selects register for muxed GPIOs */

#define KGIO_REGISTER_BASE   0xffff2900

#define KBGPIO_IO            0xffff2900 /* Keyboard I/O bits: Writeable
					 * when KBGPIO is configured as an
					 * output; reads value on I/O pin
					 * when KBGPIO is configured as an
					 * input */
#define KBGPIO_CIO           0xffff2904 /* KBGPIO configuration register */
#define KBGPIO_IRQA          0xffff2908 /* In conjunction with KBGPIO_IRQB
					 * determines the behavior when
					 * KBGPIO pins configured as input
					 * IRQ */
#define KBGPIO_IRQB          0xffff290c /* In conjunction with KBGPIO_IRQA
					 * determines the behavior when
					 * KBGPIO pins configured as input
					 * IRQ */
#define KBGPIO_DDIO          0xffff2910 /* Delta Detect Register (detects
					 * changes in the KBGPIO pins) */
#define KBGPIO_EN            0xffff2914 /* Selects register for muxed
					 * KBGPIOs */

/* Timers ***************************************************/

#define C5471_TIMER0_CTRL    0xffff2a00
#define C5471_TIMER0_CNT     0xffff2a04
#define C5471_TIMER1_CTRL    0xffff2b00
#define C5471_TIMER1_CNT     0xffff2b04
#define C5471_TIMER2_CTRL    0xffff2c00

#define C5471_TIMER2_CNT     0xffff2c04

/* Interrupts */

#define HAVE_SRC_IRQ_BIN_REG 0

#define INT_FIRST_IO         0xffff2d00
#define INT_IO_RANGE         0x5C

#define IT_REG               0xffff2d00
#define MASK_IT_REG          0xffff2d04
#define SRC_IRQ_REG          0xffff2d08
#define SRC_FIQ_REG          0xffff2d0c
#define SRC_IRQ_BIN_REG      0xffff2d10
#define INT_CTRL_REG         0xffff2d18

#define ILR_IRQ0_REG         0xffff2d1C /*  0-Timer 0       */
#define ILR_IRQ1_REG         0xffff2d20 /*  1-Timer 1       */
#define ILR_IRQ2_REG         0xffff2d24 /*  2-Timer 2       */
#define ILR_IRQ3_REG         0xffff2d28 /*  3-GPIO0         */
#define ILR_IRQ4_REG         0xffff2d2c /*  4-Ethernet      */
#define ILR_IRQ5_REG         0xffff2d30 /*  5-KBGPIO[7:0]   */
#define ILR_IRQ6_REG         0xffff2d34 /*  6-Uart   serial */
#define ILR_IRQ7_REG         0xffff2d38 /*  7-Uart   IRDA   */
#define ILR_IRQ8_REG         0xffff2d3c /*  8-KBGPIO[15:8]  */
#define ILR_IRQ9_REG         0xffff2d40 /*  9-GPIO3         */
#define ILR_IRQ10_REG        0xffff2d44 /* 10-GPIO2         */
#define ILR_IRQ11_REG        0xffff2d48 /* 11-I2C           */
#define ILR_IRQ12_REG        0xffff2d4c /* 12-GPIO1         */
#define ILR_IRQ13_REG        0xffff2d50 /* 13-SPI           */
#define ILR_IRQ14_REG        0xffff2d54 /* 14-GPIO[19:4]    */
#define ILR_IRQ15_REG        0xffff2d58 /* 15-API           */

/* I2C ******************************************************/

#define MAX_I2C 1

/* API ******************************************************/

#define DSPRAM_BASE          0xffe00000  /* DSPRAM base address */
#define DSPRAM_END           0xffe03fff

/* This is the API address range in the DSP address space. */

#define DSPMEM_DSP_START     0x2000
#define DSPMEM_DSP_END       0x3fff

/* This is the API address range in the ARM address space. */

#define DSPMEM_ARM_START     DSPRAM_BASE /* Defined in hardware.h */
#define DSPMEM_ARM_END       DSPRAM_END

/* DSPMEM_IN_RANGE is a generic macro to test is a value is within
 * a range of values.
 */

#define DSPMEM_IN_RANGE(addr, start, end) \
  ((((__u32)(addr)) >= (start)) && (((__u32)(addr)) <= (end)))

/* DSPMEM_ADDR_ALIGNED verifies that a potential DSP address is
 * properly word aligned.
 */

#define DSPMEM_ADDR_ALIGNED(addr, cpu) ((((__u32)(addr)) & 1) == 0)

/* DSPMEM_DSP_ADDR checks if a DSP address lies in within the
 * DSP's API address range.
 */

#define DSPMEM_DSP_ADDR(addr, cpu) \
  DSPMEM_IN_RANGE(addr, DSPMEM_DSP_START, DSPMEM_DSP_END)

/* DSPMEM_ARM_ADDR checks if a ARM address lies in within the
 * ARM's API address range.
 */

#define DSPMEM_ARM_ADDR(addr) \
  DSPMEM_IN_RANGE(addr, DSPMEM_ARM_START, DSPMEM_ARM_END)

/* DSPMEM_DSP_TO_ARM maps a DSP API address into an ARM API address */

#define DSPMEM_DSP_TO_ARM(addr, cpu) \
  ((((__u32)(addr) - DSPMEM_DSP_START) << 1) + DSPMEM_ARM_START)

/* DSPMEM_ARM_TO_DSP maps an ARM API address into a DSP API address */

#define DSPMEM_ARM_TO_DSP(addr) \
  ((((__u32)(addr) - DSPMEM_ARM_START) >> 1) + DSPMEM_DSP_START)

/************************************************************
 * Inline Functions
 ************************************************************/

#ifndef __ASSEMBLY__

# define getreg8(a)   (*(volatile ubyte *)(a))
# define putreg8(v,a) (*(volatile ubyte *)(a) = (v))
# define getreg32(a)   (*(volatile uint32 *)(a))
# define putreg32(v,a) (*(volatile uint32 *)(a) = (v))


/* Some compiler options will convert short loads and stores into byte loads
 * and stores.  We don't want this to happen for IO reads and writes!
 */

/* # define getreg16(a)   (*(volatile uint16 *)(a)) */
static inline unsigned short getreg16(unsigned int addr)
{
  unsigned short retval;
 __asm__ __volatile__("\tldrh %0, [%1]\n\t" : "=r"(retval) : "r"(addr));
  return retval;
}

/* define putreg16(v,a) (*(volatile uint16 *)(a) = (v)) */
static inline void putreg16(uint16 val, unsigned int addr)
{
 __asm__ __volatile__("\tstrh %0, [%1]\n\t": : "r"(val), "r"(addr));
}

#endif

#endif  /* __C5471_H */
