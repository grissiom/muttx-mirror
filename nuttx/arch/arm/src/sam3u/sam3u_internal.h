/************************************************************************************
 * arch/arm/src/sam3u/sam3u_internal.h
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

#ifndef __ARCH_ARM_SRC_SAM3U_SAM3U_INTERNAL_H
#define __ARCH_ARM_SRC_SAM3U_SAM3U_INTERNAL_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include "up_internal.h"
#include "chip.h"

/************************************************************************************
 * Definitions
 ************************************************************************************/

/* Configuration ********************************************************************/

/* Bit-encoded input to sam3u_configgpio() ******************************************/

/* 16-bit Encoding:
 * MMCC C... VPPB BBBB
 */

/* Input/Output mode:
 *
 * MM.. .... .... ....
 */

#define GPIO_CFG_SHIFT             (14)        /* Bits 14-15: GPIO mode */
#define GPIO_CFG_MASK              (3 << GPIO_CNF_SHIFT)
#  define GPIO_INPUT               (0 << GPIO_CNF_SHIFT) /* Input */
#  define GPIO_OUTPUT              (1 << GPIO_CNF_SHIFT) /* Output */
#  define GPIO_PERIPHA             (2 << GPIO_CNF_SHIFT) /* Controlled by periph A signal */
#  define GPIO_PERIPHB             (3 << GPIO_CNF_SHIFT) /* Controlled by periph B signal */

/* These bits set the configuration of the pin:
 * ..CC C... .... ....
 */

#define GPIO_CFG_SHIFT             (11)        /* Bits 11-13: GPIO configuration bits */
#define GPIO_CFG_MASK              (3 << GPIO_CNF_SHIFT)
#  define GPIO_CFG_DEFAULT         (0 << GPIO_CNF_SHIFT) /* Default, no attribute */
#  define GPIO_CFG_PULLUP          (1 << GPIO_CNF_SHIFT) /* Bit 11: Internal pull-up */
#  define GPIO_CFG_DEGLITCH        (2 << GPIO_CNF_SHIFT) /* Bit 12: Internal glitch filter */
#  define GPIO_CFG_OPENDRAIN       (4 << GPIO_CNF_SHIFT) /* Bit 13: Open drain */

/* If the pin is an GPIO output, then this identifies the initial output value:
 * .... .... V... ....
 */

#define GPIO_OUTPUT_SET            (1 << 7)    /* Bit 7: Inital value of output */
#define GPIO_OUTPUT_CLEAR          (0) 

/* This identifies the GPIO port:
 * .... .... .PP. ....
 */

#define GPIO_PORT_SHIFT            (5)         /* Bit 5-6:  Port number */
#define GPIO_PORT_MASK             (3 << GPIO_PORT_SHIFT)
#  define GPIO_PORT_PIOA           (0 << GPIO_CNF_SHIFT)
#  define GPIO_PORT_PIOB           (1 << GPIO_CNF_SHIFT)
#  define GPIO_PORT_PIOC           (2 << GPIO_CNF_SHIFT)

/* This identifies the bit in the port:
 * .... .... ...B BBBB
 */

#define GPIO_PIN_SHIFT                0        /* Bits 0-3: GPIO number: 0-15 */
#define GPIO_PIN_MASK                 (15 << GPIO_PIN_SHIFT)
#define GPIO_PIN0                     (0  << GPIO_PIN_SHIFT)
#define GPIO_PIN1                     (1  << GPIO_PIN_SHIFT)
#define GPIO_PIN2                     (2  << GPIO_PIN_SHIFT)
#define GPIO_PIN3                     (3  << GPIO_PIN_SHIFT)
#define GPIO_PIN4                     (4  << GPIO_PIN_SHIFT)
#define GPIO_PIN5                     (5  << GPIO_PIN_SHIFT)
#define GPIO_PIN6                     (6  << GPIO_PIN_SHIFT)
#define GPIO_PIN7                     (7  << GPIO_PIN_SHIFT)
#define GPIO_PIN8                     (8  << GPIO_PIN_SHIFT)
#define GPIO_PIN9                     (9  << GPIO_PIN_SHIFT)
#define GPIO_PIN10                    (10 << GPIO_PIN_SHIFT)
#define GPIO_PIN11                    (11 << GPIO_PIN_SHIFT)
#define GPIO_PIN12                    (12 << GPIO_PIN_SHIFT)
#define GPIO_PIN13                    (13 << GPIO_PIN_SHIFT)
#define GPIO_PIN14                    (14 << GPIO_PIN_SHIFT)
#define GPIO_PIN15                    (15 << GPIO_PIN_SHIFT)
#define GPIO_PIN16                    (16 << GPIO_PIN_SHIFT)
#define GPIO_PIN17                    (17 << GPIO_PIN_SHIFT)
#define GPIO_PIN18                    (18 << GPIO_PIN_SHIFT)
#define GPIO_PIN19                    (19 << GPIO_PIN_SHIFT)
#define GPIO_PIN20                    (20 << GPIO_PIN_SHIFT)
#define GPIO_PIN21                    (21 << GPIO_PIN_SHIFT)
#define GPIO_PIN22                    (22 << GPIO_PIN_SHIFT)
#define GPIO_PIN23                    (23 << GPIO_PIN_SHIFT)
#define GPIO_PIN24                    (24 << GPIO_PIN_SHIFT)
#define GPIO_PIN25                    (25 << GPIO_PIN_SHIFT)
#define GPIO_PIN26                    (26 << GPIO_PIN_SHIFT)
#define GPIO_PIN27                    (27 << GPIO_PIN_SHIFT)
#define GPIO_PIN28                    (28 << GPIO_PIN_SHIFT)
#define GPIO_PIN29                    (29 << GPIO_PIN_SHIFT)
#define GPIO_PIN30                    (30 << GPIO_PIN_SHIFT)
#define GPIO_PIN31                    (31 << GPIO_PIN_SHIFT)

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Inline Functions
 ************************************************************************************/

#ifndef __ASSEMBLY__

/************************************************************************************
 * Public Data
 ************************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/

/************************************************************************************
 * Name: sam3u_clockconfig
 *
 * Description:
 *   Called to initialize the SAM3U.  This does whatever setup is needed to put the
 *   SoC in a usable state.  This includes the initialization of clocking using the
 *   settings in board.h.  (After power-on reset, the sam3u is initiallyrunning on
 *   a 4MHz internal RC clock).  This function also performs other low-level chip
 *   initialization of the chip including EFC, master clock, IRQ and watchdog
 *   configuration.
 *
 ************************************************************************************/

EXTERN void sam3u_clockconfig(void);

/************************************************************************************
 * Name: sam3u_lowsetup
 *
 * Description:
 *   Called at the very beginning of _start.  Performs low level initialization
 *   including setup of the console UART.  This UART done early so that the serial
 *   console is available for debugging very early in the boot sequence.
 *
 ************************************************************************************/

EXTERN void sam3u_lowsetup(void);

/************************************************************************************
 * Name: sam3u_configgpio
 *
 * Description:
 *   Configure a GPIO pin based on bit-encoded description of the pin.
 *
 ************************************************************************************/

EXTERN int sam3u_configgpio(uint16_t cfgset);

/************************************************************************************
 * Name: sam3u_gpiowrite
 *
 * Description:
 *   Write one or zero to the selected GPIO pin
 *
 ************************************************************************************/

EXTERN void sam3u_gpiowrite(uint16_t pinset, bool value);

/************************************************************************************
 * Name: sam3u_gpioread
 *
 * Description:
 *   Read one or zero from the selected GPIO pin
 *
 ************************************************************************************/

EXTERN bool sam3u_gpioread(uint16_t pinset);

/************************************************************************************
 * Function:  sam3u_dumpgpio
 *
 * Description:
 *   Dump all GPIO registers associated with the port of the provided pin description.
 *
 ************************************************************************************/

#ifdef CONFIG_DEBUG
EXTERN int sam3u_dumpgpio(uint16_t pinset, const char *msg);
#else
#  define sam3u_dumpgpio(p,m)
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_SAM3U_SAM3U_INTERNAL_H */
