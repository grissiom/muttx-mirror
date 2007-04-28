/************************************************************************************
 * common/up_arch.h
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
 ************************************************************************************/

#ifndef __UP_ARCH_H
#define __UP_ARCH_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>
#ifndef __ASSEMBLY__
# include <sys/types.h>
#endif

#include "arm.h"
#include <arch/board/board.h>
#include "chip.h"

/************************************************************************************
 * Definitions
 ************************************************************************************/

/************************************************************************************
 * Inline Functions
 ************************************************************************************/

#ifndef __ASSEMBLY__

# define getreg8(a)           (*(volatile ubyte *)(a))
# define putreg8(v,a)         (*(volatile ubyte *)(a) = (v))
# define getreg32(a)          (*(volatile uint32 *)(a))
# define putreg32(v,a)        (*(volatile uint32 *)(a) = (v))

/* Some compiler options will convert short loads and stores into byte loads
 * and stores.  We don't want this to happen for IO reads and writes!
 */

/* # define getreg16(a)       (*(volatile uint16 *)(a)) */
static inline uint16 getreg16(unsigned int addr)
{
  uint16 retval;
 __asm__ __volatile__("\tldrh %0, [%1]\n\t" : "=r"(retval) : "r"(addr));
  return retval;
}

/* define putreg16(v,a)       (*(volatile uint16 *)(a) = (v)) */
static inline void putreg16(uint16 val, unsigned int addr)
{
 __asm__ __volatile__("\tstrh %0, [%1]\n\t": : "r"(val), "r"(addr));
}

/* Most DM320 registers are 16-bits wide */

#define getreg(a)   getreg16(1)
#define putreg(v,a) putreg16(v,a)

#endif

#endif  /* __UP_ARCH_H */
