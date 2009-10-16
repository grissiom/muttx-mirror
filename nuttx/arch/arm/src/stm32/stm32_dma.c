/****************************************************************************
 * arch/arm/src/stm32/stm32_dma.c
 *
 *   Copyright (C) 2009 Gregory Nutt. All rights reserved.
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

#include <debug.h>
#include <errno.h>

#include <arch/irq.h>

#include "up_arch.h"
#include "os_internal.h"
#include "chip.h"
#include "stm32_dma.h"
#include "stm32_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DMA1_NCHANNELS   7
#if STM32_NDMA > 1
#  define DMA2_NCHANNELS 5
#  define DMA_NCHANNELS  (DMA1_NCHANNELS+DMA2_NCHANNELS)
#else
#  define DMA_NCHANNELS  DMA1_NCHANNELS
#endif

/* Convert the DMA channel base address to the DMA register block address */
 
#define DMA_BASE(ch)     (ch & 0xfffffc00)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Pinch a byte if is possible if there are not very many DMA channels */

#if DMA_NCHANNELS > 8
typedef uint16 dma_bitset_t;
#else
typedef uint8 dma_bitset_t;
#endif

/* This structure descibes one DMA channel */

struct stm32_dma_s
{
  ubyte          chan;     /* DMA channel number */
  ubyte          irq;      /* DMA channel IRQ number */
  uint32         base;     /* DMA register channel base address */
  dma_callback_t callback; /* Callback invoked when the DMA completes */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This bitset indicates which DMA channels have been allocated */

static dma_bitset_t g_dmaallocated;
static sem_t        g_allocsem;

/* This array describes the state of each DMA */

static struct stm32_dma_s g_dma[DMA_NCHANNELS] =
{
  {
	.chan     = STM32_DMA1_CHAN1,
	.irq      = STM32_IRQ_DMA1CH1,
    .base     = STM32_DMA1_BASE + STM32_DMACHAN_OFFSET(0),
  },
  {
	.chan     = STM32_DMA1_CHAN2,
	.irq      = STM32_IRQ_DMA1CH2,
    .base     = STM32_DMA1_BASE + STM32_DMACHAN_OFFSET(1),
  },
  {
	.chan     = STM32_DMA1_CHAN3,
	.irq      = STM32_IRQ_DMA1CH3,
    .base     = STM32_DMA1_BASE + STM32_DMACHAN_OFFSET(2),
  },
  {
	.chan     = STM32_DMA1_CHAN4,
	.irq      = STM32_IRQ_DMA1CH4,
    .base     = STM32_DMA1_BASE + STM32_DMACHAN_OFFSET(3),
  },
  {
	.chan     = STM32_DMA1_CHAN5,
	.irq      = STM32_IRQ_DMA1CH5,
    .base     = STM32_DMA1_BASE + STM32_DMACHAN_OFFSET(4),
  },
  {
	.chan     = STM32_DMA1_CHAN6,
	.irq      = STM32_IRQ_DMA1CH6,
    .base     = STM32_DMA1_BASE + STM32_DMACHAN_OFFSET(5),
  },
  {
	.chan     = STM32_DMA1_CHAN7,
	.irq      = STM32_IRQ_DMA1CH7,
    .base     = STM32_DMA1_BASE + STM32_DMACHAN_OFFSET(6),
  },
#if STM32_NDMA > 1
  {
	.chan     = STM32_DMA2_CHAN1,
	.irq      = STM32_IRQ_DMA2CH1,
    .base     = STM32_DMA2_BASE + STM32_DMACHAN_OFFSET(0),
  },
  {
	.chan     = STM32_DMA2_CHAN2,
	.irq      = STM32_IRQ_DMA2CH2,
    .base     = STM32_DMA2_BASE + STM32_DMACHAN_OFFSET(1),
  },
  {
	.chan     = STM32_DMA2_CHAN3,
	.irq      = STM32_IRQ_DMA2CH3,
    .base     = STM32_DMA2_BASE + STM32_DMACHAN_OFFSET(2),
  },
  {
	.chan     = STM32_DMA2_CHAN4,
	.irq      = STM32_IRQ_DMA2CH4,
    .base     = STM32_DMA2_BASE + STM32_DMACHAN_OFFSET(3),
  },
  {
	.chan     = STM32_DMA2_CHAN5,
	.irq      = STM32_IRQ_DMA2CH5,
    .base     = STM32_DMA2_BASE + STM32_DMACHAN_OFFSET(4),
  },
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * DMA register access functions
 ****************************************************************************/

/* Get non-channel register from DMA1 or DMA2 */

static inline uint32 dmabase_getreg(struct stm32_dma_s *dmach, uint32 offset)
{
  return getreg32(DMA_BASE(dmach->base) + offset);
}

/* Write to non-channel register in DMA1 or DMA2 */

static inline void dmabase_putreg(struct stm32_dma_s *dmach, uint32 offset, uint32 value)
{
  putreg32(value, DMA_BASE(dmach->base) + offset);
}

/* Get channel register from DMA1 or DMA2 */

static inline uint32 dmachan_getreg(struct stm32_dma_s *dmach, uint32 offset)
{
  return getreg32(dmach->base + offset);
}

/* Write to channel register in DMA1 or DMA2 */

static inline void dmachan_putreg(struct stm32_dma_s *dmach, uint32 offset, uint32 value)
{
  putreg32(value, dmach->base + offset);
}

/************************************************************************************
 * Name: stm32_dmainterrupt
 *
 * Description:
 *  DMA interrupt handler
 *
 ************************************************************************************/

static int stm32_dmainterrupt(int irq, void *context)
{
  struct stm32_dma_s *dmach;
  uint32 isr;
  int chan;

  /* Get the channel structure from the interrupt number */

  if (irq >= STM32_IRQ_DMA1CH1 && irq <= STM32_IRQ_DMA1CH7)
    {
      chan = irq - STM32_IRQ_DMA1CH1;
    }
  else
#if STM32_NDMA > 1
  if (irq >= STM32_IRQ_DMA2CH1 && irq <= STM32_IRQ_DMA2CH5)
    {
      chan = irq - STM32_IRQ_DMA2CH1 + DMA1_NCHANNELS;
    }
  else
#endif
    {
      PANIC(OSERR_INTERNAL);
    }
  dmach = &g_dma[chan];

  /* Get the interrupt status (for this channel only) */

  isr = dmabase_getreg(dmach, STM32_DMA_ISR_OFFSET) & ~DMA_ISR_CHAN_MASK(chan);

  /* Clear pending interrupts (for this channel only) */

  dmabase_putreg(dmach, STM32_DMA_IFCR_OFFSET, isr);

  /* Invoke the callback */

  if (dmach->callback)
    {
      dmach->callback(dmach, isr >> DMA_ISR_CHAN_SHIFT(chan));
    }
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_dmainitialize
 *
 * Description:
 *   Initialize the DMA subsystem
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void weak_function stm32_dmainitialize(void)
{
  int chan;

  /* Attach DMA interrupt vectors */

  for (chan = 0; chan < DMA_NCHANNELS; chan++)
    {
      irq_attach(g_dma[chan].irq, stm32_dmainterrupt);
    }
}

/****************************************************************************
 * Name: stm32_dmachannel
 *
 * Description:
 *   Allocate a DMA channel
 *
 * Returned Value:
 *   On success, a void* DMA channel handle; NULL on failure
 *
 ****************************************************************************/

DMA_HANDLE stm32_dmachannel(int chan)
{
  struct stm32_dma_s *dmach = NULL;
  irqstate_t          flags;
  dma_bitset_t        before;
  int                 bit = (1 << chan);

  DEBUGASSERT(chan < DMA_NCHANNELS);

  /* This is essentially a test and set.  We simply disable interrupts to
   * create the critical section.  This is brutal (but very quich) and assures
   * that we have exclusive access to the allocation bitset
   */

  flags           = irqsave();
  before          = g_dmaallocated;
  g_dmaallocated |= bit;
  irq_restore(flags);

  /* Was this channel been available? */

  if ((before & bit) == 0)
    {
      /* Yes.. then the caller has it, return it */

      dmach = &g_dma[chan];
    }
     
  return (DMA_HANDLE)dmach;
}

/****************************************************************************
 * Name: stm32_dmasetup
 *
 * Description:
 *   Configure DMA before using
 *
 ****************************************************************************/

void stm32_dmasetup(DMA_HANDLE handle, uint32 paddr, uint32 maddr, size_t ntransfers, uint32 ccr)
{
  struct stm32_dma_s *dmach = (struct stm32_dma_s *)handle;
  uint32 regval;

  /* Set the peripheral register address in the DMA_CPARx register. The data
   * will be moved from/to this address to/from the memory after the
   * peripheral event.
   */

  dmachan_putreg(dmach, STM32_DMACHAN_CPAR_OFFSET, paddr);

  /* Set the memory address in the DMA_CMARx register. The data will be
   * written to or read from this memory after the peripheral event.
   */

  dmachan_putreg(dmach, STM32_DMACHAN_CMAR_OFFSET, maddr);

  /* Configure the total number of data to be transferred in the DMA_CNDTRx
   * register.  After each peripheral event, this value will be decremented.
   */

  dmachan_putreg(dmach, STM32_DMACHAN_CNDTR_OFFSET, ntransfers);

  /* Configure the channel priority using the PL[1:0] bits in the DMA_CCRx
   * register.  Configure data transfer direction, circular mode, peripheral & memory
   * incremented mode, peripheral & memory data size, and interrupt after
   * half and/or full transfer in the DMA_CCRx register.
   */

  regval  = dmachan_gettreg(dmach, STM32_DMACHAN_CCR_OFFSET);
  regval &= ~(DMA_CCR_MEM2MEM|DMA_CCR_PL_MASK|DMA_CCR_MSIZE_MASK|DMA_CCR_PSIZE_MASK|
              DMA_CCR_MINC|DMA_CCR_PINC|DMA_CCR_CIRC|DMA_CCR_DIR);
  ccr    &=  (DMA_CCR_MEM2MEM|DMA_CCR_PL_MASK|DMA_CCR_MSIZE_MASK|DMA_CCR_PSIZE_MASK|
              DMA_CCR_MINC|DMA_CCR_PINC|DMA_CCR_CIRC|DMA_CCR_DIR);
  regval |= ccr;
  dmachan_putreg(dmach, STM32_DMACHAN_CCR_OFFSET, regval);
}

/****************************************************************************
 * Name: stm32_dmastart
 *
 * Description:
 *   Start the DMA transfer
 *
 ****************************************************************************/

void stm32_dmastart(DMA_HANDLE handle, dma_callback_t callback, boolean half)
{
  struct stm32_dma_s *dmach = (struct stm32_dma_s *)handle;
  int irq;
  uint32 ccr;

  DEBUGASSERT(handle != NULL);

  /* Save the callback.  This will be invoked whent the DMA commpletes */

  dmach->callback = callback;

  /* Activate the channel by setting the ENABLE bit in the DMA_CCRx register.
   * As soon as the channel is enabled, it can serve any DMA request from the
   * peripheral connected on the channel.
   */

  ccr  = dmachan_gettreg(dmach, STM32_DMACHAN_CCR_OFFSET);
  ccr |= DMA_CCR_EN;

  /* Once half of the bytes are transferred, the half-transfer flag (HTIF) is
   * set and an interrupt is generated if the Half-Transfer Interrupt Enable
   * bit (HTIE) is set. At the end of the transfer, the Transfer Complete Flag
   * (TCIF) is set and an interrupt is generated if the Transfer Complete
   * Interrupt Enable bit (TCIE) is set.
   */

  ccr |= (half ? (DMA_CCR_HTIE|DMA_CCR_TEIE) : (DMA_CCR_TCIE|DMA_CCR_TEIE));
  dmachan_putreg(dmach, STM32_DMACHAN_CCR_OFFSET, ccr);
}
