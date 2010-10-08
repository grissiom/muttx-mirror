/************************************************************************************
 * arch/avr/src/at91uc3/at91uc3_intc.h
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

#ifndef __ARCH_AVR_SRC_AT91UC3_AT91UC3_INTC_H
#define __ARCH_AVR_SRC_AT91UC3_AT91UC3_INTC_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

/* Register offsets *****************************************************************/

#define AVR32_INTC_PRIO_OFFSET(n)  ((n) << 2)           /* Interrupt priority registers */
#define AVR32_INTC_REQ_OFFSET(n)   (0x100 + ((n) << 2)) /* Interrupt request registers */
#define AVR32_INTC_CAUSE_OFFSET(n) (0x20c - ((n) << 2)) /* Interrupt cause registers */

/* Register Addresses ***************************************************************/

#define AVR32_INTC_PRIO(n)         (AVR32_INTC_BASE+AVR32_INTC_PRIO_OFFSET(n))
#define AVR32_INTC_REQ(n)          (AVR32_INTC_BASE+AVR32_INTC_REQ_OFFSET(n))
#define AVR32_INTC_CAUSE(n)        (AVR32_INTC_BASE+AVR32_INTC_CAUSE_OFFSET(n))

/* Register Bit-field Definitions ***************************************************/

/* Interrupt priority register bit-field definitions */

#define INTC_PRIO_AUTOVECTOR_SHIFT (0)       /* Bits 0-13: Autovector address */
#define INTC_PRIO_AUTOVECTOR_MASK  (0x3fff << INTC_PRIO_AUTOVECTOR_SHIFT)
#define INTC_PRIO_INTLEVEL_SHIFT   (30)      /* Bits 30-31: Interrupt Level */
#define INTC_PRIO_INTLEVEL_MASK    (3 << INTC_PRIO_INTLEVEL_SHIFT)
#  define INTC_PRIO_INTLEVEL_INT0  (0 << INTC_PRIO_INTLEVEL_SHIFT) /* Lowest priority */
#  define INTC_PRIO_INTLEVEL_INT1  (1 << INTC_PRIO_INTLEVEL_SHIFT)
#  define INTC_PRIO_INTLEVEL_INT2  (2 << INTC_PRIO_INTLEVEL_SHIFT)
#  define INTC_PRIO_INTLEVEL_INT3  (3 << INTC_PRIO_INTLEVEL_SHIFT) /* Highest priority */

/* Interrupt request register bit-field definitions */

#define INTC_PRIO(n)               (AVR32_INTC_PRIO((n) >> 5))
#define INTC_PRIO_IRR(n)           (1 << ((n) & 0x1f))

/* Interrupt cause register bit-field definitions */

#define INTC_CAUSE_SHIFT           (0)       /* Bits 0-5: Interrupt Group Causing Interrupt of Priority n */
#define INTC_CAUSE_MASK            (0x3f << INTC_CAUSE_SHIFT)

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/

#endif /* __ARCH_AVR_SRC_AT91UC3_AT91UC3_INTC_H */

