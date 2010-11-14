/****************************************************************************
 * arch/arm/src/lpc17xx/lpc17_allocateheap.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <arch/board/board.h>

#include "chip.h"
#include "up_arch.h"
#include "up_internal.h"

#include "lpc17_memorymap.h"

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#if CONFIG_DRAM_START != LPC17_SRAM_BASE
#  warning "CONFIG_DRAM_START is not at LPC17_SRAM_BASE"
#  undef CONFIG_DRAM_START
#  undef CONFIG_DRAM_END
#  define CONFIG_DRAM_START LPC17_SRAM_BASE
#  define CONFIG_DRAM_END  (LPC17_SRAM_BASE+LPC17_CPUSRAM_SIZE)
#endif

#if CONFIG_DRAM_SIZE > LPC17_CPUSRAM_SIZE
#  warning "CONFIG_DRAM_SIZE is larger than the size of CPU SRAM"
#  undef CONFIG_DRAM_SIZE
#  undef CONFIG_DRAM_END
#  define CONFIG_DRAM_SIZE LPC17_CPUSRAM_SIZE
#  define CONFIG_DRAM_END (LPC17_SRAM_BASE+LPC17_CPUSRAM_SIZE)
#elif CONFIG_DRAM_SIZE < LPC17_CPUSRAM_SIZE
#  warning "CONFIG_DRAM_END is before end of CPU SRAM... not all of CPU SRAM used"
#endif

#ifdef LPC17_HAVE_BANK0
#  if CONFIG_MM_REGIONS < 2
#    warning "CONFIG_MM_REGIONS < 2: AHB SRAM Bank(s) not included in HEAP"
#  endif
#else
#  if CONFIG_MM_REGIONS > 1
#    warning "CONFIG_MM_REGIONS > 1: This MCU has no AHB SRAM Bank0/1"
#  endif
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   The heap may be statically allocated by
 *   defining CONFIG_HEAP_BASE and CONFIG_HEAP_SIZE.  If these
 *   are not defined, then this function will be called to
 *   dynamically set aside the heap region.
 *
 ****************************************************************************/

void up_allocate_heap(FAR void **heap_start, size_t *heap_size)
{
  up_ledon(LED_HEAPALLOCATE);
  *heap_start = (FAR void*)g_heapbase;
  *heap_size = CONFIG_DRAM_END - g_heapbase;
}

/************************************************************************
 * Name: up_addregion
 *
 * Description:
 *   Memory may be added in non-contiguous chunks.  Additional chunks are
 *   added by calling this function.
 *
 ************************************************************************/

#if CONFIG_MM_REGIONS > 1
void up_addregion(void)
{
  /* Banks 0 and 1 are each 16Kb.  If both are present, they occupy a
   * contiguous 32Kb memory region.
   *
   * If Ethernet is enabled, it will take all of bank 0 for packet
   * buffering and descriptor tables.
   */

#ifdef LPC17_HAVE_BANK0
#  if defined(CONFIG_NET) && defined(CONFIG_LPC17_ETHERNET) && defined(LPC17_NETHCONTROLLERS)
#    ifdef LPC17_HAVE_BANK1
       mm_addregion((FAR void*)LPC17_SRAM_BANK1, 16*1024);
#    endif
#  else
#    ifdef LPC17_HAVE_BANK1
       mm_addregion((FAR void*)LPC17_SRAM_BANK0, 32*1024);
#    else
       mm_addregion((FAR void*)LPC17_SRAM_BANK0, 16*1024);
#    endif
#  endif
#endif
}
#endif
