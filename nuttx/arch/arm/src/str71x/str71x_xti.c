/********************************************************************************
 * arch/arm/src/str71x/str71x_xti.c
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
 ********************************************************************************/

/********************************************************************************
 * Included Files
 ********************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <arch/board/board.h>

#include "chip.h"
#include "up_arch.h"
#include "os_internal.h"
#include "up_internal.h"

#ifdef CONFIG_STR71X_XTI

/********************************************************************************
 * Pre-procesor Definitions
 ********************************************************************************/

/********************************************************************************
 * Private Types
 ********************************************************************************/

struct xtiregs_s
{
  uint32_t mr; /* Mask register */
  uint32_t tr; /* Trigger polarity register */
};

/********************************************************************************
 * Public Data
 ********************************************************************************/

static const struct xtiregs_s g_xtiregs[2] =
{
  { STR71X_XTI_MRL, STR71X_XTI_TRL },
  { STR71X_XTI_MRH, STR71X_XTI_TRH }
};

/********************************************************************************
 * Private Data
 ********************************************************************************/

/********************************************************************************
 * Private Functions
 ********************************************************************************/

static int str7x_xtiinterrupt(int irq, FAR void *context)
{
  uint16_t enabled = (uint16_t)getreg8(STR71X_XTI_MRH) << 8 |
                     (uint16_t)getreg8(STR71X_XTI_MRL);
  uint16_t pending = (uint16_t)getreg8(STR71X_XTI_PRH) << 8 |
                     (uint16_t)getreg8(STR71X_XTI_PRL);
  uint16_t mask;

  /* Clear the interrupts now.  This seems unsafe?  Couldn't this cause lost
   * interupts?
   */

  mask = ~pending;
  putreg8(mask >> 8, STR71X_XTI_PRH);
  putreg8(mask & 0xff, STR71X_XTI_PRH);

  /* Then dispatch the interrupts */

  pending &= enabled;

  for (irq = STR71X_IRQ_FIRSTXTI, mask = 0x0001;
       irq < NR_IRQS && pending != 0;
       irq++, mask <<= 1)
    {
      /* Is this interrupt pending? */

      if ((pending & mask) != 0)
        {
	  /* Deliver the IRQ */

          irq_dispatch(irq, context);
          pending &= ~mask;
        }
    }
  return OK;
}

/********************************************************************************
 * Public Functions
 ********************************************************************************/

/********************************************************************************
 * Name: str7x_xtiinitialize
 *
 * Description:
 *   Configure XTI for operation.  Note that the lines are not used as wake-up
 *   sources in this implementation.  Some extensions would be required for that
 *   capability.
 *
 ********************************************************************************/

int str7x_xtiinitialize(void)
{
  int ret;

  /* Mask all interrupts by setting XTI MRH/L to zero */

  putreg8(0, STR71X_XTI_MRH);
  putreg8(0, STR71X_XTI_MRL);

  /* Clear any pending interrupts by setting XTI PRH/L to zero */

  putreg8(0, STR71X_XTI_PRH);
  putreg8(0, STR71X_XTI_PRL);

  /* Attach the XTI interrupt */

  ret = irq_attach(STR71X_IRQ_XTI, str7x_xtiinterrupt);
  if (ret == OK)
    {
      /* Enable the XTI interrupt at the XTI */

      putreg8(STR71X_XTICTRL_ID1S, STR71X_XTI_CTRL);

      /* And enable the XTI interrupt at the interrupt controller */

      up_enable_irq(STR71X_IRQ_XTI);
    }
  return ret;
}

/********************************************************************************
 * Name: str7x_xticonfig
 *
 * Description:
 *   Configure an external line to provide interrupts.
 *
 ********************************************************************************/

int str7x_xticonfig(int irq, bool rising)
{
  uint8_t  regval;
  int bit;
  int ndx;
  int ret = -EINVAL;

  /* Configure one of the 16 lines as an interrupt source */

  if (irq > STR71X_IRQ_FIRSTXTI && irq <= NR_IRQS)
    {
      /* Decide if we use the lower or upper regiser */
 
      bit = irq - STR71X_IRQ_FIRSTXTI;
      ndx = 0;
      if (bit > 7)
        {
          /* Select the high register */

          bit  -= 8;
          ndx  = 1;
        }

      /* Set the rising or trailing edge */

      regval = getreg8(g_xtiregs[ndx].tr);
      if (rising)
        {
          regval |= (1 << bit);
        }
      else
        {
          regval &= ~(1 << bit);
        }
      putreg8(regval, g_xtiregs[ndx].tr);

      /* Enable the interrupt be setting the corresponding mask bit
       * the XTI_MRL/H register.
       */

      regval = getreg8(g_xtiregs[ndx].mr);
      regval |= (1 << bit);
      putreg8(regval, g_xtiregs[ndx].mr);

      /* Return success */

      ret = OK;
    }
  return ret;
}

#endif /* CONFIG_STR71X_XTI */
