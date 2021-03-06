/************************************************************************************
 * arch/arm/src/lm3s/lm3s_vectors.S
 * arch/arm/src/chip/lm3s_vectors.S
 *
 *   Copyright (C) 2009-2010 Gregory Nutt. All rights reserved.
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
 ************************************************************************************/

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>
#include <arch/irq.h>

/************************************************************************************
 * Preprocessor Definitions
 ************************************************************************************/

/* Memory Map:
 *
 * 0x0000:0000 - Beginning of FLASH. Address of vectors (if not using bootloader)
 * 0x0002:0000 - Address of vectors if using bootloader
 * 0x0003:ffff - End of flash
 * 0x2000:0000 - Start of SRAM and start of .data (_sdata)
 *             - End of .data (_edata) abd start of .bss (_sbss)
 *             - End of .bss (_ebss) and bottom of idle stack
 *             - _ebss + CONFIG_IDLETHREAD_STACKSIZE = end of idle stack, start of heap
 * 0x2000:ffff - End of SRAM and end of heap
 */

#define IDLE_STACK		(_ebss+CONFIG_IDLETHREAD_STACKSIZE-4)
#define HEAP_BASE		(_ebss+CONFIG_IDLETHREAD_STACKSIZE-4)

/* The Cortex-M3 return from interrupt is unusual.  We provide the following special
 * address to the BX instruction.  The particular value also forces a return to
 * thread mode and covers state from the main stack point, the MSP (vs. the MSP).
 */

#define EXC_RETURN		0xfffffff9

/************************************************************************************
 * Global Symbols
 ************************************************************************************/

	.globl		__start

	.syntax		unified
	.thumb
	.file		"lm3s_vectors.S"

/************************************************************************************
 * Macros
 ************************************************************************************/

/* On entry into an IRQ, the hardware automatically saves the xPSR, PC, LR, R12, R0-R3
 * registers on the stack, then branches to an instantantiation of the following
 * macro.  This macro simply loads the IRQ number into R0, then jumps to the common
 * IRQ handling logic.
 */

	.macro	HANDLER, label, irqno
	.thumb_func
\label:
	mov		r0, #\irqno
	b		lm3s_irqcommon
	.endm

/************************************************************************************
 * Vectors
 ************************************************************************************/

	.section	.vectors, "ax"
	.code		16
	.align		2
	.globl		lm3s_vectors
	.type		lm3s_vectors, function

lm3s_vectors:

/* Processor Exceptions */

	.word	IDLE_STACK			/* Vector  0: Reset stack pointer */
	.word	__start				/* Vector  1: Reset vector */
	.word	lm3s_nmi			/* Vector  2: Non-Maskable Interrupt (NMI) */
	.word	lm3s_hardfault		/* Vector  3: Hard fault */
	.word	lm3s_mpu			/* Vector  4: Memory management (MPU) */
	.word	lm3s_busfault		/* Vector  5: Bus fault */
	.word	lm3s_usagefault		/* Vector  6: Usage fault */
	.word	lm3s_reserved		/* Vector  7: Reserved */
	.word	lm3s_reserved		/* Vector  8: Reserved */
	.word	lm3s_reserved		/* Vector  9: Reserved */
	.word	lm3s_reserved		/* Vector 10: Reserved */
	.word	lm3s_svcall			/* Vector 11: SVC call */
	.word	lm3s_dbgmonitor		/* Vector 12: Debug monitor */
	.word	lm3s_reserved		/* Vector 13: Reserved */
	.word	lm3s_pendsv			/* Vector 14: Pendable system service request */
	.word	lm3s_systick		/* Vector 15: System tick */

/* External Interrupts */

#if defined(CONFIG_ARCH_CHIP_LM3S6918)
	.word	lm3s_gpioa			/* Vector 16: GPIO Port A */
	.word	lm3s_gpiob			/* Vector 17: GPIO Port B */
	.word	lm3s_gpioc			/* Vector 18: GPIO Port C */
	.word	lm3s_gpiod			/* Vector 19: GPIO Port D */
	.word	lm3s_gpioe			/* Vector 20: GPIO Port E */
	.word	lm3s_uart0			/* Vector 21: UART 0 */
	.word	lm3s_uart1			/* Vector 22: UART 1 */
	.word	lm3s_ssi0			/* Vector 23: SSI 0 */
	.word	lm3s_i2c0			/* Vector 24: I2C 0 */
	.word	lm3s_reserved		/* Vector 25: Reserved */
	.word	lm3s_reserved		/* Vector 26: Reserved */
	.word	lm3s_reserved		/* Vector 27: Reserved */
	.word	lm3s_reserved		/* Vector 28: Reserved */
	.word	lm3s_reserved		/* Vector 29: Reserved */
	.word	lm3s_adc0			/* Vector 30: ADC Sequence 0 */
	.word	lm3s_adc1			/* Vector 31: ADC Sequence 1 */
	.word	lm3s_adc2			/* Vector 32: ADC Sequence 2 */
	.word	lm3s_adc3			/* Vector 33: ADC Sequence 3 */
	.word	lm3s_wdog			/* Vector 34: Watchdog Timer */
	.word	lm3s_tmr0a			/* Vector 35: Timer 0 A */
	.word	lm3s_tmr0b			/* Vector 36: Timer 0 B */
	.word	lm3s_tmr1a			/* Vector 37: Timer 1 A */
	.word	lm3s_tmr1b			/* Vector 38: Timer 1 B */
	.word	lm3s_tmr2a			/* Vector 39: Timer 2 A */
	.word	lm3s_tmr2b			/* Vector 40: Timer 3 B */
	.word	lm3s_cmp0			/* Vector 41: Analog Comparator 0 */
	.word	lm3s_cmp1			/* Vector 42: Analog Comparator 1 */
	.word	lm3s_reserved		/* Vector 43: Reserved */
	.word	lm3s_syscon			/* Vector 44: System Control */
	.word	lm3s_flashcon		/* Vector 45: FLASH Control */
	.word	lm3s_gpiof			/* Vector 46: GPIO Port F */
	.word	lm3s_gpiog			/* Vector 47: GPIO Port G */
	.word	lm3s_gpioh			/* Vector 48: GPIO Port H */
	.word	lm3s_reserved		/* Vector 49: Reserved */
	.word	lm3s_ssi1			/* Vector 50: SSI 1 */
	.word	lm3s_tmr3a			/* Vector 51: Timer 3 A */
	.word	lm3s_tmr3b			/* Vector 52: Timer 3 B */
	.word	lm3s_i2c1			/* Vector 53: I2C 1 */
	.word	lm3s_reserved		/* Vector 54: Reserved */
	.word	lm3s_reserved		/* Vector 55: Reserved */
	.word	lm3s_reserved		/* Vector 56: Reserved */
	.word	lm3s_reserved		/* Vector 57: Reserved */
 	.word	lm3s_eth			/* Vector 58: Ethernet Controller */
	.word	lm3s_hib			/* Vector 59: Hibernation Module */
	.word	lm3s_reserved		/* Vector 60: Reserved */
	.word	lm3s_reserved		/* Vector 61: Reserved */
	.word	lm3s_reserved		/* Vector 62: Reserved */
	.word	lm3s_reserved		/* Vector 63: Reserved */
	.word	lm3s_reserved		/* Vector 64: Reserved */
	.word	lm3s_reserved		/* Vector 65: Reserved */
	.word	lm3s_reserved		/* Vector 66: Reserved */
	.word	lm3s_reserved		/* Vector 67: Reserved */
	.word	lm3s_reserved		/* Vector 68: Reserved */
	.word	lm3s_reserved		/* Vector 69: Reserved */
	.word	lm3s_reserved		/* Vector 70: Reserved */
#elif defined(CONFIG_ARCH_CHIP_LM3S6965)
	.word	lm3s_gpioa			/* Vector 16: GPIO Port A */
	.word	lm3s_gpiob			/* Vector 17: GPIO Port B */
	.word	lm3s_gpioc			/* Vector 18: GPIO Port C */
	.word	lm3s_gpiod			/* Vector 19: GPIO Port D */
	.word	lm3s_gpioe			/* Vector 20: GPIO Port E */
	.word	lm3s_uart0			/* Vector 21: UART 0 */
	.word	lm3s_uart1			/* Vector 22: UART 1 */
	.word	lm3s_ssi0			/* Vector 23: SSI 0 */
	.word	lm3s_i2c0			/* Vector 24: I2C 0 */
	.word	lm3s_pwmfault		/* Vector 25: PWM Fault */
	.word	lm3s_pwm0			/* Vector 26: PWM Generator 0 */
	.word	lm3s_pwm1			/* Vector 27: PWM Generator 1 */
	.word	lm3s_pwm2			/* Vector 28: PWM Generator 2 */
	.word	lm3s_qei0			/* Vector 29: QEI0 */
	.word	lm3s_adc0			/* Vector 30: ADC Sequence 0 */
	.word	lm3s_adc1			/* Vector 31: ADC Sequence 1 */
	.word	lm3s_adc2			/* Vector 32: ADC Sequence 2 */
	.word	lm3s_adc3			/* Vector 33: ADC Sequence 3 */
	.word	lm3s_wdog			/* Vector 34: Watchdog Timer */
	.word	lm3s_tmr0a			/* Vector 35: Timer 0 A */
	.word	lm3s_tmr0b			/* Vector 36: Timer 0 B */
	.word	lm3s_tmr1a			/* Vector 37: Timer 1 A */
	.word	lm3s_tmr1b			/* Vector 38: Timer 1 B */
	.word	lm3s_tmr2a			/* Vector 39: Timer 2 A */
	.word	lm3s_tmr2b			/* Vector 40: Timer 3 B */
	.word	lm3s_cmp0			/* Vector 41: Analog Comparator 0 */
	.word	lm3s_cmp1			/* Vector 42: Analog Comparator 1 */
	.word	lm3s_reserved		/* Vector 43: Reserved */
	.word	lm3s_syscon			/* Vector 44: System Control */
	.word	lm3s_flashcon		/* Vector 45: FLASH Control */
	.word	lm3s_gpiof			/* Vector 46: GPIO Port F */
	.word	lm3s_gpiog			/* Vector 47: GPIO Port G */
	.word	lm3s_reserved		/* Vector 48: Reserved */
	.word	lm3s_uart2			/* Vector 49: UART 2 */
	.word	lm3s_reserved		/* Vector 50: Reserved */
	.word	lm3s_tmr3a			/* Vector 51: Timer 3 A */
	.word	lm3s_tmr3b			/* Vector 52: Timer 3 B */
	.word	lm3s_i2c1			/* Vector 53: I2C 1 */
	.word	lm3s_qei1			/* Vector 54: QEI1 */
	.word	lm3s_reserved		/* Vector 55: Reserved */
	.word	lm3s_reserved		/* Vector 56: Reserved */
	.word	lm3s_reserved		/* Vector 57: Reserved */
 	.word	lm3s_eth			/* Vector 58: Ethernet Controller */
	.word	lm3s_hib			/* Vector 59: Hibernation Module */
	.word	lm3s_reserved		/* Vector 60: Reserved */
	.word	lm3s_reserved		/* Vector 61: Reserved */
	.word	lm3s_reserved		/* Vector 62: Reserved */
	.word	lm3s_reserved		/* Vector 63: Reserved */
	.word	lm3s_reserved		/* Vector 64: Reserved */
	.word	lm3s_reserved		/* Vector 65: Reserved */
	.word	lm3s_reserved		/* Vector 66: Reserved */
	.word	lm3s_reserved		/* Vector 67: Reserved */
	.word	lm3s_reserved		/* Vector 68: Reserved */
	.word	lm3s_reserved		/* Vector 69: Reserved */
	.word	lm3s_reserved		/* Vector 70: Reserved */
#elif defined(CONFIG_ARCH_CHIP_LM3S9B96)
	.word	lm3s_gpioa			/* Vector 16: GPIO Port A */
	.word	lm3s_gpiob			/* Vector 17: GPIO Port B */
	.word	lm3s_gpioc			/* Vector 18: GPIO Port C */
	.word	lm3s_gpiod			/* Vector 19: GPIO Port D */
	.word	lm3s_gpioe			/* Vector 20: GPIO Port E */
	.word	lm3s_uart0			/* Vector 21: UART 0 */
	.word	lm3s_uart1			/* Vector 22: UART 1 */
	.word	lm3s_ssi0			/* Vector 23: SSI 0 */
	.word	lm3s_i2c0			/* Vector 24: I2C 0 */
	.word	lm3s_pwmfault		/* Vector 25: PWM Fault */
	.word	lm3s_pwm0			/* Vector 26: PWM Generator 0 */
	.word	lm3s_pwm1			/* Vector 27: PWM Generator 1 */
	.word	lm3s_pwm2			/* Vector 28: PWM Generator 2 */
	.word	lm3s_qei0			/* Vector 29: QEI0 */
	.word	lm3s_adc0			/* Vector 30: ADC Sequence 0 */
	.word	lm3s_adc1			/* Vector 31: ADC Sequence 1 */
	.word	lm3s_adc2			/* Vector 32: ADC Sequence 2 */
	.word	lm3s_adc3			/* Vector 33: ADC Sequence 3 */
	.word	lm3s_wdog			/* Vector 34: Watchdog Timer */
	.word	lm3s_tmr0a			/* Vector 35: Timer 0 A */
	.word	lm3s_tmr0b			/* Vector 36: Timer 0 B */
	.word	lm3s_tmr1a			/* Vector 37: Timer 1 A */
	.word	lm3s_tmr1b			/* Vector 38: Timer 1 B */
	.word	lm3s_tmr2a			/* Vector 39: Timer 2 A */
	.word	lm3s_tmr2b			/* Vector 40: Timer 3 B */
	.word	lm3s_cmp0			/* Vector 41: Analog Comparator 0 */
	.word	lm3s_cmp1			/* Vector 42: Analog Comparator 1 */
	.word	lm3s_cmp2			/* Vector 43: Reserved */
	.word	lm3s_syscon			/* Vector 44: System Control */
	.word	lm3s_flashcon		/* Vector 45: FLASH Control */
	.word	lm3s_gpiof			/* Vector 46: GPIO Port F */
	.word	lm3s_gpiog			/* Vector 47: GPIO Port G */
	.word	lm3s_gpioh			/* Vector 48: GPIO Port H */
	.word	lm3s_uart2			/* Vector 49: UART 2 */
	.word	lm3s_ssi1			/* Vector 50: SSI 1 */
	.word	lm3s_tmr3a			/* Vector 51: Timer 3 A */
	.word	lm3s_tmr3b			/* Vector 52: Timer 3 B */
	.word	lm3s_i2c1			/* Vector 53: I2C 1 */
	.word	lm3s_qei1			/* Vector 54: QEI1 */
	.word	lm3s_can0			/* Vector 55: CAN 0 */
	.word	lm3s_can1			/* Vector 56: CAN 1 */
	.word	lm3s_reserved		/* Vector 57: Reserved */
 	.word	lm3s_eth			/* Vector 58: Ethernet Controller */
	.word	lm3s_reserved		/* Vector 59: Reserved */
	.word	lm3s_usb			/* Vector 60: USB */
	.word	lm3s_pwm3			/* Vector 61: PWM 3 */
	.word	lm3s_udmasoft		/* Vector 62: uDMA Software */
	.word	lm3s_udmaerror		/* Vector 63: uDMA Error */
	.word	lm3s_adc1_0			/* Vector 64: ADC1 Sequence 0 */
	.word	lm3s_adc1_1			/* Vector 65: ADC1 Sequence 1 */
	.word	lm3s_adc1_2			/* Vector 66: ADC1 Sequence 2 */
	.word	lm3s_adc1_3			/* Vector 67: ADC1 Sequence 3 */
	.word	lm3s_i2s0 			/* Vector 68: I2S 0 */
	.word	lm3s_epi			/* Vector 69: Reserved */
	.word	lm3s_gpioj			/* Vector 70: GPIO J */
	.word	lm3s_reserved		/* Vector 71: Reserved */
	#elif defined(CONFIG_ARCH_CHIP_LM3S8962)
	.word	lm3s_gpioa			/* Vector 16: GPIO Port A */
	.word	lm3s_gpiob			/* Vector 17: GPIO Port B */
	.word	lm3s_gpioc			/* Vector 18: GPIO Port C */
	.word	lm3s_gpiod			/* Vector 19: GPIO Port D */
	.word	lm3s_gpioe			/* Vector 20: GPIO Port E */
	.word	lm3s_uart0			/* Vector 21: UART 0 */
	.word	lm3s_uart1			/* Vector 22: UART 1 */
	.word	lm3s_ssi0			/* Vector 23: SSI 0 */
	.word	lm3s_i2c0			/* Vector 24: I2C 0 */
	.word	lm3s_pwmfault		/* Vector 25: PWM Fault */
	.word	lm3s_pwm0			/* Vector 26: PWM Generator 0 */
	.word	lm3s_pwm1			/* Vector 27: PWM Generator 1 */
	.word	lm3s_pwm2			/* Vector 28: PWM Generator 2 */
	.word	lm3s_qei0			/* Vector 29: QEI0 */
	.word	lm3s_adc0			/* Vector 30: ADC Sequence 0 */
	.word	lm3s_adc1			/* Vector 31: ADC Sequence 1 */
	.word	lm3s_adc2			/* Vector 32: ADC Sequence 2 */
	.word	lm3s_adc3			/* Vector 33: ADC Sequence 3 */
	.word	lm3s_wdog			/* Vector 34: Watchdog Timer */
	.word	lm3s_tmr0a			/* Vector 35: Timer 0 A */
	.word	lm3s_tmr0b			/* Vector 36: Timer 0 B */
	.word	lm3s_tmr1a			/* Vector 37: Timer 1 A */
	.word	lm3s_tmr1b			/* Vector 38: Timer 1 B */
	.word	lm3s_tmr2a			/* Vector 39: Timer 2 A */
	.word	lm3s_tmr2b			/* Vector 40: Timer 3 B */
	.word	lm3s_cmp0			/* Vector 41: Analog Comparator 0 */
	.word	lm3s_reserved		/* Vector 42: Reserved */
	.word	lm3s_reserved		/* Vector 43: Reserved */
	.word	lm3s_syscon			/* Vector 44: System Control */
	.word	lm3s_flashcon		/* Vector 45: FLASH Control */
	.word	lm3s_gpiof			/* Vector 46: GPIO Port F */
	.word	lm3s_gpiog			/* Vector 47: GPIO Port G */
	.word	lm3s_reserved		/* Vector 48: Reserved */
	.word	lm3s_reserved		/* Vector 49: Reserved */
	.word	lm3s_reserved		/* Vector 50: Reserved */
	.word	lm3s_tmr3a			/* Vector 51: Timer 3 A */
	.word	lm3s_tmr3b			/* Vector 52: Timer 3 B */
	.word	lm3s_reserved		/* Vector 53: Reserved*/
	.word	lm3s_qei1			/* Vector 54: QEI1 */
	.word	lm3s_can0			/* Vector 55: Can Controller */
	.word	lm3s_reserved		/* Vector 56: Reserved */
	.word	lm3s_reserved		/* Vector 57: Reserved */
 	.word	lm3s_eth			/* Vector 58: Ethernet Controller */
	.word	lm3s_hib			/* Vector 59: Hibernation Module */
	.word	lm3s_reserved		/* Vector 60: Reserved */
	.word	lm3s_reserved		/* Vector 61: Reserved */
	.word	lm3s_reserved		/* Vector 62: Reserved */
	.word	lm3s_reserved		/* Vector 63: Reserved */
	.word	lm3s_reserved		/* Vector 64: Reserved */
	.word	lm3s_reserved		/* Vector 65: Reserved */
	.word	lm3s_reserved		/* Vector 66: Reserved */
	.word	lm3s_reserved		/* Vector 67: Reserved */
	.word	lm3s_reserved		/* Vector 68: Reserved */
	.word	lm3s_reserved		/* Vector 69: Reserved */
	.word	lm3s_reserved		/* Vector 70: Reserved */
#else
#  error "Vectors not specified for this LM3S chip"
#endif
	.size	lm3s_vectors, .-lm3s_vectors

/************************************************************************************
 * .text
 ************************************************************************************/

	.text
	.type	handlers, function
	.thumb_func
handlers:
	HANDLER	lm3s_reserved, LM3S_IRQ_RESERVED		/* Unexpected/reserved vector */
	HANDLER	lm3s_nmi, LM3S_IRQ_NMI					/* Vector  2: Non-Maskable Interrupt (NMI) */
	HANDLER	lm3s_hardfault, LM3S_IRQ_HARDFAULT		/* Vector  3: Hard fault */
	HANDLER	lm3s_mpu, LM3S_IRQ_MEMFAULT				/* Vector  4: Memory management (MPU) */
	HANDLER	lm3s_busfault, LM3S_IRQ_BUSFAULT		/* Vector  5: Bus fault */
	HANDLER	lm3s_usagefault, LM3S_IRQ_USAGEFAULT	/* Vector  6: Usage fault */
	HANDLER	lm3s_svcall, LM3S_IRQ_SVCALL			/* Vector 11: SVC call */
	HANDLER	lm3s_dbgmonitor, LM3S_IRQ_DBGMONITOR	/* Vector 12: Debug Monitor */
	HANDLER	lm3s_pendsv, LM3S_IRQ_PENDSV			/* Vector 14: Penable system service request */
	HANDLER	lm3s_systick, LM3S_IRQ_SYSTICK			/* Vector 15: System tick */

#if defined(CONFIG_ARCH_CHIP_LM3S6918)
	HANDLER	lm3s_gpioa, LM3S_IRQ_GPIOA				/* Vector 16: GPIO Port A */
	HANDLER	lm3s_gpiob, LM3S_IRQ_GPIOB				/* Vector 17: GPIO Port B */
	HANDLER	lm3s_gpioc, LM3S_IRQ_GPIOC				/* Vector 18: GPIO Port C */
	HANDLER	lm3s_gpiod, LM3S_IRQ_GPIOD				/* Vector 19: GPIO Port D */
	HANDLER	lm3s_gpioe, LM3S_IRQ_GPIOE				/* Vector 20: GPIO Port E */
	HANDLER	lm3s_uart0, LM3S_IRQ_UART0				/* Vector 21: UART 0 */
	HANDLER	lm3s_uart1, LM3S_IRQ_UART1				/* Vector 22: UART 1 */
	HANDLER	lm3s_ssi0, LM3S_IRQ_SSI0				/* Vector 23: SSI 0 */
	HANDLER	lm3s_i2c0, LM3S_IRQ_I2C0				/* Vector 24: I2C 0 */
	HANDLER	lm3s_adc0, LM3S_IRQ_ADC0				/* Vector 30: ADC Sequence 0 */
	HANDLER	lm3s_adc1, LM3S_IRQ_ADC1				/* Vector 31: ADC Sequence 1 */
	HANDLER	lm3s_adc2, LM3S_IRQ_ADC2				/* Vector 32: ADC Sequence 2 */
	HANDLER	lm3s_adc3, LM3S_IRQ_ADC3				/* Vector 33: ADC Sequence 3 */
	HANDLER	lm3s_wdog, LM3S_IRQ_WDOG				/* Vector 34: Watchdog Timer */
	HANDLER	lm3s_tmr0a, LM3S_IRQ_TIMER0A			/* Vector 35: Timer 0 A */
	HANDLER	lm3s_tmr0b, LM3S_IRQ_TIMER0B			/* Vector 36: Timer 0 B */
	HANDLER	lm3s_tmr1a, LM3S_IRQ_TIMER1A			/* Vector 37: Timer 1 A */
	HANDLER	lm3s_tmr1b, LM3S_IRQ_TIMER1B			/* Vector 38: Timer 1 B */
	HANDLER	lm3s_tmr2a, LM3S_IRQ_TIMER2A			/* Vector 39: Timer 2 A */
	HANDLER	lm3s_tmr2b, LM3S_IRQ_TIMER2B			/* Vector 40: Timer 3 B */
	HANDLER	lm3s_cmp0, LM3S_IRQ_COMPARE0			/* Vector 41: Analog Comparator 0 */
	HANDLER	lm3s_cmp1, LM3S_IRQ_COMPARE1			/* Vector 42: Analog Comparator 1 */
	HANDLER	lm3s_syscon, LM3S_IRQ_SYSCON			/* Vector 44: System Control */
	HANDLER	lm3s_flashcon, LM3S_IRQ_FLASHCON		/* Vector 45: FLASH Control */
	HANDLER	lm3s_gpiof, LM3S_IRQ_GPIOF				/* Vector 46: GPIO Port F */
	HANDLER	lm3s_gpiog, LM3S_IRQ_GPIOG				/* Vector 47: GPIO Port G */
	HANDLER	lm3s_gpioh, LM3S_IRQ_GPIOH				/* Vector 48: GPIO Port H */
	HANDLER	lm3s_ssi1, LM3S_IRQ_SSI1				/* Vector 50: SSI 1 */
	HANDLER	lm3s_tmr3a, LM3S_IRQ_TIMER3A			/* Vector 51: Timer 3 A */
	HANDLER	lm3s_tmr3b, LM3S_IRQ_TIMER3B			/* Vector 52: Timer 3 B */
	HANDLER	lm3s_i2c1, LM3S_IRQ_I2C1				/* Vector 53: I2C 1 */
 	HANDLER	lm3s_eth, LM3S_IRQ_ETHCON				/* Vector 58: Ethernet Controller */
	HANDLER	lm3s_hib, LM3S_IRQ_HIBERNATE			/* Vector 59: Hibernation Module */
#elif defined(CONFIG_ARCH_CHIP_LM3S6965)
	HANDLER	lm3s_gpioa, LM3S_IRQ_GPIOA				/* Vector 16: GPIO Port A */
	HANDLER	lm3s_gpiob, LM3S_IRQ_GPIOB				/* Vector 17: GPIO Port B */
	HANDLER	lm3s_gpioc, LM3S_IRQ_GPIOC				/* Vector 18: GPIO Port C */
	HANDLER	lm3s_gpiod, LM3S_IRQ_GPIOD				/* Vector 19: GPIO Port D */
	HANDLER	lm3s_gpioe, LM3S_IRQ_GPIOE				/* Vector 20: GPIO Port E */
	HANDLER	lm3s_uart0, LM3S_IRQ_UART0				/* Vector 21: UART 0 */
	HANDLER	lm3s_uart1, LM3S_IRQ_UART1				/* Vector 22: UART 1 */
	HANDLER	lm3s_ssi0, LM3S_IRQ_SSI0				/* Vector 23: SSI 0 */
	HANDLER	lm3s_i2c0, LM3S_IRQ_I2C0				/* Vector 24: I2C 0 */
	HANDLER	lm3s_pwmfault, LM3S_IRQ_PWMFAULT		/* Vector 25: PWM Fault */
	HANDLER	lm3s_pwm0, LM3S_IRQ_PWM0				/* Vector 26: PWM Generator 0 */
	HANDLER	lm3s_pwm1, LM3S_IRQ_PWM1				/* Vector 27: PWM Generator 1 */
	HANDLER	lm3s_pwm2, LM3S_IRQ_PWM2				/* Vector 28: PWM Generator 2 */
	HANDLER	lm3s_qei0, LM3S_IRQ_QEI0				/* Vector 29: QEI 0 */
	HANDLER	lm3s_adc0, LM3S_IRQ_ADC0				/* Vector 30: ADC Sequence 0 */
	HANDLER	lm3s_adc1, LM3S_IRQ_ADC1				/* Vector 31: ADC Sequence 1 */
	HANDLER	lm3s_adc2, LM3S_IRQ_ADC2				/* Vector 32: ADC Sequence 2 */
	HANDLER	lm3s_adc3, LM3S_IRQ_ADC3				/* Vector 33: ADC Sequence 3 */
	HANDLER	lm3s_wdog, LM3S_IRQ_WDOG				/* Vector 34: Watchdog Timer */
	HANDLER	lm3s_tmr0a, LM3S_IRQ_TIMER0A			/* Vector 35: Timer 0 A */
	HANDLER	lm3s_tmr0b, LM3S_IRQ_TIMER0B			/* Vector 36: Timer 0 B */
	HANDLER	lm3s_tmr1a, LM3S_IRQ_TIMER1A			/* Vector 37: Timer 1 A */
	HANDLER	lm3s_tmr1b, LM3S_IRQ_TIMER1B			/* Vector 38: Timer 1 B */
	HANDLER	lm3s_tmr2a, LM3S_IRQ_TIMER2A			/* Vector 39: Timer 2 A */
	HANDLER	lm3s_tmr2b, LM3S_IRQ_TIMER2B			/* Vector 40: Timer 3 B */
	HANDLER	lm3s_cmp0, LM3S_IRQ_COMPARE0			/* Vector 41: Analog Comparator 0 */
	HANDLER	lm3s_cmp1, LM3S_IRQ_COMPARE1			/* Vector 42: Analog Comparator 1 */
	HANDLER	lm3s_syscon, LM3S_IRQ_SYSCON			/* Vector 44: System Control */
	HANDLER	lm3s_flashcon, LM3S_IRQ_FLASHCON		/* Vector 45: FLASH Control */
	HANDLER	lm3s_gpiof, LM3S_IRQ_GPIOF				/* Vector 46: GPIO Port F */
	HANDLER	lm3s_gpiog, LM3S_IRQ_GPIOG				/* Vector 47: GPIO Port G */
	HANDLER	lm3s_uart2, LM3S_IRQ_UART1				/* Vector 49: UART 1 */
	HANDLER	lm3s_tmr3a, LM3S_IRQ_TIMER3A			/* Vector 51: Timer 3 A */
	HANDLER	lm3s_tmr3b, LM3S_IRQ_TIMER3B			/* Vector 52: Timer 3 B */
	HANDLER	lm3s_i2c1, LM3S_IRQ_I2C1				/* Vector 53: I2C 1 */
	HANDLER lm3s_qei1, LM3S_IRQ_QEI1				/* Vector 54: QEI 1 */
 	HANDLER	lm3s_eth, LM3S_IRQ_ETHCON				/* Vector 58: Ethernet Controller */
	HANDLER	lm3s_hib, LM3S_IRQ_HIBERNATE			/* Vector 59: Hibernation Module */
#elif defined(CONFIG_ARCH_CHIP_LM3S8962)
	HANDLER	lm3s_gpioa, LM3S_IRQ_GPIOA				/* Vector 16: GPIO Port A */
	HANDLER	lm3s_gpiob, LM3S_IRQ_GPIOB				/* Vector 17: GPIO Port B */
	HANDLER	lm3s_gpioc, LM3S_IRQ_GPIOC				/* Vector 18: GPIO Port C */
	HANDLER	lm3s_gpiod, LM3S_IRQ_GPIOD				/* Vector 19: GPIO Port D */
	HANDLER	lm3s_gpioe, LM3S_IRQ_GPIOE				/* Vector 20: GPIO Port E */
	HANDLER	lm3s_uart0, LM3S_IRQ_UART0				/* Vector 21: UART 0 */
	HANDLER	lm3s_uart1, LM3S_IRQ_UART1				/* Vector 22: UART 1 */
	HANDLER	lm3s_ssi0, LM3S_IRQ_SSI0				/* Vector 23: SSI 0 */
	HANDLER	lm3s_i2c0, LM3S_IRQ_I2C0				/* Vector 24: I2C 0 */
	HANDLER	lm3s_pwmfault, LM3S_IRQ_PWMFAULT		/* Vector 25: PWM Fault */
	HANDLER	lm3s_pwm0, LM3S_IRQ_PWM0				/* Vector 26: PWM Generator 0 */
	HANDLER	lm3s_pwm1, LM3S_IRQ_PWM1				/* Vector 27: PWM Generator 1 */
	HANDLER	lm3s_pwm2, LM3S_IRQ_PWM2				/* Vector 28: PWM Generator 2 */
	HANDLER	lm3s_qei0, LM3S_IRQ_QEI0				/* Vector 29: QEI 0 */
	HANDLER	lm3s_adc0, LM3S_IRQ_ADC0				/* Vector 30: ADC Sequence 0 */
	HANDLER	lm3s_adc1, LM3S_IRQ_ADC1				/* Vector 31: ADC Sequence 1 */
	HANDLER	lm3s_adc2, LM3S_IRQ_ADC2				/* Vector 32: ADC Sequence 2 */
	HANDLER	lm3s_adc3, LM3S_IRQ_ADC3				/* Vector 33: ADC Sequence 3 */
	HANDLER	lm3s_wdog, LM3S_IRQ_WDOG				/* Vector 34: Watchdog Timer */
	HANDLER	lm3s_tmr0a, LM3S_IRQ_TIMER0A			/* Vector 35: Timer 0 A */
	HANDLER	lm3s_tmr0b, LM3S_IRQ_TIMER0B			/* Vector 36: Timer 0 B */
	HANDLER	lm3s_tmr1a, LM3S_IRQ_TIMER1A			/* Vector 37: Timer 1 A */
	HANDLER	lm3s_tmr1b, LM3S_IRQ_TIMER1B			/* Vector 38: Timer 1 B */
	HANDLER	lm3s_tmr2a, LM3S_IRQ_TIMER2A			/* Vector 39: Timer 2 A */
	HANDLER	lm3s_tmr2b, LM3S_IRQ_TIMER2B			/* Vector 40: Timer 3 B */
	HANDLER	lm3s_cmp0, LM3S_IRQ_COMPARE0			/* Vector 41: Analog Comparator 0 */
	HANDLER	lm3s_syscon, LM3S_IRQ_SYSCON			/* Vector 44: System Control */
	HANDLER	lm3s_flashcon, LM3S_IRQ_FLASHCON		/* Vector 45: FLASH Control */
	HANDLER	lm3s_gpiof, LM3S_IRQ_GPIOF				/* Vector 46: GPIO Port F */
	HANDLER	lm3s_gpiog, LM3S_IRQ_GPIOG				/* Vector 47: GPIO Port G */
	HANDLER	lm3s_uart2, LM3S_IRQ_UART1				/* Vector 49: UART 1 */
	HANDLER	lm3s_tmr3a, LM3S_IRQ_TIMER3A			/* Vector 51: Timer 3 A */
	HANDLER	lm3s_tmr3b, LM3S_IRQ_TIMER3B			/* Vector 52: Timer 3 B */
	HANDLER	lm3s_i2c1, LM3S_IRQ_I2C1				/* Vector 53: I2C 1 */
	HANDLER	lm3s_qei1, LM3S_IRQ_QEI1				/* Vector 54: QEI 1 */
	HANDLER	lm3s_can0, LM3S_IRQ_CAN0				/* Vector 55: CAN 0 */
 	HANDLER	lm3s_eth, LM3S_IRQ_ETHCON				/* Vector 58: Ethernet Controller */
	HANDLER	lm3s_hib, LM3S_IRQ_HIBERNATE			/* Vector 59: Hibernation Module */
#elif defined(CONFIG_ARCH_CHIP_LM3S9B96)
	HANDLER	lm3s_gpioa, LM3S_IRQ_GPIOA				/* Vector 16: GPIO Port A */
	HANDLER	lm3s_gpiob, LM3S_IRQ_GPIOB				/* Vector 17: GPIO Port B */
	HANDLER	lm3s_gpioc, LM3S_IRQ_GPIOC				/* Vector 18: GPIO Port C */
	HANDLER	lm3s_gpiod, LM3S_IRQ_GPIOD				/* Vector 19: GPIO Port D */
	HANDLER	lm3s_gpioe, LM3S_IRQ_GPIOE				/* Vector 20: GPIO Port E */
	HANDLER	lm3s_uart0, LM3S_IRQ_UART0				/* Vector 21: UART 0 */
	HANDLER	lm3s_uart1, LM3S_IRQ_UART1				/* Vector 22: UART 1 */
	HANDLER	lm3s_ssi0, LM3S_IRQ_SSI0				/* Vector 23: SSI 0 */
	HANDLER	lm3s_i2c0, LM3S_IRQ_I2C0				/* Vector 24: I2C 0 */
	HANDLER	lm3s_pwmfault, LM3S_IRQ_PWMFAULT		/* Vector 25: PWM Fault */
	HANDLER	lm3s_pwm0, LM3S_IRQ_PWM0				/* Vector 26: PWM Generator 0 */
	HANDLER	lm3s_pwm1, LM3S_IRQ_PWM1				/* Vector 27: PWM Generator 1 */
	HANDLER	lm3s_pwm2, LM3S_IRQ_PWM2				/* Vector 28: PWM Generator 2 */
	HANDLER	lm3s_qei0, LM3S_IRQ_QEI0				/* Vector 29: QEI 0 */
	HANDLER	lm3s_adc0, LM3S_IRQ_ADC0				/* Vector 30: ADC Sequence 0 */
	HANDLER	lm3s_adc1, LM3S_IRQ_ADC1				/* Vector 31: ADC Sequence 1 */
	HANDLER	lm3s_adc2, LM3S_IRQ_ADC2				/* Vector 32: ADC Sequence 2 */
	HANDLER	lm3s_adc3, LM3S_IRQ_ADC3				/* Vector 33: ADC Sequence 3 */
	HANDLER	lm3s_wdog, LM3S_IRQ_WDOG				/* Vector 34: Watchdog Timer */
	HANDLER	lm3s_tmr0a, LM3S_IRQ_TIMER0A			/* Vector 35: Timer 0 A */
	HANDLER	lm3s_tmr0b, LM3S_IRQ_TIMER0B			/* Vector 36: Timer 0 B */
	HANDLER	lm3s_tmr1a, LM3S_IRQ_TIMER1A			/* Vector 37: Timer 1 A */
	HANDLER	lm3s_tmr1b, LM3S_IRQ_TIMER1B			/* Vector 38: Timer 1 B */
	HANDLER	lm3s_tmr2a, LM3S_IRQ_TIMER2A			/* Vector 39: Timer 2 A */
	HANDLER	lm3s_tmr2b, LM3S_IRQ_TIMER2B			/* Vector 40: Timer 3 B */
	HANDLER	lm3s_cmp0, LM3S_IRQ_COMPARE0			/* Vector 41: Analog Comparator 0 */
	HANDLER	lm3s_cmp1, LM3S_IRQ_COMPARE1			/* Vector 42: Analog Comparator 1 */
	HANDLER	lm3s_cmp2, LM3S_IRQ_COMPARE2			/* Vector 43: Analog Comparator 2 */
	HANDLER	lm3s_syscon, LM3S_IRQ_SYSCON			/* Vector 44: System Control */
	HANDLER	lm3s_flashcon, LM3S_IRQ_FLASHCON		/* Vector 45: FLASH Control */
	HANDLER	lm3s_gpiof, LM3S_IRQ_GPIOF				/* Vector 46: GPIO Port F */
	HANDLER	lm3s_gpiog, LM3S_IRQ_GPIOG				/* Vector 47: GPIO Port G */
	HANDLER	lm3s_gpioh, LM3S_IRQ_GPIOH				/* Vector 48: GPIO Port H */
	HANDLER	lm3s_uart2, LM3S_IRQ_UART2				/* Vector 49: UART 2 */
	HANDLER	lm3s_ssi1, LM3S_IRQ_SSI1				/* Vector 50: GPIO Port H */
	HANDLER	lm3s_tmr3a, LM3S_IRQ_TIMER3A			/* Vector 51: Timer 3 A */
	HANDLER	lm3s_tmr3b, LM3S_IRQ_TIMER3B			/* Vector 52: Timer 3 B */
	HANDLER	lm3s_i2c1, LM3S_IRQ_I2C1				/* Vector 53: I2C 1 */
	HANDLER	lm3s_qei1, LM3S_IRQ_QEI1				/* Vector 54: QEI 1 */
 	HANDLER	lm3s_can0, LM3S_IRQ_CAN0				/* Vector 55: CAN 0 */
	HANDLER	lm3s_can1, LM3S_IRQ_CAN1				/* Vector 56: CAN 1 */
	HANDLER	lm3s_eth, LM3S_IRQ_ETHCON				/* Vector 58: Ethernet Controller */
 	HANDLER	lm3s_usb, LM3S_IRQ_USB					/* Vector 60: USB */
 	HANDLER	lm3s_pwm3, LM3S_IRQ_PWM3				/* Vector 61: PWM 3  */
 	HANDLER	lm3s_udmasoft, LM3S_IRQ_UDMASOFT		/* Vector 62: uDMA Software  */
 	HANDLER	lm3s_udmaerror, LM3S_IRQ_UDMAERROR		/* Vector 63: uDMA Error */
 	HANDLER	lm3s_adc1_0, LM3S_IRQ_ADC1_0			/* Vector 64: ADC1 Sequence 0 */
 	HANDLER	lm3s_adc1_1, LM3S_IRQ_ADC1_1			/* Vector 65: ADC1 Sequence 1 */
 	HANDLER	lm3s_adc1_2, LM3S_IRQ_ADC1_2			/* Vector 66: ADC1 Sequence 2 */
 	HANDLER	lm3s_adc1_3, LM3S_IRQ_ADC1_3			/* Vector 67: ADC1 Sequence 3 */
 	HANDLER	lm3s_i2s0, LM3S_IRQ_I2S0				/* Vector 68: I2S 0 */
 	HANDLER	lm3s_epi, LM3S_IRQ_EPI					/* Vector 69: EPI  */
 	HANDLER	lm3s_gpioj, LM3S_IRQ_GPIOJ				/* Vector 70: GPIO Port J */
#else
#  error "Vectors not specified for this LM3S chip"
#endif

/* Common IRQ handling logic.  On entry here, the return stack is on either
 * the PSP or the MSP and looks like the following:
 *
 *      REG_XPSR
 *      REG_R15
 *      REG_R14
 *      REG_R12
 *      REG_R3
 *      REG_R2
 *      REG_R1
 * MSP->REG_R0
 *
 * And
 *      R0 contains the IRQ number
 *      R14 Contains the EXC_RETURN value
 *      We are in handler mode and the current SP is the MSP
 */

lm3s_irqcommon:

	/* Complete the context save */

#ifdef CONFIG_NUTTX_KERNEL
	/* The EXC_RETURN value will be 0xfffffff9 (privileged thread) or 0xfffffff1
	 * (handler mode) if the state is on the MSP.  It can only be on the PSP if
	 * EXC_RETURN is 0xfffffffd (unprivileged thread)
	 */

	adds	r2, r14, #3				/* If R14=0xfffffffd, then r2 == 0 */
	ite		ne						/* Next two instructions are condition */
	mrsne	r1, msp					/* R1=The main stack pointer */
	mrseq	r1, psp					/* R1=The process stack pointer */
#else
	mrs		r1, msp					/* R1=The main stack pointer */
#endif

	mov		r2, r1					/* R2=Copy of the main/process stack pointer */
	add		r2, #HW_XCPT_SIZE		/* R2=MSP/PSP before the interrupt was taken */
	mrs		r3, primask				/* R3=Current PRIMASK setting */
#ifdef CONFIG_NUTTX_KERNEL 
	stmdb	r1!, {r2-r11,r14}		/* Save the remaining registers plus the SP value */
#else
	stmdb	r1!, {r2-r11}			/* Save the remaining registers plus the SP value */
#endif

	/* Disable interrupts, select the stack to use for interrupt handling
	 * and call up_doirq to handle the interrupt
	 */

	cpsid	i						/* Disable further interrupts */

	/* If CONFIG_ARCH_INTERRUPTSTACK is defined, we will use a special interrupt
	 * stack pointer.  The way that this is done here prohibits nested interrupts!
	 * Otherwise, we will re-use the main stack for interrupt level processing.
	 */

#if CONFIG_ARCH_INTERRUPTSTACK > 3
	ldr		sp, =g_intstackbase
	str		r1, [sp, #-4]!			/* Save the MSP on the interrupt stack */
	bl		up_doirq				/* R0=IRQ, R1=register save (msp) */
	ldr		r1, [sp, #+4]!			/* Recover R1=main stack pointer */
#else
	mov		sp, r1					/* We are using the main stack pointer */
	bl		up_doirq				/* R0=IRQ, R1=register save (msp) */
	mov		r1, sp					/* Recover R1=main stack pointer */
#endif

	/* On return from up_doirq, R0 will hold a pointer to register context
	 * array to use for the interrupt return.  If that return value is the same
	 * as current stack pointer, then things are relatively easy.
	 */

	cmp		r0, r1					/* Context switch? */
	beq		1f						/* Branch if no context switch */

	/* We are returning with a pending context switch.  This case is different
	 * because in this case, the register save structure does not lie on the
	 * stack but, rather, are within a TCB structure.  We'll have to copy some
	 * values to the stack.
	 */

	add		r1, r0, #SW_XCPT_SIZE	/* R1=Address of HW save area in reg array */
	ldmia	r1, {r4-r11}			/* Fetch eight registers in HW save area */
	ldr		r1, [r0, #(4*REG_SP)]	/* R1=Value of SP before interrupt */
	stmdb	r1!, {r4-r11}			/* Store eight registers in HW save area */
#ifdef CONFIG_NUTTX_KERNEL 
	ldmia	r0, {r2-r11,r14}		/* Recover R4-R11, r14 + 2 temp values */
#else
	ldmia	r0, {r2-r11}			/* Recover R4-R11 + 2 temp values */
#endif
	b		2f						/* Re-join common logic */

	/* We are returning with no context switch.  We simply need to "unwind"
	 * the same stack frame that we created
	 */
1:
#ifdef CONFIG_NUTTX_KERNEL 
	ldmia	r1!, {r2-r11,r14}		/* Recover R4-R11, r14 + 2 temp values */
#else
	ldmia	r1!, {r2-r11}			/* Recover R4-R11 + 2 temp values */
#endif
2:
#ifdef CONFIG_NUTTX_KERNEL
	/* The EXC_RETURN value will be 0xfffffff9 (privileged thread) or 0xfffffff1
	 * (handler mode) if the state is on the MSP.  It can only be on the PSP if
	 * EXC_RETURN is 0xfffffffd (unprivileged thread)
	 */

	adds	r0, r14, #3				/* If R14=0xfffffffd, then r0 == 0 */
	ite		ne						/* Next two instructions are condition */
	msrne	msp, r1					/* R1=The main stack pointer */
	msreq	psp, r1					/* R1=The process stack pointer */
#else
	msr		msp, r1					/* Recover the return MSP value */

	/* Preload r14 with the special return value first (so that the return
	 * actually occurs with interrupts still disabled).
	 */

	ldr		r14, =EXC_RETURN		/* Load the special value */
#endif

    /* Restore the interrupt state */

	msr		primask, r3				/* Restore interrupts */

	/* Always return with R14 containing the special value that will: (1)
	 * return to thread mode, and (2) continue to use the MSP
	 */

	bx		r14						/* And return */
	.size	handlers, .-handlers

/************************************************************************************
 *  Name: up_interruptstack/g_intstackbase
 *
 * Description:
 *   Shouldn't happen
 *
 ************************************************************************************/

#if CONFIG_ARCH_INTERRUPTSTACK > 3
	.bss
	.global	g_intstackbase
	.align	4
up_interruptstack:
	.skip	(CONFIG_ARCH_INTERRUPTSTACK & ~3)
g_intstackbase:
	.size	up_interruptstack, .-up_interruptstack
#endif

/************************************************************************************
 * .rodata
 ************************************************************************************/

	.section	.rodata, "a"

/* Variables: _sbss is the start of the BSS region (see ld.script) _ebss is the end
 * of the BSS regsion (see ld.script). The idle task stack starts at the end of BSS
 * and is of size CONFIG_IDLETHREAD_STACKSIZE.  The IDLE thread is the thread that
 * the system boots on and, eventually, becomes the idle, do nothing task that runs
 * only when there is nothing else to run.  The heap continues from there until the
 * end of memory.  See g_heapbase below.
 */

	.globl	g_heapbase
	.type	g_heapbase, object
g_heapbase:
	.long	_ebss+CONFIG_IDLETHREAD_STACKSIZE
	.size	g_heapbase, .-g_heapbase

	.end
