/************************************************************************************
 * arch/avr/src/at91uc3b/at91uc3_internal.h
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
 ************************************************************************************/

#ifndef __ARCH_AVR_SRC_AVR32_AT91UC3_INTERNAL_H
#define __ARCH_AVR_SRC_AVR32_AT91UC3_INTERNAL_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: up_clkinit
 *
 * Description:
 *   Initialiaze clock/PLL settings per the definitions in the board.h file.
 *
 ************************************************************************************/

extern void up_clkinitialize(void);

/******************************************************************************
 * Name: usart_reset
 *
 * Description:
 *   Reset a USART.
 *
 ******************************************************************************/

extern void usart_reset(uintptr_t usart_base);

/******************************************************************************
 * Name: usart_configure
 *
 * Description:
 *   Configure a USART as a RS-232 UART.
 *
 ******************************************************************************/

void usart_configure(uintptr_t usart_base, uint32_t baud, unsigned int parity,
                     unsigned int nbits, bool stop2);

/************************************************************************************
 * Name: up_consoleinit
 *
 * Description:
 *   Initialize a console for debug output.  This function is called very
 *   early in the intialization sequence to configure the serial console uart
 *   (only).
 *
 ************************************************************************************/

extern void up_consoleinit(void);

/************************************************************************************
 * Name: up_boardinit
 *
 * Description:
 *   This function must be provided by the board-specific logic in the directory
 *   configs/<board-name>/up_boot.c.
 *
 ************************************************************************************/

extern void up_boardinitialize(void);

#endif /* __ARCH_AVR_SRC_AVR32_AT91UC3_INTERNAL_H */

