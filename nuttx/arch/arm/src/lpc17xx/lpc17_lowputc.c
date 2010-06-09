/**************************************************************************
 * arch/arm/src/lpc17xx/lpc17_lowputc.c
 *
 *   Copyright (C) 2010 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name NuttX nor the names of its contributors may be
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
 **************************************************************************/

/**************************************************************************
 * Included Files
 **************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <arch/irq.h>
#include <arch/board/board.h>

#include "up_internal.h"
#include "up_arch.h"

#include "lpc17_internal.h"
#include "lpc17_uart.h"
#include "lpc17_serial.h"

/**************************************************************************
 * Private Definitions
 **************************************************************************/
     
/* Select UART parameters for the selected console */

#if defined(CONFIG_UART0_SERIAL_CONSOLE)
#  define CONSOLE_BASE     LPC17_UART0_BASE
#  define CONSOLE_BAUD     CONFIG_UART0_BAUD
#  define CONSOLE_BITS     CONFIG_UART0_BITS
#  define CONSOLE_PARITY   CONFIG_UART0_PARITY
#  define CONSOLE_2STOP    CONFIG_UART0_2STOP
#elif defined(CONFIG_UART1_SERIAL_CONSOLE)
#  define CONSOLE_BASE     LPC17_UART1_BASE
#  define CONSOLE_BAUD     CONFIG_UART1_BAUD
#  define CONSOLE_BITS     CONFIG_UART1_BITS
#  define CONSOLE_PARITY   CONFIG_UART1_PARITY
#  define CONSOLE_2STOP    CONFIG_UART1_2STOP
#elif defined(CONFIG_UART2_SERIAL_CONSOLE)
#  define CONSOLE_BASE     LPC17_UART2_BASE
#  define CONSOLE_BAUD     CONFIG_UART2_BAUD
#  define CONSOLE_BITS     CONFIG_UART2_BITS
#  define CONSOLE_PARITY   CONFIG_UART2_PARITY
#  define CONSOLE_2STOP    CONFIG_UART2_2STOP
#elif defined(CONFIG_UART3_SERIAL_CONSOLE)
#  define CONSOLE_BASE     LPC17_UART3_BASE
#  define CONSOLE_BAUD     CONFIG_UART3_BAUD
#  define CONSOLE_BITS     CONFIG_UART3_BITS
#  define CONSOLE_PARITY   CONFIG_UART3_PARITY
#  define CONSOLE_2STOP    CONFIG_UART3_2STOP
#else
#  error "No CONFIG_UARTn_SERIAL_CONSOLE Setting"
#endif

/* Get word length setting for the console */

#if CONSOLE_BITS == 5
#  define CONSOLE_LCR_WLS UART_LCR_WLS_5BIT
#elif CONSOLE_BITS == 6
#  define CONSOLE_LCR_WLS UART_LCR_WLS_6BIT
#elif CONSOLE_BITS == 7
#  define CONSOLE_LCR_WLS UART_LCR_WLS_7BIT
#elif CONSOLE_BITS == 8
#  define CONSOLE_LCR_WLS UART_LCR_WLS_8BIT
#else
#  error "Invalid CONFIG_UARTn_BITS setting for console "
#endif

/* Get parity setting for the console */

#if CONSOLE_PARITY == 0
#  define CONSOLE_LCR_PAR 0
#elif CONSOLE_PARITY == 1
#  define CONSOLE_LCR_PAR (UART_LCR_PE|UART_LCR_PS_ODD)
#elif CONSOLE_PARITY == 2
#  define CONSOLE_LCR_PAR (UART_LCR_PE|UART_LCR_PS_EVEN)
#elif CONSOLE_PARITY == 3
#  define CONSOLE_LCR_PAR (UART_LCR_PE|UART_LCR_PS_STICK1)
#elif CONSOLE_PARITY == 4
#  define CONSOLE_LCR_PAR (UART_LCR_PE|UART_LCR_PS_STICK0)
#else
#    error "Invalid CONFIG_UARTn_PARITY setting for CONSOLE"
#endif

/* Get stop-bit setting for the console and UART0-3 */

#if CONSOLE_2STOP != 0
#  define CONSOLE_LCR_STOP LPC214X_LCR_STOP_2
#else
#  define CONSOLE_LCR_STOP LPC214X_LCR_STOP_1
#endif

/* LCR and FCR values for the console */

#define CONSOLE_LCR_VALUE (CONSOLE_LCR_WLS | CONSOLE_LCR_PAR | CONSOLE_LCR_STOP)
#define CONSOLE_FCR_VALUE (UART_FCR_RXTRIGGER_8 | UART_FCR_TXRST |\
                           UART_FCR_RXRST | UART_FCR_FIFOEN)

/* Select a CCLK divider to produce the UART PCLK.  The stratey is to select the
 * smallest divisor that results in an solution within range of the 16-bit
 * DLM and DLL divisor:
 *
 *   BAUD = PCLK / (16 * DL), or
 *   DL   = PCLK / BAUD / 16
 *
 * Where:
 *
 *   PCLK = CCLK / divisor
 *
 * Ignoring the fractional divider for now.
 *
 * Check divisor == 1.  This works if the upper limit is met	
 *
 *   DL < 0xffff, or
 *   PCLK / BAUD / 16 < 0xffff, or
 *   CCLK / BAUD / 16 < 0xffff, or
 *   CCLK < BAUD * 0xffff * 16
 *   BAUD > CCLK / 0xffff / 16
 *
 * And the lower limit is met (we can't allow DL to get very close to one).
 *
 *   DL >= MinDL
 *   CCLK / BAUD / 16 >= MinDL, or
 *   BAUD <= CCLK / 16 / MinDL
 */

#if CONSOLE_BAUD < (LPC17_CCLK / 16 / UART_MINDL )
#  define CONSOLE_CCLKDIV  SYSCON_PCLKSEL_CCLK
#  define CONSOLE_NUMERATOR (LPC17_CCLK)

/* Check divisor == 2.  This works if:
 *
 *   2 * CCLK / BAUD / 16 < 0xffff, or
 *   BAUD > CCLK / 0xffff / 8
 *
 * And
 *
 *   2 * CCLK / BAUD / 16 >= MinDL, or
 *   BAUD <= CCLK / 8 / MinDL
 */

#elif CONSOLE_BAUD < (LPC17_CCLK / 8 / UART_MINDL )
#  define CONSOLE_CCLKDIV SYSCON_PCLKSEL_CCLK2
#  define CONSOLE_NUMERATOR (LPC17_CCLK / 2)

/* Check divisor == 4.  This works if:
 *
 *   4 * CCLK / BAUD / 16 < 0xffff, or
 *   BAUD > CCLK / 0xffff / 4
 *
 * And
 *
 *   4 * CCLK / BAUD / 16 >= MinDL, or
 *   BAUD <= CCLK / 4 / MinDL 
 */

#elif CONSOLE_BAUD < (LPC17_CCLK / 4 / UART_MINDL )
#  define CONSOLE_CCLKDIV SYSCON_PCLKSEL_CCLK4
#  define CONSOLE_NUMERATOR (LPC17_CCLK / 4)

/* Check divisor == 8.  This works if:
 *
 *   8 * CCLK / BAUD / 16 < 0xffff, or
 *   BAUD > CCLK / 0xffff / 2
 *
 * And
 *
 *   8 * CCLK / BAUD / 16 >= MinDL, or
 *   BAUD <= CCLK / 2 / MinDL 
 */

#else /* if CONSOLE_BAUD < (LPC17_CCLK / 2 / UART_MINDL ) */
#  define CONSOLE_CCLKDIV   SYSCON_PCLKSEL_CCLK8
#  define CONSOLE_NUMERATOR (LPC17_CCLK /  8)
#endif

/* Then this is the value to use for the DLM and DLL registers */

#define CONSOLE_DL          (CONSOLE_NUMERATOR / (CONSOLE_BAUD << 4)

/**************************************************************************
 * Private Types
 **************************************************************************/

/**************************************************************************
 * Private Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Private Variables
 **************************************************************************/

/**************************************************************************
 * Private Functions
 **************************************************************************/

/**************************************************************************
 * Public Functions
 **************************************************************************/

/**************************************************************************
 * Name: up_lowputc
 *
 * Description:
 *   Output one byte on the serial console
 *
 **************************************************************************/

void up_lowputc(char ch)
{
#ifdef HAVE_UART
  /* Wait for the transmitter to be available */

  while ((getreg32(CONSOLE_BASE+LPC17_UART_LSR_OFFSET) & UART_LSR_THRE) == 0);

  /* Send the character */

  putreg32((uint32_t)ch, CONSOLE_BASE+LPC17_UART_THR_OFFSET);
#endif
}

/**************************************************************************
 * Name: lpc17_lowsetup
 *
 * Description:
 *   This performs basic initialization of the UART used for the serial
 *   console.  Its purpose is to get the console output availabe as soon
 *   as possible.
 *
 *   The UART0/2/3 peripherals are configured using the following registers:
 *   1. Power: In the PCONP register, set bits PCUART0/1/2/3.
 *      On reset, UART0 and UART 1 are enabled (PCUART0 = 1 and PCUART1 = 1)
 *      and UART2/3 are disabled (PCUART1 = 0 and PCUART3 = 0).
 *   2. Peripheral clock: In the PCLKSEL0 register, select PCLK_UART0 and
 *      PCLK_UART1; in the PCLKSEL1 register, select PCLK_UART2 and PCLK_UART3.
 *   3. Baud rate: In the LCR register, set bit DLAB = 1. This enables access
 *      to registers DLL and DLM for setting the baud rate. Also, if needed,
 *      set the fractional baud rate in the fractional divider 
 *   4. UART FIFO: Use bit FIFO enable (bit 0) in FCR register to
 *      enable FIFO.
 *   5. Pins: Select UART pins through the PINSEL registers and pin modes
 *      through the PINMODE registers. UART receive pins should not have
 *      pull-down resistors enabled.
 *   6. Interrupts: To enable UART interrupts set bit DLAB = 0 in the LCRF
 *      register. This enables access to IER. Interrupts are enabled
 *      in the NVIC using the appropriate Interrupt Set Enable register.
 *   7. DMA: UART transmit and receive functions can operate with the
 *      GPDMA controller.
 *
 **************************************************************************/

void lpc17_lowsetup(void)
{
#ifdef HAVE_UART

#if 0
  uint32_t regval;

  /* Step 1: Enable power for all selected UARTs */

  regval = getreg32(LPC17_SYSCON_PCONP);
  regval &= ~(SYSCON_PCONP_PCUART0|SYSCON_PCONP_PCUART1|SYSCON_PCONP_PCUART2|SYSCON_PCONP_PCUART3)
#ifdef CONFIG_LPC17_UART0
  regval |= SYSCON_PCONP_PCUART0;
#endif
#ifdef CONFIG_LPC17_UART1
  regval |= SYSCON_PCONP_PCUART1;
#endif
#ifdef CONFIG_LPC17_UART2
  regval |= SYSCON_PCONP_PCUART2;
#endif
#ifdef CONFIG_LPC17_UART3
  regval |= SYSCON_PCONP_PCUART3;
#endif
  putreg32(regval, LPC17_SYSCON_PCONP);

/* Step 2: Enable peripheral clocking for all selected UARTs */

#define SYSCON_PCLKSET_MASK           (3)

#define SYSCON_PCLKSEL0_UART0_SHIFT   (6)         /* Bits 6-7: Peripheral clock UART0 */
#define SYSCON_PCLKSEL0_UART0_MASK    (3 << SYSCON_PCLKSEL0_UART0_MASK)
#define SYSCON_PCLKSEL0_UART1_SHIFT   (8)         /* Bits 8-9: Peripheral clock UART1 */
#define SYSCON_PCLKSEL0_UART1_MASK    (3 << SYSCON_PCLKSEL0_UART1_SHIFT)


#define SYSCON_PCLKSEL1_UART2_SHIFT   (16)        /* Bits 16-17: Peripheral clock UART2 */
#define SYSCON_PCLKSEL1_UART2_MASK    (3 << SYSCON_PCLKSEL1_UART2_SHIFT)
#define SYSCON_PCLKSEL1_UART3_SHIFT   (18)        /* Bits 18-19: Peripheral clock UART3 */
#define SYSCON_PCLKSEL1_UART3_MASK    (3 << SYSCON_PCLKSEL1_UART3_SHIFT)

  /* Configure UART pins for all selected UARTs */

#define GPIO_UART0_TXD     (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN2)
#define GPIO_UART0_RXD     (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN3

#define GPIO_UART1_TXD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN15)
#define GPIO_UART1_RXD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN16)
#define GPIO_UART1_CTS_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN17)
#define GPIO_UART1_DCD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN18)
#define GPIO_UART1_DSR_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN19)
#define GPIO_UART1_DTR_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN20)
#define GPIO_UART1_RI_1    (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN21)
#define GPIO_UART1_RTS_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN22)

#define GPIO_UART1_TXD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN0)
#define GPIO_UART1_RXD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN1)
#define GPIO_UART1_CTS_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN2)
#define GPIO_UART1_DCD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN3)
#define GPIO_UART1_DSR_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN4)
#define GPIO_UART1_DTR_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN5)
#define GPIO_UART1_RI_2    (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN6)
#define GPIO_UART1_RTS_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN7)

#define GPIO_UART2_TXD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN10)
#define GPIO_UART2_RXD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN11)
#define GPIO_UART2_TXD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN8)
#define GPIO_UART2_RXD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN9)

#define GPIO_UART3_TXD_1   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN0)
#define GPIO_UART3_RXD_1   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN1)
#define GPIO_UART3_TXD_2   (GPIO_ALT3 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN25)
#define GPIO_UART3_RXD_2   (GPIO_ALT3 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN26)
#define GPIO_UART3_TXD_3   (GPIO_ALT3 | GPIO_PULLUP | GPIO_PORT4 | GPIO_PIN28)
#define GPIO_UART3_RXD_3     (GPIO_ALT3 | GPIO_PULLUP | GPIO_PORT4 | GPIO_PIN29)


#ifdef CONFIG_LPC17_UART0
  (void)lpc17_configgpio(GPIO_UART0_RXD);
  (void)lpc17_configgpio(GPIO_UART0_TXD);
  (void)lpc17_configgpio(GPIO_UART0_CTS);
  (void)lpc17_configgpio(GPIO_UART0_RTS);
#endif
#ifdef CONFIG_LPC17_UART1
  (void)lpc17_configgpio(GPIO_UART1_RXD);
  (void)lpc17_configgpio(GPIO_UART1_TXD);
  (void)lpc17_configgpio(GPIO_UART1_CTS);
  (void)lpc17_configgpio(GPIO_UART1_RTS);
#endif
#ifdef CONFIG_LPC17_UART2
  (void)lpc17_configgpio(GPIO_UART2_RXD);
  (void)lpc17_configgpio(GPIO_UART2_TXD);
  (void)lpc17_configgpio(GPIO_UART2_CTS);
  (void)lpc17_configgpio(GPIO_UART2_RTS);
#endif
#ifdef CONFIG_LPC17_UART3
  (void)lpc17_configgpio(GPIO_UART3_RXD);
  (void)lpc17_configgpio(GPIO_UART3_TXD);
  (void)lpc17_configgpio(GPIO_UART3_CTS);
  (void)lpc17_configgpio(GPIO_UART3_RTS);
#endif

#ifdef GPIO_CONSOLE_RXD
#endif
#ifdef GPIO_CONSOLE_TXD
  (void)lpc17_configgpio(GPIO_CONSOLE_TXD);
#endif
#ifdef GPIO_CONSOLE_CTS
  (void)lpc17_configgpio(GPIO_CONSOLE_CTS);
#endif
#ifdef GPIO_CONSOLE_RTS
  (void)lpc17_configgpio(GPIO_CONSOLE_RTS);
#endif

  /* Configure the console (only) */
#if defined(HAVE_CONSOLE) && !defined(CONFIG_SUPPRESS_UART_CONFIG)
  /* Reset and disable receiver and transmitter */

  putreg32((UART_CR_RSTRX|UART_CR_RSTTX|UART_CR_RXDIS|UART_CR_TXDIS),
           CONSOLE_BASE+LPC17_UART_CR_OFFSET);

  /* Disable all interrupts */

  putreg32(0xffffffff, CONSOLE_BASE+LPC17_UART_IDR_OFFSET);

  /* Set up the mode register */

  putreg32(MR_VALUE, CONSOLE_BASE+LPC17_UART_MR_OFFSET);

  /* Configure the console baud */

  putreg32(((LPC17_MCK_FREQUENCY + (LPC17_CONSOLE_BAUD << 3))/(LPC17_CONSOLE_BAUD << 4)),
           CONSOLE_BASE+LPC17_UART_BRGR_OFFSET);

  /* Enable receiver & transmitter */

  putreg32((UART_CR_RXEN|UART_CR_TXEN),
           CONSOLE_BASE+LPC17_UART_CR_OFFSET);
#endif
#endif /* 0 */
#endif /* HAVE_UART */
}


