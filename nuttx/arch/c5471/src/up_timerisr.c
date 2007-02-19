/************************************************************
 * up_timerisr.c
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

/************************************************************
 * Included Files
 ************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <debug.h>
#include <nuttx/arch.h>
#include "clock_internal.h"
#include "up_internal.h"
#include "c5471.h"

/************************************************************
 * Definitions
 ************************************************************/

/* We want the general purpose timer running at the rate
 * MSEC_PER_TICK. The C5471 clock is 47.5MHz and we're using
 * a timer PTV value of 3 (3 == divide incoming frequency by
 * 16) which then yields a 16 bitCLKS_PER_INT value
 * of 29687.
 *
 *   47500000 / 16 = 2968750 clocks/sec
 *   2968750 / 100 = 29687   clocks/ 100Hz interrupt
 *
 */

#define CLKS_PER_INT       29687
#define CLKS_PER_INT_SHIFT 5
#define AR                 0x00000010
#define ST                 0x00000008
#define PTV                0x00000003

/************************************************************
 * Private Types
 ************************************************************/

/************************************************************
 * Private Function Prototypes
 ************************************************************/

/************************************************************
 * Global Functions
 ************************************************************/

/************************************************************
 * Function:  timer_isr
 *
 * Description:
 *   The timer ISR will perform a variety of services for
 *   various portions of the systems.
 *
 ************************************************************/

int up_timerisr(int irq, uint32 *regs)
{
  uint32 *saved_regs;

  /* Save the pointer to the interrupted context (exercising some
   * logic for the unexpected case of nested interrupts).
   */

  if (!current_regs)
    {
      saved_regs = NULL;
      current_regs = regs;
    }
  else
    {
      saved_regs = current_regs;
    }

   /* Process timer interrupt */

   sched_process_timer();

   /* Restore the previous context */

   current_regs = saved_regs;
   return 0;
}

void up_timerinit(void)
{
  uint32 val;

  up_disable_irq(C5471_IRQ_SYSTIMER);

  /* Start the general purpose timer running in auto-reload mode
   * so that an interrupt is generated at the rate MSEC_PER_TICK.
   */

  val = ((CLKS_PER_INT-1) << CLKS_PER_INT_SHIFT) | AR | ST | PTV;
  putreg32(val, C5471_TIMER2_CTRL);

  /* Attach and enable the timer interrupt */

  irq_attach(C5471_IRQ_SYSTIMER, (xcpt_t)up_timerisr);
  up_enable_irq(C5471_IRQ_SYSTIMER);
}

