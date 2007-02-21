/************************************************************
 * irq.h
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

#ifndef __IRQ_H
#define __IRQ_H

/************************************************************
 * Included Files
 ************************************************************/

#ifndef __ASSEMBLY__
# include <sys/types.h>
# include <assert.h>
#endif

/************************************************************
 * Definitions
 ************************************************************/

#ifndef __ASSEMBLY__
# define irq_detach(isr)   irq_attach(isr, NULL)
#endif

/************************************************************
 * Public Types
 ************************************************************/

/* This struct defines the way the registers are stored */

#ifndef __ASSEMBLY__
typedef int (*xcpt_t)(int irq, void *context);
typedef int (*swint_t)(int code, int parm2, int parm3,
                       void *context);
#endif

/* Now include architecture-specific types */

#include <arch/irq.h>

/************************************************************
 * Public Variables
 ************************************************************/

#ifndef __ASSEMBLY__
struct xcptcontext *current_xcp;
#endif

/************************************************************
 * Public Function Prototypes
 ************************************************************/

#ifndef __ASSEMBLY__
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

EXTERN int irq_attach(int irq, xcpt_t isr);

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif

#endif /* __IRQ_H */

