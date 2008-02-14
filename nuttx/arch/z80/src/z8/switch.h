/************************************************************************************
 * arch/z80/src/z8/switch.h
 * arch/z80/src/chip/switch.h
 *
 *   Copyright (C) 2008 Gregory Nutt. All rights reserved.
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

#ifndef __Z80_SWITCH_H
#define __Z80_SWITCH_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <sys/types.h>
#include <nuttx/sched.h>
#include <nuttx/arch.h>
#include "up_internal.h"

/************************************************************************************
 * Definitions
 ************************************************************************************/

/* Macros for portability */

/* Initialize the IRQ state */

#define INIT_IRQCONTEXT()        current_regs = NULL

/* IN_INTERRUPT returns TRUE if the system is current operating in the interrupt
 * context.  IN_INTERRUPT is the inline equivalent of up_interrupt_context().
 */

#define IN_INTERRUPT()           (current_regs != NULL)

/* The following macro is used when the system enters interrupt handling logic */

#define IRQ_ENTER(irq, regs)     current_regs = (regs)

/* The following macro is used when the system exits interrupt handling logic */

#define IRQ_LEAVE(irq)           current_regs = NULL

/* The following macro is used to sample the interrupt state (as a opaque handle) */

#define IRQ_STATE()              (current_regs)

/* Copy a register state save structure to another location */

#define COPYSTATE(r1,r2)         z8_copystate(r1,r2)

/* Save the current IRQ context in the specified TCB */

#define SAVE_IRQCONTEXT(tcb)     COPYSTATE((tcb)->xcp.regs, current_regs)

/* Set the current IRQ context to the state specified in the TCB */

#define SET_IRQCONTEXT(tcb)      COPYSTATE(current_regs, (tcb)->xcp.regs)

/* Save the user context in the specified TCB.  User context saves can be simpler
 * because only those registers normally saved in a C called need be stored.
 */

#define SAVE_USERCONTEXT(tcb)    z8_saveusercontext((tcb)->xcp.regs)

/* Restore the full context -- either a simple user state save or the full,
 * IRQ state save.
 */

#define RESTORE_USERCONTEXT(tcb) z8_restorecontext((tcb)->xcp.regs)

/* Verify that we have a signal handler */

#define SIGNAL_DELIVERING(tcb)   (tcb->xcp.sigdeliver != NULL)

/* Setup the signal handler trampoline */

#define SIGNAL_SETUP(tcb, sigdeliver, regs)  z8_sigsetup(tcb, sigdeliver, regs)

/* Return from a signal handler using the provided register context */

#define SIGNAL_RETURN(regs)      z8_restorecontext(regs)

/* Dump the current machine registers */

#define _REGISTER_DUMP()         z8_registerdump()

/************************************************************************************
 * Public Variables
 ************************************************************************************/

#ifndef __ASSEMBLY__
/* This holds a references to the current interrupt level
 * register storage structure.  If is non-NULL only during
 * interrupt processing.
 */

extern chipreg_t *current_regs;
#endif

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/

#ifndef __ASSEMBLY__
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/* Defined in z8_copystate.c */

EXTERN void z8_copystate(FAR chipreg_t *dest, FAR const chipreg_t *src);

/* Defined in z8_saveusercontext.asm */

EXTERN int z8_saveusercontext(FAR chipreg_t *regs);

/* Defined in z8_restorecontext.asm */

EXTERN int z8_restorecontext(FAR chipreg_t *regs);

/* Defined in z8_sigsetup.c */

EXTERN void z8_sigsetup(FAR _TCB *tcb, sig_deliver_t sigdeliver, FAR chipreg_t *regs);

/* Defined in z8_registerdump.c */

EXTERN void z8_registerdump(void);

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __Z80_SWITCH_H */
